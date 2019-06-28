#include "ComService.h"
#include "Configuration.h"
#include "protocol.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

extern volatile CONFIG_TYPE curr_config;

uint8_t* COM_RECV_BUFF;     //Recive Buffer
uint8_t* COM_SEND_BUFF;     //Send Buffer

static GMutex comMutex;
static ComService* comService = NULL;

int ComPortRead(HANDLE hComPort)
{
    DWORD dwBytesRead = RECV_BUFF_SIZE;
    BOOL bReadRet;
    DWORD dwErrorFlag;
    COMSTAT statCom;
    OVERLAPPED ovlpRead;

    ClearCommError(hComPort,&dwErrorFlag,&statCom);
    if(!statCom.cbInQue)
    {
        return -1;      //immediate return if recive buffer is empty
    }

    memset(&ovlpRead,0,sizeof(OVERLAPPED));
    ovlpRead.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    bReadRet = ReadFile(hComPort,COM_RECV_BUFF,RECV_BUFF_SIZE,&dwBytesRead,&ovlpRead);
    if(!bReadRet)       //May Not Error if Overlapped Function return False
    {
        if(GetLastError() == ERROR_IO_PENDING)      //Recive Buffer not empty
        {
            GetOverlappedResult(hComPort,&ovlpRead,&dwBytesRead,TRUE);      //Wait blocking until io-pending success
            //------------------------------------------------------------
            //read and parse data
            protocol_parse(COM_RECV_BUFF,dwBytesRead);
        }
    }
    return dwBytesRead;
}

static gboolean comHandler(gpointer user_data)
{
    HANDLE hComPort;

    ComService* service = (ComService *)user_data;
    //-----------------------------
    //Open SerialPort for recive
    hComPort = CreateFile((char *)curr_config.com_port,         //SerialPort
                          GENERIC_READ|GENERIC_WRITE,           //Mode
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_OVERLAPPED,                 //Async SerialPort
                          NULL);

    if(hComPort == INVALID_HANDLE_VALUE)
    {
        PRINT_ERROR_INFO("Can't open SerialPort: %s ",curr_config.com_port);
    }else{
        PRINT_TRACE_INFO("Open SerialPort %s Success ... ",curr_config.com_port);

        DCB comPortDCB;
        //设置输入输出缓冲区大小
        SetupComm(hComPort,RECV_BUFF_SIZE,SEND_BUFF_SIZE);

        COMMTIMEOUTS commTimeout;
        //Setup Read TimeOut
        commTimeout.ReadIntervalTimeout = 100;
        commTimeout.ReadTotalTimeoutMultiplier = 100;
        commTimeout.ReadTotalTimeoutConstant = 200;
        //Setup Write TimeOut
        commTimeout.WriteTotalTimeoutMultiplier = 100;
        commTimeout.WriteTotalTimeoutConstant = 100;
        SetCommTimeouts(hComPort,&commTimeout);

        //获取到之前的串口参数
        GetCommState(hComPort,&comPortDCB);

        comPortDCB.BaudRate = CBR_9600;
        comPortDCB.ByteSize = 8;
        comPortDCB.Parity = NOPARITY;
        comPortDCB.StopBits = ONESTOPBIT;

        //修改串口参数
        SetCommState(hComPort,&comPortDCB);
        //清空发送和接收缓冲区
        PurgeComm(hComPort,PURGE_TXCLEAR|PURGE_RXCLEAR);

        //---------------------------------------
        while(1)
        {
            /**
            *   add intevel between 10 and 50 ms
            **/
            Sleep(10);

            ComPortRead(hComPort);
            //--------------------------------------
            g_mutex_lock(&comMutex);
            if(service->isQuit)
            {
                g_mutex_unlock(&comMutex);
                //线程结束后必须关闭串口
                CloseHandle(hComPort);

                PRINT_TRACE_INFO("Close SerialPort: %s",curr_config.com_port);

                g_thread_exit(user_data);
            }
            g_mutex_unlock(&comMutex);
        }
    }
    return G_SOURCE_CONTINUE;
}

void ComServiceInit()
{
    comService = malloc(sizeof(ComService));
    memset(comService,0,sizeof(ComService));
    comService->isQuit = 0;

    COM_RECV_BUFF = malloc(RECV_BUFF_SIZE);
    COM_SEND_BUFF = malloc(SEND_BUFF_SIZE);
    memset(COM_RECV_BUFF,0,RECV_BUFF_SIZE);
    memset(COM_SEND_BUFF,0,SEND_BUFF_SIZE);
    //-------------------------
    //Create Thread for listener
    if((COM_RECV_BUFF != NULL) && (COM_SEND_BUFF != NULL))
    {
        comService->listener = g_thread_new(NULL,(GThreadFunc)comHandler,comService);
    }
}

void ComServiceDestroy()
{
    free(COM_RECV_BUFF);
    free(COM_SEND_BUFF);

    if(comService != NULL)
    {
        comService->isQuit = 1;
    }
    if(comService != NULL)
    {
        free(comService);
    }
}

