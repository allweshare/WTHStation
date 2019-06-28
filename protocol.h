#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include <stdint.h>

#define REALTIME_DATA_LEN       46

typedef struct
{
    float tempt;
    float humid;
    float press;
    float rain;

    uint16_t wind_dir_3s;
    float wind_speed_3s;

    uint16_t wind_dir_1m;
    float wind_speed_1m;

    uint16_t wind_dir_10m;
    float wind_speed_10m;

    float voltage;          //当前设备供电电压
    uint8_t vol_precp;      //当前剩余电量

} REALTIME_DATA_TYPE;

typedef struct
{
    char dtime[32];         //当前历史数据的时间
    REALTIME_DATA_TYPE record;  //历史数据
    uint8_t isValid;        //当前数据是否有效
} HISTROY_DATA_TYPE;

void protocol_init(void);

void protocol_parse(uint8_t* recv_buff,uint16_t recv_len);

void protocol_analysis();

void protocol_destroy(void);

#endif // PROTOCOL_H_INCLUDED
