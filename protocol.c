/**
*   协议层
*   封装所有数据格式相关代码
**/

#include "protocol.h"
#include "ComService.h"
#include "Configuration.h"
#include "MainFrame.h"
#include "DataBase.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BATTRY_VMIN     9.0f
#define BATTRY_VMAX     12.0f

extern volatile REALTIME_DATA_TYPE CURR_DATA;

uint8_t* REALTIME_DATA_BUFF = NULL;

uint16_t WDIR_1MIN_FIFO[60000/SAMPLING_INTERVEL];
float WSPEED_1MIN_FIFO[60000/SAMPLING_INTERVEL];
uint16_t WDIR_10MIN_FIFO[60000/SAMPLING_INTERVEL*10];
float WSPEED_10MIN_FIFO[60000/SAMPLING_INTERVEL*10];

void protocol_init(void)
{
    REALTIME_DATA_BUFF = (uint8_t *)malloc(REALTIME_DATA_LEN * 2);
    memset(REALTIME_DATA_BUFF,0,REALTIME_DATA_LEN * 2);
    //--------------------------------------
    memset(WDIR_1MIN_FIFO,0,sizeof(uint16_t)*(60000/SAMPLING_INTERVEL));
    memset(WSPEED_1MIN_FIFO,0,sizeof(float)*(60000/SAMPLING_INTERVEL));
    memset(WDIR_10MIN_FIFO,0,sizeof(uint16_t)*(60000/SAMPLING_INTERVEL*10));
    memset(WSPEED_10MIN_FIFO,0,sizeof(float)*(60000/SAMPLING_INTERVEL*10));
}

/**
*   按照协议解析指定的传感器响应数据
**/
void protocol_parse(uint8_t* recv_buff,uint16_t recv_len)
{
    static uint32_t joint_index = 0;

    //Check Start Flag
    if(recv_buff[0] == '0')
    {
        joint_index = 0;
    }

    //Check overflow
    if(joint_index + recv_len > REALTIME_DATA_LEN)
    {
        PRINT_ERROR_INFO("Protocol Parse OverFlow");

        joint_index = 0;
    }else{
        memcpy(&REALTIME_DATA_BUFF[joint_index],recv_buff,recv_len);
        joint_index += recv_len;
    }

    //Check End Flag
    if(joint_index > 0 && REALTIME_DATA_BUFF[joint_index-1] == 0x0A)
    {
        //Parse String by protocol
        //char cvt_buff[8];
        char* field = NULL;
        {
            switch(REALTIME_DATA_BUFF[2])
            {
                case 0x31:         //风向,风速
                    field = strtok((char *)REALTIME_DATA_BUFF,"=,\r\n");    //OR1
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Dm
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //风向
                    CURR_DATA.wind_dir_3s = atoi(field);
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Sm
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //风速
                    CURR_DATA.wind_speed_3s = atof(field);
                    break;
                case 0x32:         //温度，湿度，气压
                    field = strtok((char *)REALTIME_DATA_BUFF,"=,\r\n");    //OR2
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Ta
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //温度
                    CURR_DATA.tempt = atof(field);
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Ua
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //湿度
                    CURR_DATA.humid = atof(field);
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Pa
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //气压
                    CURR_DATA.press = atof(field);
                    break;
                case 0x33:         //分钟雨量
                    field = strtok((char *)REALTIME_DATA_BUFF,"=,\r\n");    //OR3
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Rc
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //雨量
                    CURR_DATA.rain = atof(field);
                    break;
                case 0x35:         //设备电压
                    field = strtok((char *)REALTIME_DATA_BUFF,"=,\r\n");    //OR5
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //Vs
                    if(field == NULL)   return;
                    field = strtok(NULL,"=,\r\n");      //电压
                    CURR_DATA.voltage = atof(field);
                    break;
                default:
                    break;
            }

        }
    }
}

/**
*   传感器对应的测量数据分析
*   包括 风向风速的滑动平均
**/
void protocol_analysis()
{
    //一分钟平均风
    memmove(&WDIR_1MIN_FIFO[1],&WDIR_1MIN_FIFO[0],sizeof(uint16_t)*(60000/SAMPLING_INTERVEL-1));
    WDIR_1MIN_FIFO[0] = CURR_DATA.wind_dir_3s;
    memmove(&WSPEED_1MIN_FIFO[1],&WSPEED_1MIN_FIFO[0],sizeof(float)*(60000/SAMPLING_INTERVEL-1));
    WSPEED_1MIN_FIFO[0] = CURR_DATA.wind_speed_3s;
    {
        double a = 0.0,b = 0.0;
        double speed = 0.0;
        int i = 0;
        for(i = 0;i < 60000/SAMPLING_INTERVEL;i ++)
        {
            a += sin(M_PI / 180.0 * (double)WDIR_1MIN_FIFO[i]);
            b += cos(M_PI / 180.0 * (double)WDIR_1MIN_FIFO[i]);

            speed += WSPEED_1MIN_FIFO[i];
        }
        a = a / (double)(60000/SAMPLING_INTERVEL);
        b = b / (double)(60000/SAMPLING_INTERVEL);

        speed = speed / (double)(60000/SAMPLING_INTERVEL);
        double angle = 180.0 / M_PI * atan2(a,b);
        if(angle <= 0)
        {
            angle += 360.0;
        }
        //PRINT_TRACE_INFO("Angle: %0.1f ",angle);
        CURR_DATA.wind_dir_1m = (uint16_t)angle;
        CURR_DATA.wind_speed_1m = (float)speed;
    }
    //-----------------------------------------
    //十分钟平均风
    memmove(&WDIR_10MIN_FIFO[1],&WDIR_10MIN_FIFO[0],sizeof(uint16_t)*(60000/SAMPLING_INTERVEL*10-1));
    WDIR_10MIN_FIFO[0] = CURR_DATA.wind_dir_3s;
    memmove(&WSPEED_10MIN_FIFO[1],&WSPEED_10MIN_FIFO[0],sizeof(float)*(60000/SAMPLING_INTERVEL*10-1));
    WSPEED_10MIN_FIFO[0] = CURR_DATA.wind_speed_3s;
    {
        double a = 0.0,b = 0.0;
        double speed = 0.0;
        int i = 0;
        for(i = 0;i < 60000/SAMPLING_INTERVEL*10;i ++)
        {
            a += sin(M_PI / 180.0 * (double)WDIR_10MIN_FIFO[i]);
            b += cos(M_PI / 180.0 * (double)WDIR_10MIN_FIFO[i]);

            speed += WSPEED_10MIN_FIFO[i];
        }
        a = a / (double)(60000/SAMPLING_INTERVEL*10);
        b = b / (double)(60000/SAMPLING_INTERVEL*10);

        speed = speed / (double)(60000/SAMPLING_INTERVEL*10);
        double angle = 180.0 / M_PI * atan2(a,b);
        if(angle <= 0)
        {
            angle += 360.0;
        }
        //PRINT_TRACE_INFO("Angle: %0.1f ",angle);
        CURR_DATA.wind_dir_10m = (uint16_t)angle;
        CURR_DATA.wind_speed_10m = (float)speed;
    }

    //---------------------------
    //当前版本使用直流供电
    CURR_DATA.vol_precp = 100;
}


void protocol_destroy(void)
{
    free(REALTIME_DATA_BUFF);
}

