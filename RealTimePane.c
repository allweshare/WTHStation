#include "RealTimePane.h"
#include "CustomDraw.h"
#include "protocol.h"
#include "main.h"
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern volatile REALTIME_DATA_TYPE CURR_DATA;

const double contain_start_X = 30;      //起始坐标
const double contain_start_Y = 30;

static char disp_field_buffer[64];      //显示缓冲区


const char* lab_wind_3s = "三秒瞬时风";
const char* lab_wind_1m = "一分钟平均风";
const char* lab_wind_10m = "十分钟平均风";


/**
*   显示基本的实时数据
*   如: 温度，湿度，气压，雨量
*   param: width,height为实际显示区域的大小
**/
void refresh_realtime_info(cairo_t* cr,guint width,guint height)
{
    if(cr != NULL)
    {
        if((width > 100) && (height > 100))
        {
            cairo_text_extents_t font_extent;
            double fntSize = MIN(width,height)/18;

            cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_source_rgb(cr,1,1,1);

            cairo_set_font_size(cr,fntSize);

            {
                //温度
                float currTemp = CURR_DATA.tempt;
                memset(disp_field_buffer,0,64);
                sprintf((char *)disp_field_buffer,"温度: %0.1f℃",currTemp);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                cairo_move_to(cr,contain_start_X,contain_start_Y+font_extent.height);
                cairo_show_text(cr,disp_field_buffer);
            }

            {
                //湿度
                float currHumid = CURR_DATA.humid;
                memset(disp_field_buffer,0,64);
                sprintf((char *)disp_field_buffer,"湿度: %0.1f%%RH",currHumid);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                cairo_move_to(cr,width/2+contain_start_X,contain_start_Y+font_extent.height);
                cairo_show_text(cr,disp_field_buffer);
            }

            {
                //气压
                float currPress = CURR_DATA.press;
                memset(disp_field_buffer,0,64);
                sprintf((char *)disp_field_buffer,"气压: %0.1fhPa",currPress);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                double offsetY = (contain_start_Y+font_extent.height) + font_extent.height*2;
                cairo_move_to(cr,contain_start_X,offsetY);
                cairo_show_text(cr,disp_field_buffer);
            }

            {
                //雨量
                float currRain = CURR_DATA.rain;
                memset(disp_field_buffer,0,64);
                sprintf((char *)disp_field_buffer,"雨量: %0.1fmm",currRain);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                double offsetY = (contain_start_Y+font_extent.height) + font_extent.height*2 + (MIN(width,height)/120);
                cairo_move_to(cr,width/2+contain_start_X,offsetY);
                cairo_show_text(cr,disp_field_buffer);
            }

            //保存视图
            cairo_fill_preserve(cr);

            //--------------------------------------------------------------
            /**
            *   绘制风显示面板
            **/
            cairo_set_source_rgb (cr, APP_BKG_RGB);
            {
                //三秒瞬时风
                double offsetY = (contain_start_Y+height/5);
                draw_rectangle(cr,contain_start_X+5,offsetY,width-contain_start_X-10,height/5);
                cairo_fill_preserve(cr);
            }

            {
                //一分钟风
                double offsetY = (contain_start_Y+height/5*2) + height/24;
                draw_rectangle(cr,contain_start_X+5,offsetY,width-contain_start_X-10,height/5);
                cairo_fill_preserve(cr);
            }

            {
                //十分钟风
                double offsetY = (contain_start_Y+height/5*3) + height/24*2;
                draw_rectangle(cr,contain_start_X+5,offsetY,width-contain_start_X-10,height/5);
                cairo_fill_preserve(cr);
            }

            cairo_pattern_t* top_patter = cairo_pattern_create_linear(contain_start_X+5,contain_start_X+5,width,height);
            cairo_pattern_add_color_stop_rgba(top_patter, 0, 1, 1, 1, 1);
            cairo_pattern_add_color_stop_rgba(top_patter, 0.7, APP_BKG_RGB, 1);
            cairo_pattern_add_color_stop_rgba(top_patter, 1, 0.7, 0.7, 0.7, 1);

            cairo_set_source(cr,top_patter);
            cairo_set_line_width (cr, 10.0);
            cairo_stroke(cr);
            cairo_stroke_preserve(cr);

            /**
            *   绘制风数据
            **/
            {
                double wind_font_size = MIN(width-contain_start_X-10,height/5)/5;

                //三秒风
                cairo_set_source_rgb(cr,1,1,1);
                cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr,wind_font_size);

                memset(disp_field_buffer,0,64);
                strcpy((char *)disp_field_buffer,lab_wind_3s);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                double offsetX = width/2 - (font_extent.width/2);
                double offsetY = (contain_start_Y+height/5)+font_extent.height*2;
                cairo_move_to(cr,offsetX,offsetY);
                cairo_show_text(cr,lab_wind_3s);

                cairo_stroke_preserve(cr);

                {
                    //瞬时风向,风速
                    uint16_t curr_wind_dir = CURR_DATA.wind_dir_3s;
                    float curr_wind_speed = CURR_DATA.wind_speed_3s;

                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风向: %d°",curr_wind_dir);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = contain_start_X/2 + width/4 - font_extent.width/2;
                    offsetY += font_extent.height*2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);
                    //-----------------------------------------------------
                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风速: %0.1fm/s",curr_wind_speed);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = width/2 + contain_start_X/2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);

                    cairo_stroke_preserve(cr);
                }

                //double offsetY = (contain_start_Y+height/5*2) + height/24;

                //显示一分钟风标签
                memset(disp_field_buffer,0,64);
                strcpy((char *)disp_field_buffer,lab_wind_1m);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                offsetX = width/2 - (font_extent.width/2);
                //offsetY = (contain_start_Y+height/5)+font_extent.height*9;
                offsetY = (contain_start_Y+height/5) + height/24*8;
                cairo_move_to(cr,offsetX,offsetY);
                cairo_show_text(cr,lab_wind_1m);

                {
                    //一分钟平均风
                    uint16_t curr_wind_dir = CURR_DATA.wind_dir_1m;
                    float curr_wind_speed = CURR_DATA.wind_speed_1m;

                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风向: %d°",curr_wind_dir);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = contain_start_X/2 + width/4 - font_extent.width/2;
                    offsetY += font_extent.height*2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);
                    //-----------------------------------------------------
                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风速: %0.1fm/s",curr_wind_speed);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = width/2 + contain_start_X/2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);

                    cairo_stroke_preserve(cr);
                }

                //显示十分钟风标签
                memset(disp_field_buffer,0,64);
                strcpy((char *)disp_field_buffer,lab_wind_10m);
                cairo_text_extents(cr,disp_field_buffer,&font_extent);

                offsetX = width/2 - (font_extent.width/2);
                offsetY = (contain_start_Y+height/5*3) + height/24*4;
                cairo_move_to(cr,offsetX,offsetY);
                cairo_show_text(cr,lab_wind_10m);

                {
                    //十分钟平均风
                    uint16_t curr_wind_dir = CURR_DATA.wind_dir_10m;
                    float curr_wind_speed = CURR_DATA.wind_speed_10m;

                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风向: %d°",curr_wind_dir);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = contain_start_X/2 + width/4 - font_extent.width/2;
                    offsetY += font_extent.height*2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);
                    //-----------------------------------------------------
                    memset(disp_field_buffer,0,64);
                    sprintf((char *)disp_field_buffer,"风速: %0.1fm/s",curr_wind_speed);
                    cairo_text_extents(cr,disp_field_buffer,&font_extent);

                    offsetX = width/2 + contain_start_X/2;
                    cairo_move_to(cr,offsetX,offsetY);
                    cairo_show_text(cr,disp_field_buffer);

                    cairo_stroke_preserve(cr);
                }
            }

            //刷新画布
            cairo_fill(cr);

        }
    }
}

//绘制标题栏的回调函数
static gboolean on_realtime_pane_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    guint width,height;

    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_pattern_t* top_patter = cairo_pattern_create_linear(0,0,0,height);
    cairo_pattern_add_color_stop_rgba (top_patter, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba (top_patter, 0.5, APP_BKG_RGB, 1);
    cairo_pattern_add_color_stop_rgba (top_patter, 1, 0.7, 0.7, 0.7, 1);

    /**
    *   绘制边框
    **/
    {
        cairo_set_source_rgb (cr, RT_BKG_GRB);
        draw_rectangle(cr,10,5,width-20,height-15);
        cairo_fill_preserve(cr);

        cairo_set_source(cr,top_patter);
        cairo_set_line_width (cr, 10.0);
        cairo_stroke (cr);
        cairo_fill_preserve(cr);
    }

    //printf("Draw !\r\n");

    //刷新实时数据
    refresh_realtime_info(cr,width-30,height-20);



    cairo_fill(cr);
    cairo_destroy(cr);

    return FALSE;
}

//构建实时数据显示面板
GtkDrawingArea* build_realtime_pane()
{
    GtkDrawingArea* container = (GtkDrawingArea *)gtk_drawing_area_new();

    g_signal_connect(G_OBJECT(container) , "draw" ,
              G_CALLBACK(on_realtime_pane_paint) , NULL);

    return container;
}

