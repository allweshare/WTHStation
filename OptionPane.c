#include "OptionPane.h"
#include "CustomDraw.h"
#include "ComSetDialog.h"
#include "MainFrame.h"
#include "StatSetDialog.h"
#include "DataListDialog.h"
#include "main.h"

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gtk/gtkbbox.h>
#include <glib/gmacros.h>

extern GtkWidget* framewindow;
extern GtkDrawingArea* option_pane;

static uint8_t time_info[128];

CUSTOM_BUTTON* btn_comset;
CUSTOM_BUTTON* btn_statset;
CUSTOM_BUTTON* btn_dataprv;
CUSTOM_BUTTON* btn_exit;

gint option_pane_timer;              //局部刷新的Timer

void draw_option_btn(cairo_t* cr,CUSTOM_BUTTON* btn,double width,double height);

#define CHECK_BUTTON_CLICK(btn)        \
        {       \
            double startX = 0,startY = 0;       \
            cairo_t* cr;        \
            cairo_text_extents_t btn_lab_ext;       \
            cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(option_pane)));      \
            cairo_set_source_rgba(cr,0.26,0.42,0.7,1);      \
            cairo_set_font_size(cr,MIN(width,height)/12);       \
            draw_rectangle(cr,btn->rect.x,btn->rect.y,btn->rect.width,btn->rect.height);        \
            cairo_fill_preserve(cr);        \
            cairo_select_font_face(cr,"SIMHEI",     \
                               CAIRO_FONT_SLANT_NORMAL,     \
                               CAIRO_FONT_WEIGHT_BOLD);     \
            cairo_set_source_rgba(cr,0,0,1,1);      \
            cairo_text_extents(cr,btn->label,&btn_lab_ext);      \
            startX = btn->rect.x+(btn->rect.width/2)-btn_lab_ext.width/2;     \
            startY = btn->rect.y + btn_lab_ext.height;   \
            cairo_move_to(cr,startX,startY);        \
            cairo_show_text(cr,btn->label);  \
            cairo_set_source_rgba(cr,0,1,0,1);      \
            cairo_set_line_width(cr, 2.0);      \
            cairo_stroke(cr);       \
            cairo_stroke_preserve(cr);      \
            cairo_fill(cr);     \
            cairo_destroy(cr);      \
        }

#define CHECK_MOUSE_ENTER(x,y,btn)      \
    if(x > btn->rect.x && x < (btn->rect.x + btn->rect.width)        \
       && y > btn->rect.y && y < (btn->rect.y + btn->rect.height))   \
    {   \
        btn->isMoveIn = 1;      \
        cairo_t* cr;        \
        cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(option_pane)));      \
        cairo_set_source_rgba(cr,0,0,0,0);      \
        cairo_set_font_size(cr,MIN(width,height)/12);   \
        draw_rectangle(cr,btn->rect.x,btn->rect.y,btn->rect.width,btn->rect.height);    \
        cairo_set_source_rgba(cr,0,1,0,1);      \
        cairo_set_line_width(cr, 2.0);      \
        cairo_stroke(cr);       \
        cairo_stroke_preserve(cr);      \
        cairo_fill(cr);     \
        cairo_destroy(cr);      \
    }else{              \
        cairo_t* cr;            \
        cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(option_pane)));      \
        cairo_set_source_rgba(cr,0,0,0,0);      \
        cairo_set_font_size(cr,MIN(width,height)/12);       \
        draw_rectangle(cr,btn->rect.x,btn->rect.y,btn->rect.width,btn->rect.height);    \
        cairo_set_source_rgba(cr,0,0,1,1);  \
        cairo_set_line_width(cr, 2.0);      \
        cairo_stroke(cr);       \
        cairo_stroke_preserve(cr);      \
        cairo_fill(cr);     \
        cairo_destroy(cr);      \
        btn->isMoveIn = 0;      \
    }

void option_pane_move(double x,double y)
{
    guint width,height;
    width = gtk_widget_get_allocated_width(GTK_WIDGET(option_pane));
    height = gtk_widget_get_allocated_height(GTK_WIDGET(option_pane));

    CHECK_MOUSE_ENTER(x,y,btn_comset);
    CHECK_MOUSE_ENTER(x,y,btn_statset);
    CHECK_MOUSE_ENTER(x,y,btn_dataprv);
    CHECK_MOUSE_ENTER(x,y,btn_exit);
}

void option_pane_click(double x,double y)
{
    if(btn_comset->isMoveIn == 1){
        btn_comset->click(x,y);
        //CHECK_BUTTON_CLICK(btn_comset);
    }else if(btn_statset->isMoveIn == 1){
        btn_statset->click(x,y);
        //CHECK_BUTTON_CLICK(btn_statset);
    }else if(btn_dataprv->isMoveIn == 1){
        btn_dataprv->click(x,y);
        //CHECK_BUTTON_CLICK(btn_dataprv);
    }else if(btn_exit->isMoveIn == 1){
        btn_exit->click(x,y);
    }else{
    }
}

void draw_option_btn(cairo_t* cr,CUSTOM_BUTTON* btn,double width,double height)
{
    double startX = 0,startY = 0;
    cairo_text_extents_t btn_lab_ext;

    //选择按钮颜色
    cairo_set_source_rgba(cr,BTN_BKG_GRB,1);
    draw_rectangle(cr,btn->rect.x,btn->rect.y,btn->rect.width,btn->rect.height);
    cairo_fill_preserve(cr);
    //绘制按钮内容
    cairo_set_source_rgba(cr,0,0,1,1);
    cairo_set_font_size(cr,MIN(width,height)/12);
    cairo_text_extents(cr,btn->label,&btn_lab_ext);
    startX = btn->rect.x+(btn->rect.width/2)-btn_lab_ext.width/2;
    startY = btn->rect.y + btn_lab_ext.height;
    cairo_move_to(cr,startX,startY);
    cairo_show_text(cr,btn->label);
    //绘制按钮边框
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);
    cairo_stroke_preserve(cr);
}

//刷新当前时间
void refresh_datetime(cairo_t* cr,double x,double y,double width,double height)
{
    cairo_text_extents_t time_info_ext;
    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);

    //当前时间
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_font_size(cr,MIN(width,height)/12);
    cairo_move_to(cr,30,(MIN(width,height)/12*9));
    cairo_show_text(cr,"当前时间:");

    cairo_set_font_size(cr,MIN(width,height)/12);
    cairo_text_extents(cr,(char *)time_info,&time_info_ext);
    cairo_move_to(cr,width/2-time_info_ext.width/2,(MIN(width,height)/12*10.7));
    cairo_show_text(cr,(char *)time_info);
    cairo_stroke_preserve(cr);
}

//绘制功能选项的回调函数
static gboolean on_option_pane_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    guint width,height;

    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_move_to(cr,30,35);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr,MIN(width,height)/12);
    cairo_show_text(cr,"功能选择:");

    double btn_width = width/2-40;
    double btn_height = (MIN(width,height)/10);

    {
        //通信设置按钮
        btn_comset->rect.x = 30;
        btn_comset->rect.y = 30+(MIN(width,height)/12);
        btn_comset->rect.width = btn_width;
        btn_comset->rect.height = btn_height;
        draw_option_btn(cr,btn_comset,width,height);

        //测站设置按钮
        btn_statset->rect.x = width/2+10;
        btn_statset->rect.y = 30+(MIN(width,height)/12);
        btn_statset->rect.width = btn_width;
        btn_statset->rect.height = btn_height;
        draw_option_btn(cr,btn_statset,width,height);

        //数据浏览按钮
        btn_dataprv->rect.x = 30;
        btn_dataprv->rect.y = 30+(MIN(width,height)/6)+(MIN(width,height)/12);
        btn_dataprv->rect.width = btn_width;
        btn_dataprv->rect.height = btn_height;
        draw_option_btn(cr,btn_dataprv,width,height);

        //退出
        btn_exit->rect.x = width/2+10;
        btn_exit->rect.y = 30+(MIN(width,height)/6)+(MIN(width,height)/12);
        btn_exit->rect.width = btn_width;
        btn_exit->rect.height = btn_height;
        draw_option_btn(cr,btn_exit,width,height);
    }

    //刷新显示当前时间
    refresh_datetime(cr,30,(MIN(width,height)/12*9),width,height);

    cairo_fill(cr);
    cairo_destroy(cr);

    return FALSE;
}


//---------------------------------------------------
/**
*   为每个按钮绑定事件
**/
void on_comset_btn_click(double x,double y)
{
    show_comset_dialog();
}
void on_statset_btn_click(double x,double y)
{
    show_station_set_dialog();
}
void on_dataprv_btn_click(double x,double y)
{
    show_data_list_dialog();
}
void on_exit_btn_click(double x,double y)
{
    gtk_window_close(GTK_WINDOW(framewindow));
}
//---------------------------------------------------

gint option_pane_timer_proc(gpointer data)
{
    time_t now;
    struct tm *ltime;
    now = time((time_t *)NULL);
    ltime = localtime(&now);

    memset(time_info,0,128);
    sprintf((char *)time_info,"%04d年",(1900+ltime->tm_year));
    sprintf((char *)time_info,"%s%02d月",time_info,ltime->tm_mon+1);
    sprintf((char *)time_info,"%s%02d日 ",time_info,ltime->tm_mday);
    sprintf((char *)time_info,"%s%02d时",time_info,ltime->tm_hour);
    sprintf((char *)time_info,"%s%02d分",time_info,ltime->tm_min);
    sprintf((char *)time_info,"%s%02d秒",time_info,ltime->tm_sec);

    //设置刷新指定区域
    guint width,height;
    width = gtk_widget_get_allocated_width(GTK_WIDGET(option_pane));
    height = gtk_widget_get_allocated_height(GTK_WIDGET(option_pane));
    gtk_widget_queue_draw_area(GTK_WIDGET(option_pane),0,height/2,width,height/2);

    return 1;
}

//创建状态面板
GtkDrawingArea* build_option_pane()
{
    btn_comset = (CUSTOM_BUTTON *)malloc(sizeof(CUSTOM_BUTTON));
    btn_statset = (CUSTOM_BUTTON *)malloc(sizeof(CUSTOM_BUTTON));
    btn_dataprv = (CUSTOM_BUTTON *)malloc(sizeof(CUSTOM_BUTTON));
    btn_exit = (CUSTOM_BUTTON *)malloc(sizeof(CUSTOM_BUTTON));

    memset((void *)btn_comset->label,0,32);
    strcpy((char *)btn_comset->label,"通信设置(1键)");
    btn_comset->click = on_comset_btn_click;

    memset((void *)btn_statset->label,0,32);
    strcpy((char *)btn_statset->label,"测站设置(2键)");
    btn_statset->click = on_statset_btn_click;

    memset((void *)btn_dataprv->label,0,32);
    strcpy((char *)btn_dataprv->label,"数据浏览(3键)");
    btn_dataprv->click = on_dataprv_btn_click;

    memset((void *)btn_exit->label,0,32);
    strcpy((char *)btn_exit->label,"退出(Fn+ESC)");
    btn_exit->click = on_exit_btn_click;

    memset(time_info,0,128);
    strcpy((char *)time_info,"2000年00月00日 00时00分00秒");

    GtkDrawingArea* option_pane = (GtkDrawingArea *)gtk_drawing_area_new();

    g_signal_connect(G_OBJECT(option_pane) , "draw" ,
              G_CALLBACK(on_option_pane_paint) , NULL);

    option_pane_timer = g_timeout_add(INTERVEL_SELF_REFRESH,option_pane_timer_proc,NULL);

    return option_pane;
}


void option_pane_release()
{
    free(btn_comset);
    free(btn_statset);
    free(btn_dataprv);
    free(btn_exit);
}


