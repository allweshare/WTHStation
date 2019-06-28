#include "ComSetDialog.h"
#include "CustomDraw.h"
#include "ComService.h"
#include "BLEService.h"
#include "Configuration.h"
#include "protocol.h"
#include "main.h"
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t* root_dir_path;

static GtkWidget* dialog;
static const char* dialog_title = "通信设置";

static const char* baudrate_label[8] = {"2400","4800","9600","14400","19200","38400","57600","115200"};
static DWORD baudrate_value[8] = {BAUD_2400,BAUD_4800,BAUD_9600,BAUD_14400,BAUD_19200,BAUD_38400,BAUD_57600,BAUD_115200};

static GtkRadioButton* rdbComm = NULL;
static GtkRadioButton* rdbBLE = NULL;
static GtkComboBox* cmbComPort = NULL;
static GtkComboBox* cmbBaudrate = NULL;
static GtkComboBox* cmbDataBits = NULL;
static GtkComboBox* cmbCheckBits = NULL;
static GtkComboBox* cmbStopBits = NULL;

extern CONFIG_TYPE curr_config;
static uint8_t isChangeMode = 0;       //模式是否发生改变

static uint8_t* img_ble_path = NULL;

static gboolean on_title_bar_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    guint width,height;
    cairo_text_extents_t font_extent;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_source_rgb(cr,1,1,1);
    cairo_set_font_size(cr,30);
    cairo_text_extents(cr,dialog_title,&font_extent);

    cairo_move_to(cr,width/2-font_extent.width/2,height/2+font_extent.height/2);
    cairo_show_text(cr,dialog_title);

    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;
}

static gboolean dialog_key_press(GtkWidget* window,GdkEventKey* event,gpointer data)
{
    if(event->keyval == GDK_KEY_Escape)
    {
        gtk_window_close(GTK_WINDOW(dialog));
    }
    return FALSE;
}

void dialog_close()
{
    if(img_ble_path != NULL)
    {
        free(img_ble_path);
        img_ble_path = NULL;
    }
    gtk_window_close(GTK_WINDOW(dialog));
}

GtkComboBox* buildComPortList(int port_count)
{
    char itemBuff[16];
    int i = 0;

    GtkTreeIter listItert;
    GtkListStore* listComPort = gtk_list_store_new(1,G_TYPE_STRING);

    for(i=0;i<port_count;i++)
    {
        memset(itemBuff,0,16);
        sprintf((char *)itemBuff,"COM%d",(i+1));
        gtk_list_store_append(listComPort,&listItert);
        gtk_list_store_set(listComPort,&listItert,0,itemBuff,-1);
    }

    GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
    GtkComboBox* cmbCom = (GtkComboBox *)gtk_combo_box_new_with_model(GTK_TREE_MODEL(listComPort));

    //查找当前选定的串口，如果没有指定，默认选择第一项
    memset(itemBuff,0,16);
    strcpy((char *)itemBuff,(char *)&curr_config.com_port[3]);
    gtk_combo_box_set_active(cmbCom,atoi(itemBuff)-1);

    gtk_widget_set_size_request(GTK_WIDGET(cmbCom),80,30);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmbCom),listRender,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmbCom),listRender,"text",0,NULL);

    return cmbCom;
}

GtkComboBox* buildBaudrateList()
{
    int i = 0;
    GtkTreeIter listItert;
    GtkListStore* listComPort = gtk_list_store_new(1,G_TYPE_STRING);

    for(i=0;i<8;i++)
    {
        gtk_list_store_append(listComPort,&listItert);
        gtk_list_store_set(listComPort,&listItert,0,baudrate_label[i],-1);
    }

    GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
    GtkComboBox* cmboBox = (GtkComboBox *)gtk_combo_box_new_with_model(GTK_TREE_MODEL(listComPort));

    //Search Baudrate for cached
    for(i=0;i<8;i++)
    {
        if(curr_config.com_baudrate == baudrate_value[i])
        {
            gtk_combo_box_set_active(cmboBox,i);
        }
    }
    gtk_widget_set_size_request(GTK_WIDGET(cmboBox),80,30);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmboBox),listRender,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmboBox),listRender,"text",0,NULL);

    return cmboBox;
}

GtkComboBox* buildDataBitsList()
{
    GtkTreeIter listItert;
    GtkListStore* listComPort = gtk_list_store_new(1,G_TYPE_STRING);

    gtk_list_store_append(listComPort,&listItert);
    gtk_list_store_set(listComPort,&listItert,0,"5",-1);

    gtk_list_store_append(listComPort,&listItert);
    gtk_list_store_set(listComPort,&listItert,0,"6",-1);

    gtk_list_store_append(listComPort,&listItert);
    gtk_list_store_set(listComPort,&listItert,0,"7",-1);

    gtk_list_store_append(listComPort,&listItert);
    gtk_list_store_set(listComPort,&listItert,0,"8",-1);

    GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
    GtkComboBox* cmbDataBit = (GtkComboBox *)gtk_combo_box_new_with_model(GTK_TREE_MODEL(listComPort));

    //Load Data Bit
    if(curr_config.com_bytebits == DATABITS_5){
        gtk_combo_box_set_active(cmbDataBit,0);
    }else if(curr_config.com_bytebits == DATABITS_6){
        gtk_combo_box_set_active(cmbDataBit,1);
    }else if(curr_config.com_bytebits == DATABITS_7){
        gtk_combo_box_set_active(cmbDataBit,2);
    }else if(curr_config.com_bytebits == DATABITS_8){
        gtk_combo_box_set_active(cmbDataBit,3);
    }else{}

    gtk_widget_set_size_request(GTK_WIDGET(cmbDataBit),80,30);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmbDataBit),listRender,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmbDataBit),listRender,"text",0,NULL);

    return cmbDataBit;
}

GtkComboBox* buildCheckBitsList()
{
    GtkTreeIter listItert;
    GtkListStore* list = gtk_list_store_new(1,G_TYPE_STRING);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"无校验",-1);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"奇校验",-1);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"偶校验",-1);

    GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
    GtkComboBox* cmbCheckBit = (GtkComboBox *)gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
    //Load Parity Bits
    if(curr_config.com_checkBits == PARITY_NONE){
        gtk_combo_box_set_active(cmbCheckBit,0);
    }else if(curr_config.com_checkBits == PARITY_ODD){
        gtk_combo_box_set_active(cmbCheckBit,1);
    }else if(curr_config.com_checkBits == PARITY_EVEN){
        gtk_combo_box_set_active(cmbCheckBit,2);
    }else{}

    gtk_widget_set_size_request(GTK_WIDGET(cmbCheckBit),80,30);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmbCheckBit),listRender,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmbCheckBit),listRender,"text",0,NULL);

    return cmbCheckBit;
}

GtkComboBox* buildStopBitsList()
{
    GtkTreeIter listItert;
    GtkListStore* list = gtk_list_store_new(1,G_TYPE_STRING);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"1",-1);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"1.5",-1);

    gtk_list_store_append(list,&listItert);
    gtk_list_store_set(list,&listItert,0,"2",-1);

    GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
    GtkComboBox* cmbStopBit = (GtkComboBox *)gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
    //Load StopBits from Config-File
    if(curr_config.com_stopBits == STOPBITS_10){
        gtk_combo_box_set_active(cmbStopBit,0);
    }else if(curr_config.com_stopBits == STOPBITS_15){
        gtk_combo_box_set_active(cmbStopBit,1);
    }else if(curr_config.com_stopBits == STOPBITS_20){
        gtk_combo_box_set_active(cmbStopBit,2);
    }else{}

    gtk_widget_set_size_request(GTK_WIDGET(cmbStopBit),80,30);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cmbStopBit),listRender,TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmbStopBit),listRender,"text",0,NULL);

    return cmbStopBit;
}

static gboolean on_cancel_btn_click( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
    gtk_window_close(GTK_WINDOW(dialog));
    return FALSE;
}

static gboolean on_ensure_btn_click( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
    gint comPortIndex = gtk_combo_box_get_active(cmbComPort);
    memset(curr_config.com_port,0,8);
    sprintf((char *)curr_config.com_port,"COM%d",(comPortIndex+1));

    gint comBaudIndex = gtk_combo_box_get_active(cmbBaudrate);
    curr_config.com_baudrate = baudrate_value[comBaudIndex];

    gint comDataBitIndex = gtk_combo_box_get_active(cmbDataBits);
    if(comDataBitIndex == 0)
    {
        curr_config.com_bytebits = DATABITS_5;
    }else if(comDataBitIndex == 1){
        curr_config.com_bytebits = DATABITS_6;
    }else if(comDataBitIndex == 2){
        curr_config.com_bytebits = DATABITS_7;
    }else if(comDataBitIndex == 3){
        curr_config.com_bytebits = DATABITS_8;
    }else{
        curr_config.com_bytebits = DATABITS_8;
    }

    gint comCheckBitIndex = gtk_combo_box_get_active(cmbCheckBits);
    if(comCheckBitIndex == 0)
    {
        curr_config.com_checkBits = PARITY_NONE;
    }else if(comCheckBitIndex == 1){
        curr_config.com_checkBits = PARITY_ODD;
    }else if(comCheckBitIndex == 2){
        curr_config.com_checkBits = PARITY_EVEN;
    }else{
        curr_config.com_checkBits = PARITY_NONE;
    }

    gint comStopBitIndex = gtk_combo_box_get_active(cmbStopBits);
    if(comStopBitIndex == 0)
    {
        curr_config.com_stopBits = STOPBITS_10;
    }else if(comStopBitIndex == 1){
        curr_config.com_stopBits = STOPBITS_15;
    }else if(comStopBitIndex == 2){
        curr_config.com_stopBits = STOPBITS_20;
    }else{
        curr_config.com_stopBits = STOPBITS_10;
    }

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdbComm)))
    {
        if(curr_config.mode == 2)
        {
            curr_config.mode = 1;

            BLEServiceDestroy();

            isChangeMode = 1;

            //保存配置
            saveConfig();

            exit(0);        //改变通信模式后，必须重新启动应用程序
        }

        //关闭串口重新打开一次
        ComServiceDestroy();

        ComServiceInit();

    }
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdbBLE)))
    {
        if(curr_config.mode == 1)
        {
            curr_config.mode = 2;

            ComServiceDestroy();

            isChangeMode = 1;

            //保存配置
            saveConfig();

            exit(0);        //改变通信模式后，必须重新启动应用程序
        }

        BLEServiceDestroy();

        BLEServiceInit();
    }

    //保存配置
    saveConfig();
    //PRINT_TRACE_INFO("Save mode: %");

    gtk_window_close(GTK_WINDOW(dialog));

    return FALSE;
}

static gboolean on_ble_pane_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    cairo_pattern_t* top_patter = cairo_pattern_create_linear(0,COMSTAT_TITLE_HIGHT,COMSTAT_DLG_WIDTH/2-5,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
    cairo_pattern_add_color_stop_rgba(top_patter, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 0.7, APP_BKG_RGB, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 1, 0.7, 0.7, 0.7, 1);

    cairo_set_source_rgb(cr, 0.83, 0.83, 0.83);
    draw_rectangle(cr,COMSTAT_DLG_WIDTH/2+15,COMSTAT_TITLE_HIGHT,COMSTAT_DLG_WIDTH/2-30,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
    cairo_fill_preserve(cr);

    cairo_set_source(cr,top_patter);
    cairo_set_line_width (cr, 10.0);
    cairo_stroke (cr);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;
}

static gboolean on_comm_pane_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    cairo_pattern_t* top_patter = cairo_pattern_create_linear(0,COMSTAT_TITLE_HIGHT,COMSTAT_DLG_WIDTH/2-5,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
    cairo_pattern_add_color_stop_rgba(top_patter, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 0.7, APP_BKG_RGB, 1);
    cairo_pattern_add_color_stop_rgba(top_patter, 1, 0.7, 0.7, 0.7, 1);

    cairo_set_source_rgb(cr, 0.83, 0.83, 0.83);
    draw_rectangle(cr,15,COMSTAT_TITLE_HIGHT,COMSTAT_DLG_WIDTH/2-30,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
    cairo_fill_preserve(cr);

    cairo_set_source(cr,top_patter);
    cairo_set_line_width (cr, 10.0);
    cairo_stroke (cr);
    cairo_stroke_preserve(cr);

    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;
}


void show_comset_dialog()
{
    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_title(GTK_WINDOW(dialog),dialog_title);
    gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);

    g_signal_connect(G_OBJECT(dialog), "key-press-event",G_CALLBACK(dialog_key_press),NULL);

    GdkRGBA bkgClr = {APP_BKG_RGB, 1};
    gtk_widget_override_background_color(GTK_WIDGET(dialog),GTK_STATE_FLAG_NORMAL,&bkgClr);

    gtk_window_set_resizable(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
    //----------------------------------------

    GtkBox* dialogBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_VERTICAL,0);

    GtkDrawingArea* titleArea = (GtkDrawingArea *)gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(titleArea) , "draw" ,
              G_CALLBACK(on_title_bar_paint) , NULL);

    gtk_widget_set_size_request(GTK_WIDGET(titleArea),30,COMSTAT_TITLE_HIGHT);
    gtk_box_pack_start(dialogBox,GTK_WIDGET(titleArea),FALSE,FALSE,0);

    GtkBox* containerBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    GSList* rdbGroup = NULL;
    {
        //添加左边串口通信设置部分
        GtkFrame* paneComm = (GtkFrame *)gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(paneComm,GTK_SHADOW_NONE);
        g_signal_connect(G_OBJECT(paneComm) , "draw" ,
              G_CALLBACK(on_comm_pane_paint) , NULL);

        gtk_widget_set_size_request(GTK_WIDGET(paneComm),COMSTAT_DLG_WIDTH/2-10,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
        gtk_box_pack_start(containerBox,GTK_WIDGET(paneComm),TRUE,TRUE,0);

        GtkFixed* container = (GtkFixed *)gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(paneComm),GTK_WIDGET(container));

        rdbComm = (GtkRadioButton *)gtk_radio_button_new_with_label(NULL,"串口模式");
        rdbGroup = gtk_radio_button_get_group(rdbComm);
        PangoFontDescription* ctrlFont;
        ctrlFont = pango_font_description_from_string("SIMHEI 16");
        gtk_widget_override_font(GTK_WIDGET(rdbComm),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(rdbComm),30,30);
        gtk_fixed_put(container,GTK_WIDGET(rdbComm),60,20);

        gint startX = 40;
        gint startY = 80;
        gint offsetX = 80;
        gint offsetY = 50;

        ctrlFont = pango_font_description_from_string("SIMHEI 13");

        GtkLabel* labComPort = (GtkLabel *)gtk_label_new("通信口:");
        gtk_widget_override_font(GTK_WIDGET(labComPort),ctrlFont);
        gtk_fixed_put(container,GTK_WIDGET(labComPort),startX,startY);

        cmbComPort = buildComPortList(10);
        gtk_fixed_put(container,GTK_WIDGET(cmbComPort),startX+offsetX,startY-5);

        GtkLabel* labBaudRate = (GtkLabel *)gtk_label_new("波特率:");
        gtk_widget_override_font(GTK_WIDGET(labBaudRate),ctrlFont);
        gtk_fixed_put(container,GTK_WIDGET(labBaudRate),startX,startY+offsetY);

        cmbBaudrate = buildBaudrateList();
        gtk_fixed_put(container,GTK_WIDGET(cmbBaudrate),startX+offsetX,startY+offsetY-5);

        GtkLabel* labDataBit = (GtkLabel *)gtk_label_new("数据位:");
        gtk_widget_override_font(GTK_WIDGET(labDataBit),ctrlFont);
        gtk_fixed_put(container,GTK_WIDGET(labDataBit),startX,startY+offsetY*2);

        cmbDataBits = buildDataBitsList();
        gtk_fixed_put(container,GTK_WIDGET(cmbDataBits),startX+offsetX,startY+offsetY*2-5);

        GtkLabel* labCheckBit = (GtkLabel *)gtk_label_new("校验位:");
        gtk_widget_override_font(GTK_WIDGET(labCheckBit),ctrlFont);
        gtk_fixed_put(container,GTK_WIDGET(labCheckBit),startX,startY+offsetY*3);

        cmbCheckBits = buildCheckBitsList();
        gtk_fixed_put(container,GTK_WIDGET(cmbCheckBits),startX+offsetX,startY+offsetY*3-5);

        GtkLabel* labStopBit = (GtkLabel *)gtk_label_new("停止位:");
        gtk_widget_override_font(GTK_WIDGET(labStopBit),ctrlFont);
        gtk_fixed_put(container,GTK_WIDGET(labStopBit),startX,startY+offsetY*4);

        cmbStopBits = buildStopBitsList();
        gtk_fixed_put(container,GTK_WIDGET(cmbStopBits),startX+offsetX,startY+offsetY*4-5);

        //--------------------------------
        //Check Mode in Config
        if(curr_config.mode == 1)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rdbComm),TRUE);
        }
    }
    {
        //添加右边蓝牙通信设置部分
        GtkFrame* paneBLE = (GtkFrame *)gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(paneBLE,GTK_SHADOW_NONE);
        g_signal_connect(G_OBJECT(paneBLE) , "draw" ,
              G_CALLBACK(on_ble_pane_paint) , NULL);

        gtk_widget_set_size_request(GTK_WIDGET(paneBLE),COMSTAT_DLG_WIDTH/2-10,COMSTAT_DLG_HEIGHT-COMSTAT_TITLE_HIGHT-80);
        gtk_box_pack_start(containerBox,GTK_WIDGET(paneBLE),TRUE,TRUE,0);

        GtkFixed* container = (GtkFixed *)gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(paneBLE),GTK_WIDGET(container));

        rdbBLE = (GtkRadioButton *)gtk_radio_button_new_with_label(rdbGroup,"蓝牙模式");
        PangoFontDescription* ctrlFont;
        ctrlFont = pango_font_description_from_string("SIMHEI 16");
        gtk_widget_override_font(GTK_WIDGET(rdbBLE),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(rdbBLE),30,30);
        gtk_fixed_put(container,GTK_WIDGET(rdbBLE),60,20);

        img_ble_path = malloc(1024);
        memset(img_ble_path,0,1024);
        sprintf((char *)img_ble_path,"%s//IMG//bleMgr.png",root_dir_path);

        GtkImage* imgBleMgr = (GtkImage *)gtk_image_new_from_file((char *)img_ble_path);
        gtk_fixed_put(container,GTK_WIDGET(imgBleMgr),20,60);
        //--------------------------------
        //Check Mode in Config
        if(curr_config.mode == 2)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rdbBLE),TRUE);
        }
    }

    gtk_box_pack_start(dialogBox,GTK_WIDGET(containerBox),TRUE,TRUE,0);

    GtkBox* bottomBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_widget_set_size_request(GTK_WIDGET(bottomBox),30,60);

    PangoFontDescription* btnFont;
    btnFont = pango_font_description_from_string("SIMHEI 16");

    GtkButton* btn_ensure = (GtkButton *)gtk_button_new_with_label("应用");
    gtk_widget_override_font(GTK_WIDGET(btn_ensure),btnFont);
    gtk_box_pack_start(bottomBox,GTK_WIDGET(btn_ensure),TRUE,TRUE,30);
    g_signal_connect(G_OBJECT(btn_ensure) , "clicked" ,
              G_CALLBACK(on_ensure_btn_click) , NULL);

    GtkButton* btn_cancel = (GtkButton *)gtk_button_new_with_label("取消");
    gtk_widget_override_font(GTK_WIDGET(btn_cancel),btnFont);
    gtk_box_pack_end(bottomBox,GTK_WIDGET(btn_cancel),TRUE,TRUE,30);
    g_signal_connect(G_OBJECT(btn_cancel) , "clicked" ,
              G_CALLBACK(on_cancel_btn_click) , NULL);

    gtk_box_pack_end(dialogBox,GTK_WIDGET(bottomBox),FALSE,FALSE,30);

    gtk_container_add(GTK_CONTAINER(dialog),GTK_WIDGET(dialogBox));
    //----------------------------------------
    //设置禁用蓝牙功能
    gtk_widget_set_sensitive(GTK_WIDGET(rdbBLE),FALSE);

    gtk_widget_set_size_request(GTK_WIDGET(dialog),COMSTAT_DLG_WIDTH,COMSTAT_DLG_HEIGHT);
    gtk_widget_set_opacity(GTK_WIDGET(dialog),0.95);
    gtk_widget_show_all(dialog);

}

