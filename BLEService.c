#include "BLEService.h"
#include "protocol.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

InitBLEServiceFunc initBLEService = NULL;
ReadBLECharacsFunc readBLECharacs = NULL;
DestroyBLEServiceFunc destroyBLEService = NULL;

static GMutex bleMutex;
static HINSTANCE hLibInstance = NULL;
static BLEService* bleService = NULL;

static uint8_t* BLE_RECIVE_BUFF = NULL;            //BLE接收缓冲区

static gboolean bleHandler(gpointer user_data)
{
    BLEService* service = (BLEService *)user_data;

    while(1)
    {
        Sleep(1000);

        size_t numReads;
        readBLECharacs((char *)BLE_RECIVE_BUFF,&numReads);
        if(numReads == REALTIME_DATA_LEN)
        {
            PRINT_TRACE_INFO("Recive From BLE: %.16s... ",(char *)BLE_RECIVE_BUFF);
            //解析数据
            protocol_parse(BLE_RECIVE_BUFF,REALTIME_DATA_LEN);
        }

        g_mutex_lock(&bleMutex);
        if(service->isQuit == 1)
        {
            g_mutex_unlock(&bleMutex);
            break;
        }
        g_mutex_unlock(&bleMutex);
    }
    g_thread_exit(user_data);
    return G_SOURCE_CONTINUE;
}

void BLEServiceInit()
{
    if(hLibInstance != NULL)
    {
        FreeLibrary(hLibInstance);
        hLibInstance = NULL;
    }

#ifndef __WIN64
    PRINT_TRACE_INFO("Load Library for x86");
    hLibInstance = LoadLibrary(TEXT("BLEService_x86.dll"));
#else
    PRINT_TRACE_INFO("Load Library for x64");
    hLibInstance = LoadLibrary(TEXT("BLEService_x64.dll"));
#endif // __WIN64

    if(hLibInstance == NULL)
    {
        PRINT_TRACE_INFO("Load Library Error");
    }else{
        //绑定函数
        initBLEService = (InitBLEServiceFunc)GetProcAddress(hLibInstance,"InitBLEService");
        readBLECharacs = (ReadBLECharacsFunc)GetProcAddress(hLibInstance,"ReadBLECharacs");
        destroyBLEService = (DestroyBLEServiceFunc)GetProcAddress(hLibInstance,"DestroyBLEService");

        if((initBLEService == NULL) || (readBLECharacs == NULL) || (destroyBLEService == NULL))
        {
            PRINT_ERROR_INFO("Function Binding Error");
        }
        //--------------------------------------------------
        //初始化BLE通信接口
        if(initBLEService != NULL)
        {
            initBLEService();
            PRINT_TRACE_INFO("BLE Service Init ...");

            BLE_RECIVE_BUFF = malloc(REALTIME_DATA_LEN);

            bleService = malloc(sizeof(BLEService));
            memset(bleService,0,sizeof(BLEService));
            bleService->isQuit = 0;
            bleService->listener = g_thread_new(NULL,(GThreadFunc)bleHandler,bleService);

        }
    }
}

void BLEServiceDestroy()
{
    if(bleService != NULL)
    {
        bleService->isQuit = 1;
    }

    if(destroyBLEService != NULL)
    {
        destroyBLEService();
        destroyBLEService = NULL;
    }

    if(BLE_RECIVE_BUFF != NULL)
    {
        free(BLE_RECIVE_BUFF);
        BLE_RECIVE_BUFF = NULL;
    }

    if(bleService != NULL)
    {
        free(bleService);
    }
}


