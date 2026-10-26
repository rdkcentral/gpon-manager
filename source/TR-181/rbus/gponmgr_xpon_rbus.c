#include "gponmgr_xpon_rbus.h"
#include "tr181/gponmgr_xpon_tr181.h"
#include <pthread.h>

static pthread_mutex_t g_lifecycleMutex = PTHREAD_MUTEX_INITIALIZER;
static rbusHandle_t g_handle;

int GponMgrXponRbus_Init(void)
{
    rbusError_t result;

    pthread_mutex_lock(&g_lifecycleMutex);
    if (g_handle)
    {
        pthread_mutex_unlock(&g_lifecycleMutex);
        return RBUS_ERROR_SUCCESS;
    }

    result = rbus_open(&g_handle, "GponManager.XPON");
    if (result == RBUS_ERROR_SUCCESS)
    {
        result = GponMgrXponTr181_Init(g_handle);
        if (result != RBUS_ERROR_SUCCESS)
        {
            GponMgrXponTr181_Close();
            rbus_close(g_handle);
            g_handle = NULL;
        }
    }
    else
        g_handle = NULL;

    pthread_mutex_unlock(&g_lifecycleMutex);
    return result;
}

void GponMgrXponRbus_Close(void)
{
    pthread_mutex_lock(&g_lifecycleMutex);
    if (g_handle)
    {
        GponMgrXponTr181_Close();
        rbus_close(g_handle);
        g_handle = NULL;
    }
    pthread_mutex_unlock(&g_lifecycleMutex);
}