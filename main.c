#include "main.h"
#include "MainFrame.h"
#include "OptionPane.h"
#include "protocol.h"
#include "Configuration.h"
#include "ComService.h"
#include "BLEService.h"
#include "DataBase.h"

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

volatile REALTIME_DATA_TYPE CURR_DATA;
volatile CONFIG_TYPE curr_config;
extern uint8_t* config_file_path;

uint8_t* root_dir_path;

void app_init(int argc, char* argv[])
{
    CURR_DATA.tempt = 0.0f;
    CURR_DATA.humid = 0.0f;
    CURR_DATA.press = 500.0f;
    CURR_DATA.rain = 0.0f;

    CURR_DATA.wind_dir_3s = 0;
    CURR_DATA.wind_speed_3s = 0.0f;

    CURR_DATA.wind_dir_1m = 0;
    CURR_DATA.wind_speed_1m = 0.0f;

    CURR_DATA.wind_dir_10m = 0;
    CURR_DATA.wind_speed_10m = 0.0f;

    CURR_DATA.voltage = 0.0f;
    CURR_DATA.vol_precp = 0;

    //------------------------------------------------------
    config_file_path = malloc(MAX_PATH_LEN);
    root_dir_path = malloc(MAX_PATH_LEN);

    char* cfgFile = strrchr(argv[0],'\\'); //Find last separator
    if(cfgFile > argv[0])
    {
        memset(config_file_path,0,MAX_PATH_LEN);
        memcpy(config_file_path,argv[0],(cfgFile-argv[0]+1));
        memcpy(root_dir_path,argv[0],(cfgFile-argv[0]+1));

        if(access((char *)config_file_path, F_OK) == 0)
        {
            memcpy(&config_file_path[(cfgFile-argv[0]+1)],CFG_FILE_NAME,10);
            PRINT_TRACE_INFO("Load Configuration:%s",config_file_path);
            if(access((char *)config_file_path, F_OK) == 0)
            {
                //加载配置文件
                loadConfig();
            }else{
                PRINT_ERROR_INFO("Create Configuration:%s",config_file_path);

                //Create default configuration if file not exists
                curr_config.mode = 1;
                memset((void *)curr_config.com_port,0,8);
                strcpy((char *)curr_config.com_port,"COM1");

                curr_config.com_baudrate = BAUD_9600;
                curr_config.com_bytebits = DATABITS_8;
                curr_config.com_checkBits = PARITY_NONE;
                curr_config.com_stopBits = STOPBITS_10;

                curr_config.station_id = 0;     //默认未指定观测点
                memset((void *)curr_config.station_name,0,STATION_NAME_LEN);
                strcpy((char *)curr_config.station_name,"默认观测点");

                //Write Configuration
                saveConfig();
            }
        }else{
            PRINT_ERROR_INFO("Error? Please Check Dir: %s \r\n",config_file_path);
        }
    }else{
        PRINT_ERROR_INFO("Error? Please Check Path: %s \r\n",argv[0]);
    }
    //------------------------------------------------------
    //Initialize Database
    database_init(argc, argv);
    //------------------------------------------------------
    protocol_init();

    if(curr_config.mode == 1)       //SerialPort
    {
        ComServiceInit();
    }
    if(curr_config.mode == 2)       //Bluetooth
    {
        BLEServiceInit();
    }
}

void app_exit()
{
    ComServiceDestroy();
    BLEServiceDestroy();

//    if(curr_config.mode == 1)       //SerialPort
//    {
//        ComServiceDestroy();
//    }
//    if(curr_config.mode == 2)       //Bluetooth
//    {
//        BLEServiceDestroy();
//    }

    protocol_destroy();

    database_close();

    //应用程序退出
    option_pane_release();

    if(config_file_path != NULL)
    {
        free(config_file_path);
        config_file_path = NULL;
    }

    if(root_dir_path != NULL)
    {
        free(root_dir_path);
        root_dir_path = NULL;
    }

    PRINT_TRACE_INFO("App Exit");
}


int main(int argc, char* argv[])
{
    //Do some thing before app started
    app_init(argc,argv);

    //------------------------------------
    //应用程序初始化
    init_layout(argc,argv);


    //system("pause");
    return 0;
}

