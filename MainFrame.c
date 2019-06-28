#include "MainFrame.h"
#include "TitleBar.h"
#include "RealTimePane.h"
#include "StatusPane.h"
#include "OptionPane.h"
#include "protocol.h"
#include "DataBase.h"

#include "main.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GtkWidget* framewindow;
GtkDrawingArea* realtime_pane;
GtkDrawingArea* status_pane;
GtkDrawingArea* option_pane;

extern CUSTOM_BUTTON* btn_comset;
extern CUSTOM_BUTTON* btn_statset;
extern CUSTOM_BUTTON* btn_dataprv;
extern CUSTOM_BUTTON* btn_exit;

extern volatile REALTIME_DATA_TYPE CURR_DATA;

gint main_frame_timer;              //局部刷新的Timer
gint sampling_timer;                //采样计时器,实现3秒一次采样

static gint main_frame_timer_proc(gpointer data)
{
    //realtime pane refresh
    {
        guint width,height;
        width = gtk_widget_get_allocated_width(GTK_WIDGET(realtime_pane));
        height = gtk_widget_get_allocated_height(GTK_WIDGET(realtime_pane));
        gtk_widget_queue_draw_area(GTK_WIDGET(realtime_pane),0,0,width,height);
    }
    //status pane refresh
    {
        guint width,height;
        width = gtk_widget_get_allocated_width(GTK_WIDGET(status_pane));
        height = gtk_widget_get_allocated_height(GTK_WIDGET(status_pane));
        gtk_widget_queue_draw_area(GTK_WIDGET(status_pane),0,0,width,height);
    }
    return 1;
}

static gint sampling_timer_proc(gpointer data)
{
    char timeStr[32];
    time_t times;
    struct tm* p_time;

    //PRINT_TRACE_INFO("Timer ... ");
    //分析并统计当前的采集数据
    protocol_analysis();

    time(&times);
    p_time = localtime(&times);
    memset(timeStr,0,32);
    int second = p_time->tm_sec/3*3;
    sprintf(timeStr,"%04d-%02d-%02d %02d:%02d:%02d",
            p_time->tm_year+1900,p_time->tm_mon+1,p_time->tm_mday,
            p_time->tm_hour,p_time->tm_min,second);
    //将当前实时数据加入数据库
    database_add_record(timeStr,0,(REALTIME_DATA_TYPE *)&CURR_DATA);
    return 1;
}

static gboolean frame_mouse_move(GtkWidget* window,GdkEventMotion* event,gpointer data)
{
    guint width,height;
    width = gtk_widget_get_allocated_width(window);
    height = gtk_widget_get_allocated_height(window);

    //路由鼠标的移动事件
    double startX = (event->x)-width/2;
    double startY = (event->y)-APP_TITLE_HEIGHT*3 - (height-APP_TITLE_HEIGHT*3)/2;

    if(startX > 0 && startY > 0)
    {
        option_pane_move(startX,startY);
    }
    return FALSE;
}

static gboolean frame_mouse_press(GtkWidget* window,GdkEventMotion* event,gpointer data)
{
    guint width,height;
    width = gtk_widget_get_allocated_width(window);
    height = gtk_widget_get_allocated_height(window);

    //路由鼠标的移动事件
    double startX = (event->x)-width/2;
    double startY = (event->y)-APP_TITLE_HEIGHT*3 - (height-APP_TITLE_HEIGHT*3)/2;

    if(startX > 0 && startY > 0)
    {
        option_pane_click(startX,startY);
    }

    return FALSE;
}

static gboolean frame_key_press(GtkWidget* window,GdkEventKey* event,gpointer data)
{
    if((event->keyval == GDK_KEY_1) || (event->keyval == GDK_KEY_KP_1))
    {
        btn_comset->click(0,0);
    }
    if((event->keyval == GDK_KEY_2) || (event->keyval == GDK_KEY_KP_2))
    {
        btn_statset->click(0,0);
    }
    if((event->keyval == GDK_KEY_3) || (event->keyval == GDK_KEY_KP_3))
    {
        btn_dataprv->click(0,0);
    }
    if(event->keyval == GDK_KEY_Escape)
    {
        btn_exit->click(0,0);
    }
    return FALSE;
}

void frame_window_close()
{
    printf("Close Frame Window !\r\n");
    app_exit();
    gtk_main_quit();
    //exit(0);
}

void init_layout(int argc, char* argv[])
{
    //初始化GTK+程序
    gtk_init(&argc, &argv);
    //创建窗口，并为窗口的关闭信号加回调函数以便退出
    framewindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(framewindow),"delete_event",G_CALLBACK(frame_window_close),NULL);
    //------------------------------------------
    //检测按钮单击事件
    gtk_widget_set_events(framewindow, GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(framewindow), "motion-notify-event",G_CALLBACK(frame_mouse_move), NULL);
    g_signal_connect(G_OBJECT(framewindow), "button_press_event",G_CALLBACK(frame_mouse_press),NULL);
    //------------------------------------------
    //绑定快捷键
    g_signal_connect(G_OBJECT(framewindow), "key-press-event",G_CALLBACK(frame_key_press),NULL);

    //------------------------------------------

    gtk_window_set_title((GtkWindow *)framewindow,APP_TITLE_NAME);
    gtk_window_set_default_size((GtkWindow *)framewindow,APP_DEFAULT_WIDTH,APP_DEFAULT_HEIGHT);

    //默认背景色
    GdkRGBA clr_rgba;
    gdk_rgba_parse(&clr_rgba,"#2F4F4F");
    gtk_widget_override_background_color(GTK_WIDGET(framewindow),GTK_STATE_NORMAL,&clr_rgba);

    GtkBox* frameBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
    {
        GtkDrawingArea* titleArea = build_title_bar();
        gtk_box_pack_start(frameBox,GTK_WIDGET(titleArea),FALSE,FALSE,APP_TITLE_HEIGHT);
    }
    {
        GtkBox* containerBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);

        realtime_pane = build_realtime_pane();
        gtk_box_pack_start(containerBox,GTK_WIDGET(realtime_pane),TRUE,TRUE,0);

        {
            GtkBox* optionBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_VERTICAL,10);

            status_pane = build_status_pane();
            gtk_box_pack_start(optionBox,GTK_WIDGET(status_pane),TRUE,TRUE,0);

            option_pane = build_option_pane();
            gtk_box_pack_end(optionBox,GTK_WIDGET(option_pane),TRUE,TRUE,0);

            gtk_box_pack_end(containerBox,GTK_WIDGET(optionBox),TRUE,TRUE,0);
        }

        gtk_box_pack_end(frameBox,GTK_WIDGET(containerBox),TRUE,TRUE,0);
    }
    gtk_container_add(GTK_CONTAINER(framewindow),GTK_WIDGET(frameBox));

    //Create Self-Refresh Timer
    main_frame_timer = g_timeout_add(INTERVEL_SELF_REFRESH,main_frame_timer_proc,NULL);
    //Create Sampling-Intervel Timer
    sampling_timer = g_timeout_add(SAMPLING_INTERVEL,sampling_timer_proc,NULL);

    gtk_window_set_position(GTK_WINDOW(framewindow),GTK_WIN_POS_CENTER);

    gtk_window_fullscreen(GTK_WINDOW(framewindow));

    gtk_widget_show_all(framewindow);
    gtk_main();
}

