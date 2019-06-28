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

    float voltage;          //��ǰ�豸�����ѹ
    uint8_t vol_precp;      //��ǰʣ�����

} REALTIME_DATA_TYPE;

typedef struct
{
    char dtime[32];         //��ǰ��ʷ���ݵ�ʱ��
    REALTIME_DATA_TYPE record;  //��ʷ����
    uint8_t isValid;        //��ǰ�����Ƿ���Ч
} HISTROY_DATA_TYPE;

void protocol_init(void);

void protocol_parse(uint8_t* recv_buff,uint16_t recv_len);

void protocol_analysis();

void protocol_destroy(void);

#endif // PROTOCOL_H_INCLUDED
