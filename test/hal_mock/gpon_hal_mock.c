/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file gpon_hal_mock.c
 * @brief Mock GPON JSON HAL server for GponManager integration tests.
 *
 * Replaces the vendor json_hal_server_gpon. It serves the
 * Device.X_RDK_ONT.* HAL data model from an in-memory store loaded from a
 * data file, and is driven at run time through a control FIFO so a test
 * script can change values, publish HAL events and inject faults.
 *
 * Control commands (one per line, written to the FIFO):
 *   set <name> <value...>             Update the store only (seen on the next poll).
 *   event <name> <value...>           Update the store and publish the event.
 *   publish <name> <value...>         Publish the event without touching the store.
 *   fail get|set|subscribe on|off     Reply with a failure to that action.
 *   hang get|set|subscribe <n>        Delay the next <n> requests past the client timeout.
 *   hangtime <seconds>                Delay used by "hang" (default 12 s; client timeout is 10 s).
 *   dump [prefix]                     Log the store (optionally filtered).
 *   subs                              Log the subscriptions received.
 *   sync <token>                      Log "SYNC <token>" once all earlier commands are done.
 *   quit                              Stop the server.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "json_hal_server.h"

#define MOCK_MAX_PARAMS   1024
#define MOCK_MAX_SUBS     64
#define MOCK_NAME_LEN     256
#define MOCK_VALUE_LEN    2048
#define MOCK_LINE_LEN     (MOCK_NAME_LEN + MOCK_VALUE_LEN + 64)

#define DEFAULT_CONF      "/etc/rdk/conf/gpon_manager_wan_unify_conf.json"
#define DEFAULT_SCHEMA    "/etc/rdk/schemas/gpon_wan_unify_hal_schema.json"
#define DEFAULT_FIFO      "/tmp/gpon_hal_mock.ctl"

typedef struct
{
    char name[MOCK_NAME_LEN];
    char value[MOCK_VALUE_LEN];
    eParamType type;
} mock_param_t;

typedef enum
{
    ACT_GET = 0,
    ACT_SET,
    ACT_SUBSCRIBE,
    ACT_MAX
} mock_action_t;

static const char *const k_action_names[ACT_MAX] = {"get", "set", "subscribe"};

static mock_param_t g_params[MOCK_MAX_PARAMS];
static int g_param_count;
static char g_subs[MOCK_MAX_SUBS][MOCK_NAME_LEN];
static int g_sub_count;
static int g_fail[ACT_MAX];
static int g_hang[ACT_MAX];
static int g_hang_seconds = 12;
static char g_schema_path[1024] = DEFAULT_SCHEMA;
static const char *g_fifo_path = DEFAULT_FIFO;
static volatile sig_atomic_t g_stop;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;

static void mock_log(const char *fmt, ...)
{
    struct timeval tv;
    struct tm tm_now;
    va_list ap;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_now);
    pthread_mutex_lock(&g_log_lock);
    printf("%02d:%02d:%02d.%03ld MOCK ", tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, (long)(tv.tv_usec / 1000));
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
    pthread_mutex_unlock(&g_log_lock);
}

static int type_from_string(const char *text, eParamType *type)
{
    static const struct { const char *name; eParamType type; } k_types[] = {
        {"string", PARAM_STRING},
        {"boolean", PARAM_BOOLEAN},
        {"bool", PARAM_BOOLEAN},
        {"int", PARAM_INTEGER},
        {"unsignedInt", PARAM_UNSIGNED_INTEGER},
        {"uint", PARAM_UNSIGNED_INTEGER},
        {"long", PARAM_LONG},
        {"unsignedLong", PARAM_UNSIGNED_LONG},
        {"ulong", PARAM_UNSIGNED_LONG},
        {"hexBinary", PARAM_HEXBINARY},
        {"base64", PARAM_BASE64},
    };
    size_t i;

    for (i = 0; i < sizeof(k_types) / sizeof(k_types[0]); ++i)
    {
        if (strcmp(text, k_types[i].name) == 0)
        {
            *type = k_types[i].type;
            return 0;
        }
    }
    return -1;
}

static void copy_text(char *dst, size_t size, const char *src)
{
    snprintf(dst, size, "%s", src ? src : "");
}

/* Caller holds g_lock. */
static mock_param_t *store_find(const char *name)
{
    int i;

    for (i = 0; i < g_param_count; ++i)
        if (strcmp(g_params[i].name, name) == 0)
            return &g_params[i];
    return NULL;
}

/* Caller holds g_lock. New entries are created with @p type; existing ones keep theirs. */
static int store_put(const char *name, eParamType type, const char *value)
{
    mock_param_t *param = store_find(name);

    if (param == NULL)
    {
        if (g_param_count >= MOCK_MAX_PARAMS)
            return -1;
        param = &g_params[g_param_count++];
        copy_text(param->name, sizeof(param->name), name);
        param->type = type;
    }
    copy_text(param->value, sizeof(param->value), value);
    return 0;
}

static char *trim(char *text)
{
    char *end;

    while (*text == ' ' || *text == '\t')
        ++text;
    end = text + strlen(text);
    while (end > text && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
    return text;
}

/* Split "<word> <rest>" in place; returns the rest (possibly empty). */
static char *split_word(char *text)
{
    char *space = strpbrk(text, " \t");

    if (space == NULL)
        return text + strlen(text);
    *space++ = '\0';
    while (*space == ' ' || *space == '\t')
        ++space;
    return space;
}

/* Data file format: "<name> <type> <value...>"; '#' starts a comment line. */
static int load_data_file(const char *path)
{
    char line[MOCK_LINE_LEN];
    int line_no = 0, errors = 0;
    FILE *file = fopen(path, "r");

    if (file == NULL)
    {
        mock_log("ERROR cannot open data file %s: %s", path, strerror(errno));
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *name, *type_text, *value;
        eParamType type;

        ++line_no;
        name = trim(line);
        if (*name == '\0' || *name == '#')
            continue;
        type_text = split_word(name);
        value = split_word(type_text);
        if (*type_text == '\0' || type_from_string(type_text, &type) != 0 || store_put(name, type, value) != 0)
        {
            mock_log("ERROR %s:%d invalid entry", path, line_no);
            ++errors;
        }
    }
    pthread_mutex_unlock(&g_lock);
    fclose(file);
    mock_log("LOADED %d parameters from %s (%d errors)", g_param_count, path, errors);
    return errors ? -1 : 0;
}

/* Consume one pending "hang" for @p action and sleep past the client timeout. */
static void maybe_hang(mock_action_t action, const char *name)
{
    int hang = 0;

    pthread_mutex_lock(&g_lock);
    if (g_hang[action] > 0)
    {
        --g_hang[action];
        hang = g_hang_seconds;
    }
    pthread_mutex_unlock(&g_lock);
    if (hang > 0)
    {
        mock_log("HANG %s %s for %d s", k_action_names[action], name, hang);
        sleep((unsigned int)hang);
    }
}

static int is_failing(mock_action_t action)
{
    int fail;

    pthread_mutex_lock(&g_lock);
    fail = g_fail[action];
    pthread_mutex_unlock(&g_lock);
    return fail;
}

static int get_parameters_cb(const json_object *jmsg, int param_count, json_object *jreply)
{
    int index, i;

    for (index = 0; index < param_count; ++index)
    {
        hal_param_t request;
        size_t length;
        int matched = 0;

        memset(&request, 0, sizeof(request));
        if (json_hal_get_param((json_object *)jmsg, index, GET_REQUEST_MESSAGE, &request) != RETURN_OK)
        {
            mock_log("ERROR getParameters: cannot parse request %d", index);
            return RETURN_ERR;
        }
        maybe_hang(ACT_GET, request.name);
        if (is_failing(ACT_GET))
        {
            mock_log("GET %s -> injected failure", request.name);
            return RETURN_ERR;
        }

        /* A trailing '.' requests a whole object, as GponManager does for its tables. */
        length = strlen(request.name);
        pthread_mutex_lock(&g_lock);
        for (i = 0; i < g_param_count; ++i)
        {
            int match = (length > 0 && request.name[length - 1] == '.')
                            ? strncmp(g_params[i].name, request.name, length) == 0
                            : strcmp(g_params[i].name, request.name) == 0;
            if (match)
            {
                hal_param_t response;

                memset(&response, 0, sizeof(response));
                copy_text(response.name, sizeof(response.name), g_params[i].name);
                copy_text(response.value, sizeof(response.value), g_params[i].value);
                response.type = g_params[i].type;
                if (json_hal_add_param(jreply, GET_RESPONSE_MESSAGE, &response) != RETURN_OK)
                {
                    pthread_mutex_unlock(&g_lock);
                    return RETURN_ERR;
                }
                ++matched;
            }
        }
        pthread_mutex_unlock(&g_lock);
        mock_log("GET %s -> %d params", request.name, matched);
    }
    return RETURN_OK;
}

static int set_parameters_cb(const json_object *jmsg, int param_count, json_object *jreply)
{
    eResult_t result = RESULT_SUCCESS;
    int index;

    for (index = 0; index < param_count; ++index)
    {
        hal_param_t request;

        memset(&request, 0, sizeof(request));
        if (json_hal_get_param((json_object *)jmsg, index, SET_REQUEST_MESSAGE, &request) != RETURN_OK)
        {
            mock_log("ERROR setParameters: cannot parse request %d", index);
            return RETURN_ERR;
        }
        maybe_hang(ACT_SET, request.name);
        if (is_failing(ACT_SET))
        {
            mock_log("SET %s=%s -> injected failure", request.name, request.value);
            result = RESULT_FAILURE;
            continue;
        }
        pthread_mutex_lock(&g_lock);
        if (store_put(request.name, request.type, request.value) != 0)
            result = RESULT_FAILURE;
        pthread_mutex_unlock(&g_lock);
        mock_log("SET %s=%s -> %s", request.name, request.value, result == RESULT_SUCCESS ? "ok" : "store full");
    }
    return json_hal_add_result_status(jreply, result) == RETURN_OK ? RETURN_OK : RETURN_ERR;
}

static int subscribe_event_cb(const json_object *jmsg, int param_count, json_object *jreply)
{
    eResult_t result = RESULT_SUCCESS;
    int index;

    for (index = 0; index < param_count; ++index)
    {
        hal_subscribe_event_request_t request;

        memset(&request, 0, sizeof(request));
        if (json_hal_get_subscribe_event_request((json_object *)jmsg, index, &request) != RETURN_OK)
        {
            mock_log("ERROR subscribeEvent: cannot parse request %d", index);
            return RETURN_ERR;
        }
        maybe_hang(ACT_SUBSCRIBE, request.name);
        if (is_failing(ACT_SUBSCRIBE))
        {
            mock_log("SUBSCRIBE %s -> injected failure", request.name);
            result = RESULT_FAILURE;
            continue;
        }
        pthread_mutex_lock(&g_lock);
        if (g_sub_count < MOCK_MAX_SUBS)
            copy_text(g_subs[g_sub_count++], MOCK_NAME_LEN, request.name);
        pthread_mutex_unlock(&g_lock);
        mock_log("SUBSCRIBE %s type=%d", request.name, (int)request.type);
    }
    return json_hal_add_result_status(jreply, result) == RETURN_OK ? RETURN_OK : RETURN_ERR;
}

static int delete_object_cb(const json_object *jmsg, int param_count, json_object *jreply)
{
    (void)jmsg;
    mock_log("DELETE %d objects (ignored)", param_count);
    return json_hal_add_result_status(jreply, RESULT_SUCCESS) == RETURN_OK ? RETURN_OK : RETURN_ERR;
}

static int get_schema_cb(const json_object *jmsg, int param_count, json_object *jreply)
{
    hal_schema_response_t response;

    (void)jmsg;
    (void)param_count;
    memset(&response, 0, sizeof(response));
    copy_text(response.filepath, sizeof(response.filepath), g_schema_path);
    mock_log("SCHEMA %s", response.filepath);
    return json_hal_add_schema_response(jreply, &response) == RETURN_OK ? RETURN_OK : RETURN_ERR;
}

static int parse_action(const char *text, mock_action_t *action)
{
    int i;

    for (i = 0; i < ACT_MAX; ++i)
    {
        if (strcmp(text, k_action_names[i]) == 0)
        {
            *action = (mock_action_t)i;
            return 0;
        }
    }
    return -1;
}

static void publish(const char *name, const char *value)
{
    int rc = json_hal_server_publish_event(name, value);

    mock_log("PUBLISH %s=%s rc=%d", name, value, rc);
}

static void handle_command(char *line)
{
    char *command = trim(line), *args, *value;
    mock_action_t action;

    if (*command == '\0' || *command == '#')
        return;
    args = split_word(command);
    value = split_word(args);

    if (strcmp(command, "set") == 0 || strcmp(command, "event") == 0)
    {
        int rc;

        if (*args == '\0')
        {
            mock_log("ERROR usage: %s <name> <value>", command);
            return;
        }
        pthread_mutex_lock(&g_lock);
        rc = store_put(args, PARAM_STRING, value);
        pthread_mutex_unlock(&g_lock);
        mock_log("STORE %s=%s rc=%d", args, value, rc);
        if (command[0] == 'e')
            publish(args, value);
    }
    else if (strcmp(command, "publish") == 0)
    {
        if (*args == '\0')
            mock_log("ERROR usage: publish <name> <value>");
        else
            publish(args, value);
    }
    else if (strcmp(command, "fail") == 0 || strcmp(command, "hang") == 0)
    {
        if (parse_action(args, &action) != 0)
        {
            mock_log("ERROR unknown action '%s'", args);
            return;
        }
        pthread_mutex_lock(&g_lock);
        if (command[0] == 'f')
            g_fail[action] = (strcmp(value, "on") == 0);
        else
            g_hang[action] = atoi(value);
        pthread_mutex_unlock(&g_lock);
        mock_log("CONFIG %s %s %s", command, args, value);
    }
    else if (strcmp(command, "hangtime") == 0)
    {
        if (atoi(args) > 0)
            g_hang_seconds = atoi(args);
        mock_log("CONFIG hangtime %d", g_hang_seconds);
    }
    else if (strcmp(command, "dump") == 0)
    {
        int i;

        pthread_mutex_lock(&g_lock);
        for (i = 0; i < g_param_count; ++i)
            if (*args == '\0' || strncmp(g_params[i].name, args, strlen(args)) == 0)
                mock_log("DUMP %s=%s", g_params[i].name, g_params[i].value);
        pthread_mutex_unlock(&g_lock);
    }
    else if (strcmp(command, "subs") == 0)
    {
        int i;

        pthread_mutex_lock(&g_lock);
        for (i = 0; i < g_sub_count; ++i)
            mock_log("SUBS %s", g_subs[i]);
        pthread_mutex_unlock(&g_lock);
    }
    else if (strcmp(command, "sync") == 0)
        mock_log("SYNC %s", args);
    else if (strcmp(command, "quit") == 0)
        g_stop = 1;
    else
        mock_log("ERROR unknown command '%s'", command);
}

static void on_signal(int signo)
{
    (void)signo;
    g_stop = 1;
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s -d <data file> [-c <hal conf>] [-s <schema>] [-f <control fifo>]\n"
            "  -c  JSON HAL conf with server_port (default %s)\n"
            "  -s  schema path returned by getSchema (default %s)\n"
            "  -f  control FIFO (default %s)\n",
            prog, DEFAULT_CONF, DEFAULT_SCHEMA, DEFAULT_FIFO);
}

int main(int argc, char **argv)
{
    const char *conf_path = DEFAULT_CONF, *data_path = NULL;
    char line[MOCK_LINE_LEN];
    struct sigaction sa;
    FILE *fifo;
    int opt, fd;

    while ((opt = getopt(argc, argv, "c:d:s:f:h")) != -1)
    {
        switch (opt)
        {
        case 'c': conf_path = optarg; break;
        case 'd': data_path = optarg; break;
        case 's': copy_text(g_schema_path, sizeof(g_schema_path), optarg); break;
        case 'f': g_fifo_path = optarg; break;
        default: usage(argv[0]); return 2;
        }
    }
    if (data_path == NULL)
    {
        usage(argv[0]);
        return 2;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    if (load_data_file(data_path) != 0)
        return 1;

    unlink(g_fifo_path);
    if (mkfifo(g_fifo_path, 0600) != 0)
    {
        mock_log("ERROR mkfifo %s: %s", g_fifo_path, strerror(errno));
        return 1;
    }
    /* O_RDWR keeps a writer open, so the reader never sees EOF between commands. */
    fd = open(g_fifo_path, O_RDWR);
    if (fd < 0 || (fifo = fdopen(fd, "r")) == NULL)
    {
        mock_log("ERROR open %s: %s", g_fifo_path, strerror(errno));
        return 1;
    }

    if (json_hal_server_init(conf_path) != RETURN_OK)
    {
        mock_log("ERROR json_hal_server_init(%s) failed", conf_path);
        return 1;
    }
    json_hal_server_register_action_callback("getSchema", get_schema_cb);
    json_hal_server_register_action_callback("getParameters", get_parameters_cb);
    json_hal_server_register_action_callback("setParameters", set_parameters_cb);
    json_hal_server_register_action_callback("deleteObject", delete_object_cb);
    json_hal_server_register_action_callback("subscribeEvent", subscribe_event_cb);
    if (json_hal_server_run() != RETURN_OK)
    {
        mock_log("ERROR json_hal_server_run failed (port already in use?)");
        json_hal_server_terminate();
        return 1;
    }

    /* No SA_RESTART: a signal interrupts the blocking FIFO read so the loop can exit. */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    mock_log("READY conf=%s fifo=%s", conf_path, g_fifo_path);
    while (!g_stop)
    {
        if (fgets(line, sizeof(line), fifo) == NULL)
        {
            if (errno == EINTR && !g_stop)
            {
                clearerr(fifo);
                continue;
            }
            break;
        }
        handle_command(line);
    }

    mock_log("STOPPING");
    json_hal_server_terminate();
    fclose(fifo);
    unlink(g_fifo_path);
    return 0;
}
