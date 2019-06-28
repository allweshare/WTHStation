#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#include <stdint.h>
#include <windows.h>

#define CFG_FILE_NAME   "config.ini"
#define MAX_PATH_LEN    256


#define STATION_NAME_LEN    64

typedef struct
{
    uint8_t mode;               //Current Mode; 1: Serial Port; 2: BlueTooth Low Energy

    uint8_t com_port[8];        //Com x: Port
    DWORD com_baudrate;         //BAUD_4800,BAUD_9600,BAUD_115200 ...
    WORD com_bytebits;          //DataBits,DATABITS_5,DATABITS_6,DATABITS_7,DATABITS_8
    WORD com_checkBits;         //PARITY_NONE,PARITY_ODD,PARITY_EVEN
    WORD com_stopBits;          //STOPBITS_10,STOPBITS_15,STOPBITS_20

    int station_id;
    uint8_t station_name[STATION_NAME_LEN];        //Current Station Name
} CONFIG_TYPE;

void loadConfig(void);

void saveConfig(void);

#endif // CONFIGURATION_H_INCLUDED
