#ifndef BLESERVICE_H_INCLUDED
#define BLESERVICE_H_INCLUDED

#include <gtk/gtk.h>

#define DLL_SERVICE_X86     "BLEService_x86.dll"
#define DLL_SERVICE_X64     "BLEService_x64.dll"

/**
*   Bluetooth Communication
**/
typedef struct
{
    GThread* listener;
    volatile int isQuit;
} BLEService;

typedef int (__cdecl *InitBLEServiceFunc)();
typedef int (__cdecl *ReadBLECharacsFunc)(char* optBuff, size_t* optBytes);
typedef void (__cdecl *DestroyBLEServiceFunc)();

void BLEServiceInit();

void BLEServiceDestroy();

#endif // BLESERVICE_H_INCLUDED
