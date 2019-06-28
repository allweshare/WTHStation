#include "StatusPane.h"
#include "CustomDraw.h"
#include "Configuration.h"
#include "protocol.h"
#include "main.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile REALTIME_DATA_TYPE CURR_DATA;
extern volatile CONFIG_TYPE curr_config;

const char* lab_com_state = "通信状态:";
const char* lab_station = "观测站点:";
const char* lab_dev_state = "设备状态";

static char dev_state_buffer[64];       //设备状态缓冲区


//绘制标题栏的回调函数
static gboolean on_status_pane_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    guint width,height;

    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);

    cairo_pattern_t* top_patter = cairo_pattern_create_linear(height,0,height/2,height);
    cairo_pattern_add_color_stop_rgba(top_patter, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 0.7, APP_BKG_RGB, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 1, 0.7, 0.7, 0.7, 1);

    /**
    *   绘制通信状态边框
    **/
    {
        cairo_text_extents_t com_state_ext;

        cairo_set_source_rgb (cr, CS_BKG_GRB);
        draw_rectangle(cr,5,5,width-15,height/2-15);
        cairo_fill_preserve(cr);

        //通信状态标签
        {
            double state_font_size = MIN(width-15,height/2-15)/5;
            cairo_set_font_size(cr,state_font_size);
            cairo_set_source_rgb(cr,1,1,1);     //通信状态颜色
            cairo_text_extents(cr,lab_com_state,&com_state_ext);

            double startX = width/2 - com_state_ext.width*3/2;
            double startY = com_state_ext.height * 2 + 10;

            cairo_move_to(cr,startX,startY);
            cairo_show_text(cr,lab_com_state);

            startX += com_state_ext.width + state_font_size;
            cairo_move_to(cr,startX,startY);

            if(curr_config.mode == 1)       //串口模式
            {
                cairo_show_text(cr,"串口模式(正常)");
            }
            if(curr_config.mode == 2)       //蓝牙模式
            {
                cairo_show_text(cr,"蓝牙模式(正常)");
            }
            //--------------------------------------------

            startX = width/2 - com_state_ext.width*3/2;
            startY += com_state_ext.height * 2;
            cairo_move_to(cr,startX,startY);
            cairo_show_text(cr,lab_station);

            startX += com_state_ext.width + state_font_size;
            cairo_move_to(cr,startX,startY);

            if(curr_config.station_id == 0)
            {
                cairo_show_text(cr,"默认观测点");
            }else{
                cairo_show_text(cr,(char *)curr_config.station_name);
            }

            cairo_stroke_preserve(cr);
        }

        cairo_set_source(cr,top_patter);
        cairo_set_line_width (cr, 10.0);
        cairo_stroke (cr);
        cairo_stroke_preserve(cr);
    }
    /**
    *   绘制设备状态边框
    **/
    {
        cairo_text_extents_t dev_state_ext;

        cairo_set_source_rgb (cr, DS_BKG_GRB);
        draw_rectangle(cr,5,height/2+10,width-15,height/2-15);
        cairo_fill_preserve(cr);
        //设备状态标签
        {
            double state_font_size = MIN(width-15,height/2-15)/5;
            cairo_set_font_size(cr,state_font_size);
            cairo_set_source_rgb(cr,1,1,1);     //设备状态颜色
            cairo_text_extents(cr,lab_dev_state,&dev_state_ext);

            double startX = width/2 - dev_state_ext.width/2 - 5;
            double startY = height/2+10+state_font_size*2;

            cairo_move_to(cr,startX,startY);
            cairo_show_text(cr,lab_dev_state);
            cairo_stroke_preserve(cr);
        }
        //设备状态信息
        {
            double startX = 0,startY = 0;
            float curr_voltage = CURR_DATA.voltage;
            uint8_t curr_volprec = CURR_DATA.vol_precp;
            double state_font_size = MIN(width-15,height/2-15)/6;

            cairo_set_font_size(cr,state_font_size);
            cairo_set_source_rgb(cr,1,1,1);     //设备状态颜色

            memset(dev_state_buffer,0,64);
            sprintf(dev_state_buffer,"供电电压: %0.1fV",curr_voltage);
            cairo_text_extents(cr,dev_state_buffer,&dev_state_ext);

            startX = width/4 - dev_state_ext.width/2;
            startY = height/2+10 + state_font_size*4.5;
            cairo_move_to(cr,startX,startY);
            cairo_show_text(cr,dev_state_buffer);
            cairo_stroke_preserve(cr);
            //---------------------------------------------------------

            memset(dev_state_buffer,0,64);
            sprintf(dev_state_buffer,"电池电量: %d%%",curr_volprec);
            cairo_text_extents(cr,dev_state_buffer,&dev_state_ext);

            startX = width/2 + dev_state_ext.width/4;
            startY = height/2+10 + state_font_size*4.5;
            cairo_move_to(cr,startX,startY);
            cairo_show_text(cr,dev_state_buffer);
            cairo_stroke_preserve(cr);

        }

        cairo_set_source(cr,top_patter);
        cairo_set_line_width (cr, 10.0);
        cairo_stroke (cr);
        cairo_stroke_preserve(cr);
    }

    cairo_fill(cr);
    cairo_destroy(cr);

    return FALSE;
}

//创建状态面板
GtkDrawingArea* build_status_pane()
{
    GtkDrawingArea* status_pane = (GtkDrawingArea *)gtk_drawing_area_new();

    g_signal_connect(G_OBJECT(status_pane) , "draw" ,
              G_CALLBACK(on_status_pane_paint) , NULL);

    return status_pane;

}

