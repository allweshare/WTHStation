#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <stdint.h>
#include <gtk/gtk.h>

#define APP_TITLE_NAME  "数据接收和终端处理系统软件"

#define APP_DEFAULT_WIDTH       800
#define APP_DEFAULT_HEIGHT      600

#define APP_TITLE_HEIGHT        30          //文本高度，不是容器高度

#define APP_BKG_RGB             0.1843, 0.3, 0.3
#define RT_BKG_GRB              0, 0, 0             //实时数据面板颜色
#define CS_BKG_GRB              0, 0, 0             //通信状态面板颜色
#define DS_BKG_GRB              0, 0, 0             //设备状态面板颜色
#define BTN_BKG_GRB             0.8, 0.8, 0.8       //设备状态面板颜色


#define PRINT_TRACE_INFO(fmt,args...)  fprintf(stderr,"[TRACE:] "fmt" ! File: %s | Line: %d .\r\n",##args,__FILE__,__LINE__);
#define PRINT_ERROR_INFO(fmt,args...)  fprintf(stderr,"[ERROR:] "fmt" ! File: %s | Line: %d .\r\n",##args,__FILE__,__LINE__);

/**
*   自定义按钮数据结构
**/
typedef struct
{
    const char label[32];               //按钮标签
    cairo_rectangle_t rect;             //按钮的位置和大小
    uint8_t isMoveIn;                   //鼠标移入
    void (* click)(double x,double y);  //鼠标点击
} CUSTOM_BUTTON;

void app_init(int argc, char* argv[]);

void app_exit();


#endif // MAIN_H_INCLUDED
