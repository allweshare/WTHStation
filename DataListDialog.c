#include "DataListDialog.h"
#include "Configuration.h"
#include "CustomDraw.h"
#include "DataBase.h"
#include "main.h"
#include <gtk/gtk.h>
#include <math.h>

extern volatile CONFIG_TYPE curr_config;
extern uint8_t* root_dir_path;

static GtkWidget* dialog;
static const char* dialog_title = "数据浏览";
static GtkComboBoxText* cmb_station_select = NULL;  //选择要查询的观测点
static GtkDrawingArea* tableArea = NULL;            //查询并刷新数据列表
static GtkCalendar* cal_select_date = NULL;         //弹出的日期选择控件
static GtkButton* btn_select_date = NULL;           //显示选择的日期
static GtkLabel* lab_page_curr = NULL;              //显示总页数的标签
static GtkComboBoxText* cmb_page_select = NULL;     //选择页码的下拉框
static GtkButton* btn_export = NULL;                //导出数据的按钮

static const char* table_col_name[DATAPRV_COLNUM_PAGE] = {"时间","温度","相对湿度","气压","瞬时风","一分钟风","十分钟风","分钟雨量"};
static const char* table_col_unit[DATAPRV_COLNUM_PAGE] = {"HH:MM:SS","℃","%RH","hPa","°|m/s","°|m/s","°|m/s","mm"};
static const int table_col_width[DATAPRV_COLNUM_PAGE] = {80,70,70,80,90,90,90,80};

static HISTROY_DATA_TYPE* page_data_list = NULL;    //分配默认的显示数据

static int numOfPages = 0;                          //当前分页查询的总页数
static int indexOfPages = 0;                        //当前分页查询的页索引

static void dialog_close()
{
    if(page_data_list != NULL)
    {
        free(page_data_list);
    }

    gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean dialog_key_press(GtkWidget* window,GdkEventKey* event,gpointer data)
{
    if(event->keyval == GDK_KEY_Escape)
    {
        gtk_window_close(GTK_WINDOW(dialog));
    }
    return FALSE;
}

static void select_date_callback(GtkWidget* widget,GdkEvent* event,gpointer user_data)
{
    guint year,month,day;
    gtk_calendar_get_date(cal_select_date,&year,&month,&day);       //获取的月份索引从0开始

    PRINT_TRACE_INFO("Select Date: %04d-%02d-%02d",year,(month+1),day);

    char timeStr[32];
    memset(timeStr,0,32);
    sprintf(timeStr,"%04d-%02d-%02d",year,(month+1),day);
    gtk_button_set_label(GTK_BUTTON(btn_select_date),timeStr);
    //------------------------------------------------------
    //选择完日期后关闭窗口
    //GtkWindow* wnd_select_date = (GtkWindow *)user_data;
    //gtk_window_close(wnd_select_date);
}

static void select_date_focus_out(GtkWidget* widget,GdkEvent* event,gpointer user_data)
{
    //关闭日期选择控件
    GtkWindow* wnd_select_date = (GtkWindow *)user_data;
    gtk_window_close(wnd_select_date);
}

//点击导出数据
static void popup_select_date(GtkWidget* widget,GdkEvent* event,gpointer user_dat)
{
    GtkWidget* wnd_select_date = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(wnd_select_date),FALSE);
    gtk_window_set_position(GTK_WINDOW(wnd_select_date),GTK_WIN_POS_MOUSE);
    gtk_window_set_resizable(GTK_WINDOW(wnd_select_date),FALSE);
    gtk_window_set_modal(GTK_WINDOW(wnd_select_date),TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(wnd_select_date),GTK_WINDOW(dialog));
    //----------------------------------------------------------
    cal_select_date = (GtkCalendar *)gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(wnd_select_date),GTK_WIDGET(cal_select_date));
    g_signal_connect(G_OBJECT(cal_select_date) , "day-selected" , G_CALLBACK(select_date_callback), (void *)wnd_select_date);
    g_signal_connect(G_OBJECT(cal_select_date) , "focus-out-event" , G_CALLBACK(select_date_focus_out), (void *)wnd_select_date);
    //----------------------------------------------------------
    gtk_widget_show_all(wnd_select_date);
}

static void navgate_to_page(char* date,int stationId)
{
    int select_count = 0;
    database_search_by_page(date,stationId,indexOfPages,DATAPRV_ROWNUM_PAGE,page_data_list,&select_count);
    numOfPages = select_count/DATAPRV_COLNUM_PAGE;
    int i = 0;
    char page_str[32];

    gtk_combo_box_text_remove_all(cmb_page_select);
    for(i=0;i<numOfPages;i++)
    {
        memset(page_str,0,32);
        sprintf(page_str,"%d",(i+1));
        gtk_combo_box_text_append_text(cmb_page_select,page_str);
    }

    gtk_widget_set_size_request(GTK_WIDGET(cmb_page_select),50,30);
    //默认选中第一页
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_page_select),0);

    //更新总页数
    memset(page_str,0,32);
    sprintf(page_str,"页    共 %d 页",numOfPages);
    gtk_label_set_label(lab_page_curr,page_str);
}

//切换当前显示页
static void navigate_callback(GtkWidget* widget,GdkEvent* event,gpointer user_dat)
{
    gchar* navgPageStr = gtk_combo_box_text_get_active_text(cmb_page_select);
    const gchar* selectStation = gtk_combo_box_text_get_active_text(cmb_station_select);
    if((indexOfPages+1) == atoi(navgPageStr))
    {
        //页索引没变
    }else{
        const char* split = "|[]";
        char* field = NULL;
        field = strtok((char *)selectStation,split);
        field = strtok(NULL,split);
        int stationId = atoi(field);
        const gchar* selectDate = gtk_button_get_label(btn_select_date);

        indexOfPages = atoi(navgPageStr)-1;
        if(indexOfPages < 0)
        {
            indexOfPages = 0;
        }

        int select_count = 0;
        database_search_by_page((char *)selectDate,stationId,indexOfPages,DATAPRV_ROWNUM_PAGE,page_data_list,&select_count);
        numOfPages = select_count/DATAPRV_COLNUM_PAGE;

    }
}

//查询当前页的数据
static void select_record_current(GtkWidget* widget,GdkEvent* event,gpointer user_dat)
{
    const gchar* selectDate = gtk_button_get_label(btn_select_date);
    const gchar* selectStation = gtk_combo_box_text_get_active_text(cmb_station_select);

    PRINT_TRACE_INFO("Search %s Time: %s",selectStation,selectDate);

    const char* split = "|[]";
    char* field = NULL;
    field = strtok((char *)selectStation,split);
    field = strtok(NULL,split);
    int stationId = atoi(field);

    //重置页索引
    indexOfPages = 0;
    //---------------------------------------------
    //转到指定页
    navgate_to_page((char *)selectDate,stationId);

}

//绘制表格的回调函数
static gboolean on_data_list_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    gint row = 0;
    gint col = 0;
    guint width,height;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    cairo_set_source_rgb(cr,1,1,1);

    int innerWidth = width-DATAPRV_BORDER_WIDTH*2;
    int innerHeight = height-DATAPRV_BORDER_WIDTH*2;
    int rowHeight = innerHeight/(DATAPRV_ROWNUM_PAGE+1);    //在顶部加一行表头

    //绘制边框
    draw_rectangle(cr,DATAPRV_BORDER_WIDTH,DATAPRV_BORDER_WIDTH,innerWidth,innerHeight);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr,0,0,0);
    //------------------------------------------------------------------
    static const double gridDash[] = {4.1,1.0};
    static int gridLen = sizeof(gridDash)/sizeof(gridDash[0]);
    cairo_set_dash(cr,gridDash,gridLen,0);
    cairo_set_line_width(cr,1.0);

    //绘制表格,第一行是表头
    for(row = 0;row < (DATAPRV_ROWNUM_PAGE+1);row ++)
    {
        //最后一行不绘制横线
        if(row == DATAPRV_ROWNUM_PAGE)
        {

        }else{
            cairo_move_to(cr,DATAPRV_BORDER_WIDTH,DATAPRV_BORDER_WIDTH*3/2 + (row*rowHeight + rowHeight));
            cairo_line_to(cr,DATAPRV_BORDER_WIDTH+innerWidth,DATAPRV_BORDER_WIDTH*3/2 + (row*rowHeight + rowHeight));
        }
    }
    int startOffsetX = 0;
    for(col = 0;col < (DATAPRV_COLNUM_PAGE-1);col ++)
    {
        startOffsetX += table_col_width[col];
        cairo_move_to(cr,DATAPRV_BORDER_WIDTH+startOffsetX,DATAPRV_BORDER_WIDTH);
        cairo_line_to(cr,DATAPRV_BORDER_WIDTH+startOffsetX,DATAPRV_BORDER_WIDTH + innerHeight);
    }
    cairo_stroke(cr);
    cairo_stroke_preserve(cr);
    //------------------------------------------------------------------
    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr,rowHeight/2);
    //绘制列名
    startOffsetX = 0;
    cairo_text_extents_t font_extent;
    int colOffsetX = 0;

    for(col = 0;col < DATAPRV_COLNUM_PAGE;col ++)
    {
        cairo_text_extents(cr,table_col_name[col],&font_extent);
        colOffsetX = startOffsetX + DATAPRV_BORDER_WIDTH + table_col_width[col]/2;
        cairo_move_to(cr,colOffsetX - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight/2);
        cairo_show_text(cr,table_col_name[col]);

        cairo_text_extents(cr,table_col_unit[col],&font_extent);
        cairo_move_to(cr,colOffsetX - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight);
        cairo_show_text(cr,table_col_unit[col]);

        startOffsetX += table_col_width[col];
    }
    cairo_stroke(cr);
    cairo_stroke_preserve(cr);
    //------------------------------------------------------------------
    /**
    *   绘制数据
    **/
    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr,rowHeight/3*2);

    char buffer[32];
    for(row = 0;row < DATAPRV_ROWNUM_PAGE;row ++)
    {
        colOffsetX = 0;
        if(page_data_list[row].isValid)
        {
            //时间
            memset(buffer,0,32);
            strcpy(buffer,&page_data_list[row].dtime[11]);

            if(strlen(buffer) == 8 )
            {
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += DATAPRV_BORDER_WIDTH;
                cairo_move_to(cr,colOffsetX + table_col_width[0]/2 - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                cairo_show_text(cr,buffer);

                //温度
                memset(buffer,0,32);
                sprintf(buffer,"%0.1f",page_data_list[row].record.tempt);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[0];
                cairo_move_to(cr,colOffsetX + table_col_width[1]/2 - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                cairo_show_text(cr,buffer);

                //湿度
                memset(buffer,0,32);
                sprintf(buffer,"%0.1f",page_data_list[row].record.humid);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[1];
                cairo_move_to(cr,colOffsetX + table_col_width[2]/2 - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                cairo_show_text(cr,buffer);

                //气压
                memset(buffer,0,32);
                sprintf(buffer,"%0.1f",page_data_list[row].record.press);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[2];
                cairo_move_to(cr,colOffsetX + table_col_width[3]/2 - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                cairo_show_text(cr,buffer);

                //瞬时风
                memset(buffer,0,32);
                sprintf(buffer,"%d|",page_data_list[row].record.wind_dir_3s);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[3];
                cairo_move_to(cr,colOffsetX + table_col_width[4]/2 - font_extent.width,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                memset(buffer,0,32);
                sprintf(buffer,"%d|%0.1f",page_data_list[row].record.wind_dir_3s,page_data_list[row].record.wind_speed_3s);
                cairo_show_text(cr,buffer);

                //一分钟风
                memset(buffer,0,32);
                sprintf(buffer,"%d|",page_data_list[row].record.wind_dir_1m);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[4];
                cairo_move_to(cr,colOffsetX + table_col_width[5]/2 - font_extent.width,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                memset(buffer,0,32);
                sprintf(buffer,"%d|%0.1f",page_data_list[row].record.wind_dir_1m,page_data_list[row].record.wind_speed_1m);
                cairo_show_text(cr,buffer);

                //十分钟风
                memset(buffer,0,32);
                sprintf(buffer,"%d|",page_data_list[row].record.wind_dir_10m);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[5];
                cairo_move_to(cr,colOffsetX + table_col_width[6]/2 - font_extent.width,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                memset(buffer,0,32);
                sprintf(buffer,"%d|%0.1f",page_data_list[row].record.wind_dir_10m,page_data_list[row].record.wind_speed_10m);
                cairo_show_text(cr,buffer);

                //分钟雨量
                memset(buffer,0,32);
                sprintf(buffer,"%0.1f",page_data_list[row].record.rain);
                cairo_text_extents(cr,buffer,&font_extent);
                colOffsetX += table_col_width[6];
                cairo_move_to(cr,colOffsetX + table_col_width[7]/2 - font_extent.width/2,DATAPRV_BORDER_WIDTH + rowHeight * (row + 2));
                cairo_show_text(cr,buffer);
            }
        }else{
            //如果数据无效，后面的数据不再显示
            break;
        }
    }
    cairo_stroke(cr);

    //------------------------------------------------------------------
    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;
}


static void export_handler(void* args,FILE* pFile)
{
    if(pFile != NULL)
    {
        HISTROY_DATA_TYPE* curr_record = (HISTROY_DATA_TYPE *)args;

        if(curr_record->isValid == 1)
        {
            fprintf(pFile,"%s\t"    //时间
                    "%0.1f\t"       //温度
                    "%0.1f\t"       //湿度
                    "%0.1f\t"       //气压
                    "%d\t%0.1f\t"   //瞬时风
                    "%d\t%0.1f\t"   //一分钟风
                    "%d\t%0.1f\t"   //十分钟风
                    "%0.1f\t"       //雨量
                    "\r\n",

                    curr_record->dtime,
                    curr_record->record.tempt,
                    curr_record->record.humid,
                    curr_record->record.press,
                    curr_record->record.wind_dir_3s,curr_record->record.wind_speed_3s,
                    curr_record->record.wind_dir_1m,curr_record->record.wind_speed_1m,
                    curr_record->record.wind_dir_10m,curr_record->record.wind_speed_10m,
                    curr_record->record.rain
                    );
        }
    }
}

static gboolean record_export(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    char* saveFile = (char*)malloc(512);
    memset(saveFile,0,512);
    //生成保存文件名
    const gchar* labSelectDate = gtk_button_get_label(btn_select_date);
    sprintf(saveFile,"%sDATA\\%s_%d.txt",root_dir_path,labSelectDate,curr_config.station_id);

    FILE* fp_export;
    PRINT_TRACE_INFO("Append File: %s",saveFile);
    fp_export = fopen(saveFile,"wb");
    if(fp_export == NULL)
    {
        PRINT_ERROR_INFO("Can't open output file");
        return FALSE;
    }

    //写入导出文件的列描述
    fprintf(fp_export,"\t时间\t\t温度\t湿度\t气压\t   瞬时风\t  一分钟风\t  十分钟风\t雨量\t\r\n");

    //导出数据至文件
    HISTROY_DATA_TYPE* export_record = (HISTROY_DATA_TYPE *)malloc(sizeof(HISTROY_DATA_TYPE));
    if(export_record != NULL)
    {
        database_export_by_date((char *)labSelectDate,curr_config.station_id,export_handler,export_record,fp_export);
        free(export_record);
    }

    fclose(fp_export);
    free(saveFile);

    //禁止重复导出
    gtk_widget_set_sensitive(GTK_WIDGET(btn_export),FALSE);

    return FALSE;
}

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
    cairo_set_font_size(cr,30);
    cairo_text_extents(cr,dialog_title,&font_extent);

    cairo_set_source_rgb(cr,1,1,1);
    cairo_move_to(cr,width/2-font_extent.width/2,height/2+font_extent.height/2);

    cairo_show_text(cr,dialog_title);       //dialog_title

    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;
}

void show_data_list_dialog()
{
    int i=0;
    //创建空的结果集并设置数据默认无效
    page_data_list = (HISTROY_DATA_TYPE *)malloc(sizeof(HISTROY_DATA_TYPE) * DATAPRV_ROWNUM_PAGE);
    //------------------------------------------------------
    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_title(GTK_WINDOW(dialog),dialog_title);
    gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

    g_signal_connect(G_OBJECT(dialog), "key-press-event",G_CALLBACK(dialog_key_press),NULL);
    //------------------------------------------------------
    GtkFrame* containFrame = (GtkFrame *)gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(containFrame),10);
    gtk_container_add(GTK_CONTAINER(dialog),GTK_WIDGET(containFrame));

    GtkBox* containerBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(containFrame),GTK_WIDGET(containerBox));

    GtkDrawingArea* titleArea = (GtkDrawingArea *)gtk_drawing_area_new();
    gtk_widget_set_size_request(GTK_WIDGET(titleArea),30,DATAPRV_TITLE_HIGHT);
    gtk_box_pack_start(containerBox,GTK_WIDGET(titleArea),FALSE,FALSE,0);
    g_signal_connect(G_OBJECT(titleArea) , "draw" , G_CALLBACK(on_title_bar_paint) , NULL);

    GtkBox* toolArea = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_widget_set_size_request(GTK_WIDGET(toolArea),30,40);
    gtk_box_pack_start(containerBox,GTK_WIDGET(toolArea),FALSE,FALSE,5);

    GdkRGBA rgb_white;
    gdk_rgba_parse(&rgb_white,"WHITE");
    PangoFontDescription* ctrlFont;
    ctrlFont = pango_font_description_from_string("SIMHEI 12");

    {   //添加功能按钮
        GtkButton* btn_close = (GtkButton *)gtk_button_new_with_label("关闭");
        gtk_widget_set_size_request(GTK_WIDGET(btn_close),60,30);
        gtk_widget_override_font(GTK_WIDGET(btn_close),ctrlFont);
        gtk_box_pack_end(toolArea,GTK_WIDGET(btn_close),FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(btn_close) , "clicked" , G_CALLBACK(dialog_close) , NULL);

        btn_export = (GtkButton *)gtk_button_new_with_label("导出");
        gtk_widget_set_size_request(GTK_WIDGET(btn_export),60,30);
        gtk_widget_override_font(GTK_WIDGET(btn_export),ctrlFont);
        gtk_box_pack_end(toolArea,GTK_WIDGET(btn_export),FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(btn_export) , "clicked" , G_CALLBACK(record_export) , NULL);

        GtkButton* btn_search = (GtkButton *)gtk_button_new_with_label("查询");
        gtk_widget_set_size_request(GTK_WIDGET(btn_search),60,30);
        gtk_widget_override_font(GTK_WIDGET(btn_search),ctrlFont);
        gtk_box_pack_end(toolArea,GTK_WIDGET(btn_search),FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(btn_search) , "clicked" , G_CALLBACK(select_record_current) , NULL);
        //----------------------------------------------------------------------
        GtkLabel* lab_station_select = (GtkLabel *)gtk_label_new("选择观测点:");
        gtk_widget_override_color(GTK_WIDGET(lab_station_select),GTK_STATE_FLAG_NORMAL,&rgb_white);
        gtk_widget_override_font(GTK_WIDGET(lab_station_select),ctrlFont);
        gtk_box_pack_start(toolArea,GTK_WIDGET(lab_station_select),FALSE,FALSE,0);

        cmb_station_select = database_load_stations();
        gtk_widget_override_font(GTK_WIDGET(cmb_station_select),ctrlFont);
        gtk_box_pack_start(toolArea,GTK_WIDGET(cmb_station_select),FALSE,FALSE,0);

        //----------------------------------------------------------------------
        GtkLabel* lab_date_select = (GtkLabel *)gtk_label_new("    选择日期:");
        gtk_widget_override_color(GTK_WIDGET(lab_date_select),GTK_STATE_FLAG_NORMAL,&rgb_white);
        gtk_widget_override_font(GTK_WIDGET(lab_date_select),ctrlFont);
        gtk_box_pack_start(toolArea,GTK_WIDGET(lab_date_select),FALSE,FALSE,0);

        {
            //获取当前日期
            char timeStr[32];
            time_t times;
            struct tm* p_time;
            time(&times);
            p_time = localtime(&times);
            memset(timeStr,0,32);
            sprintf(timeStr,"%04d-%02d-%02d",p_time->tm_year+1900,(p_time->tm_mon+1),p_time->tm_mday);

            //查询一页当天的数据
            int select_count = 0;
            database_search_by_page(timeStr,curr_config.station_id,0,DATAPRV_ROWNUM_PAGE,page_data_list,&select_count);
            printf("Total Count: %d \r\n",select_count);
            numOfPages = select_count/DATAPRV_ROWNUM_PAGE;

            PRINT_TRACE_INFO("Select count(*): %d",numOfPages);

            btn_select_date = (GtkButton *)gtk_button_new_with_label(timeStr);
            gtk_widget_set_size_request(GTK_WIDGET(btn_select_date),80,30);
            gtk_widget_override_font(GTK_WIDGET(btn_select_date),ctrlFont);
            gtk_box_pack_start(toolArea,GTK_WIDGET(btn_select_date),FALSE,FALSE,0);
            g_signal_connect(G_OBJECT(btn_select_date) , "clicked" , G_CALLBACK(popup_select_date) , NULL);
        }
    }

    {   //绘制表格部分
        tableArea = (GtkDrawingArea *)gtk_drawing_area_new();
        gtk_box_pack_start(containerBox,GTK_WIDGET(tableArea),TRUE,TRUE,0);
        g_signal_connect(G_OBJECT(tableArea) , "draw" , G_CALLBACK(on_data_list_paint) , NULL);
    }

    GtkBox* pageArea = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_widget_set_size_request(GTK_WIDGET(pageArea),30,40);
    gtk_box_pack_end(containerBox,GTK_WIDGET(pageArea),FALSE,FALSE,5);
    {
        char page_str[32];

        memset(page_str,0,32);
        sprintf(page_str," 页    共 %d 页",numOfPages);

        lab_page_curr = (GtkLabel *)gtk_label_new(page_str);
        gtk_widget_override_color(GTK_WIDGET(lab_page_curr),GTK_STATE_FLAG_NORMAL,&rgb_white);
        gtk_widget_override_font(GTK_WIDGET(lab_page_curr),ctrlFont);
        gtk_box_pack_end(pageArea,GTK_WIDGET(lab_page_curr),FALSE,FALSE,0);

        {
            //页码改为下拉列表
            cmb_page_select = (GtkComboBoxText *)gtk_combo_box_text_new();
            gtk_widget_override_font(GTK_WIDGET(cmb_page_select),ctrlFont);
            gtk_combo_box_text_remove_all(cmb_page_select);
            for(i=0;i<numOfPages;i++)
            {
                memset(page_str,0,32);
                sprintf(page_str,"%d",(i+1));
                if(strlen(page_str) > 0)
                {
                    gtk_combo_box_text_append_text(cmb_page_select,page_str);
                }
            }
            gtk_box_pack_end(pageArea,GTK_WIDGET(cmb_page_select),FALSE,FALSE,0);
            gtk_widget_set_size_request(GTK_WIDGET(cmb_page_select),50,30);
            g_signal_connect(G_OBJECT(cmb_page_select) , "changed" , G_CALLBACK(navigate_callback) , NULL);
            //-----------------------------------------
            //页下拉框默认选择第一页
            gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_page_select),0);
        }

        GtkLabel* lab_page_navg = (GtkLabel *)gtk_label_new("转到 ");
        gtk_widget_override_color(GTK_WIDGET(lab_page_navg),GTK_STATE_FLAG_NORMAL,&rgb_white);
        gtk_widget_override_font(GTK_WIDGET(lab_page_navg),ctrlFont);
        gtk_box_pack_end(pageArea,GTK_WIDGET(lab_page_navg),FALSE,FALSE,0);

        //重置页索引
        indexOfPages = 0;

    }
    //------------------------------------------------------
    GdkRGBA bkgClr = {APP_BKG_RGB, 1};
    gtk_widget_override_background_color(GTK_WIDGET(dialog),GTK_STATE_FLAG_NORMAL,&bkgClr);
    gtk_widget_set_size_request(GTK_WIDGET(dialog),DATAPRV_DLG_WIDTH,DATAPRV_DLG_HEIGHT);
    gtk_widget_set_opacity(GTK_WIDGET(dialog),0.95);
    gtk_widget_show_all(dialog);
}


