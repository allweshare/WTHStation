#ifndef COMSERVICE_H_INCLUDED
#define COMSERVICE_H_INCLUDED

#include <gtk/gtk.h>

#include <stdint.h>

#define SEND_BUFF_SIZE  1024
#define RECV_BUFF_SIZE  1024

/**
*   Serial Port Communication
**/
typedef struct
{
    GThread* listener;
    volatile int isQuit;
} ComService;

void ComServiceInit();

void ComServiceDestroy();

#endif // COMSERVICE_H_INCLUDED
