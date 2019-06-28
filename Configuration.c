#include "Configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CONFIG_TYPE curr_config;

uint8_t* config_file_path;
uint8_t* config_buffer = NULL;

void loadConfig(void)
{
    const char* split = "= \r\n";
    FILE* fp;
    fp = fopen((char *)config_file_path,"rb");
    if(fp != NULL)
    {
        config_buffer = (uint8_t *)malloc(1024);
        if(config_buffer != NULL)
        {
            memset(config_buffer,0,1024);
            fread(config_buffer,1,1024,fp);
            //------------------------------
            char* field = strtok((char *)config_buffer,split);
            if(strcmp(field,"Mode") == 0)
            {
                field = strtok(NULL,split);
                curr_config.mode = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"ComPort") == 0)
            {
                field = strtok(NULL,split);
                memset(curr_config.com_port,0,8);
                strcpy((char *)curr_config.com_port,field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"ComBaud") == 0)
            {
                field = strtok(NULL,split);
                curr_config.com_baudrate = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"ComData") == 0)
            {
                field = strtok(NULL,split);
                curr_config.com_bytebits = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"ComCheck") == 0)
            {
                field = strtok(NULL,split);
                curr_config.com_checkBits = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"ComStops") == 0)
            {
                field = strtok(NULL,split);
                curr_config.com_stopBits = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"StationId") == 0)
            {
                field = strtok(NULL,split);
                curr_config.station_id = atoi(field);
            }
            field = strtok(NULL,split);
            if(strcmp(field,"StationName") == 0)
            {
                field = strtok(NULL,split);
                memset((void *)curr_config.station_name,0,STATION_NAME_LEN);
                strcpy((char *)curr_config.station_name,field);
            }
        }
        free(config_buffer);
        config_buffer = NULL;
    }
    fclose(fp);
}

void saveConfig(void)
{
    FILE* fp;
    fp = fopen((char *)config_file_path,"wb");
    if(fp != NULL)
    {
        fprintf(fp,"Mode=%d\r\n",curr_config.mode);
        fprintf(fp,"ComPort=%s\r\n",curr_config.com_port);
        fprintf(fp,"ComBaud=%ld\r\n",curr_config.com_baudrate);
        fprintf(fp,"ComData=%d\r\n",curr_config.com_bytebits);
        fprintf(fp,"ComCheck=%d\r\n",curr_config.com_checkBits);
        fprintf(fp,"ComStops=%d\r\n",curr_config.com_stopBits);
        fprintf(fp,"StationId=%d\r\n",curr_config.station_id);
        fprintf(fp,"StationName=%s\r\n",curr_config.station_name);
    }
    fclose(fp);
}

