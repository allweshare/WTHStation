#include "StatSetDialog.h"
#include "Configuration.h"
#include "CustomDraw.h"
#include "DataBase.h"
#include "main.h"
#include <gtk/gtk.h>

extern volatile CONFIG_TYPE curr_config;

static GtkWidget* dialog;
static GtkRadioButton* rdbChooseExists;
static GtkRadioButton* rdbCreateNew;
static GtkComboBoxText* cmbChooseStation;
static GtkEntry* txtCreateStation;
static const char* dialog_title = "测站设置";


static gboolean dialog_key_press(GtkWidget* window,GdkEventKey* event,gpointer data)
{
    if(event->keyval == GDK_KEY_Escape)
    {
        gtk_window_close(GTK_WINDOW(dialog));
    }
    return FALSE;
}

static void station_ensure()
{
    //选择已有的观测点
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdbChooseExists)))
    {
        GtkTreeIter itert;
        gint selectIndex = 0;
        char* itemText = NULL;
        GtkTreeModel* comboMode = gtk_combo_box_get_model(GTK_COMBO_BOX(cmbChooseStation));

        gtk_tree_model_get_iter_first(comboMode,&itert);
        selectIndex = 0;

        while(gtk_tree_model_iter_next(comboMode,&itert))
        {
            selectIndex ++;
            //检查当前选中项
            if(gtk_combo_box_get_active(GTK_COMBO_BOX(cmbChooseStation)) == selectIndex)
            {
                gtk_tree_model_get(comboMode,&itert,0,&itemText,-1);
                //---------------------------------
                //保存选择的观测点
                char* sname = strtok(itemText,"|[]");
                char* sid = strtok(NULL,"|[]");
                curr_config.station_id = atoi(sid);
                memset((void *)curr_config.station_name,0,STATION_NAME_LEN);
                strcpy((char *)curr_config.station_name,sname);
                //保存到配置文件
                saveConfig();
            }
        }
    }
    //创建新的观测点
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdbCreateNew)))
    {
        //GtkEntry* txtCreateStation;
        const gchar* stationName = gtk_entry_get_text(txtCreateStation);
        database_add_station((char *)stationName);
        int stationId = last_insert_rowid();

        curr_config.station_id = stationId;
        memset((void *)curr_config.station_name,0,STATION_NAME_LEN);
        strcpy((char *)curr_config.station_name,stationName);
        //保存到配置文件
        saveConfig();
    }
    //------------------------------------
    gtk_window_close(GTK_WINDOW(dialog));
}

static void dialog_close()
{
    gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean on_container_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    guint width,height;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

//    cairo_pattern_t* top_patter = cairo_pattern_create_linear(0,);
//    cairo_pattern_add_color_stop_rgba(top_patter, 0, 1, 1, 1, 1);
//    cairo_pattern_add_color_stop_rgba(top_patter, 0.7, APP_BKG_RGB, 1);
//    cairo_pattern_add_color_stop_rgba(top_patter, 1, 0.7, 0.7, 0.7, 1);

    cairo_set_source_rgb(cr, 0.83, 0.83, 0.83);
    draw_rectangle(cr,width*0.3/2,height*0.3/2 + STATSET_TITLE_HIGHT,width*0.7,height*0.7);

    cairo_fill(cr);
    cairo_destroy(cr);
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

void show_station_set_dialog()
{
    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //g_signal_connect(G_OBJECT(dialog),"destroy",G_CALLBACK(dialog_close),NULL);
    gtk_window_set_decorated(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_title(GTK_WINDOW(dialog),dialog_title);
    gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
    GdkRGBA bkgClr = {APP_BKG_RGB, 1};
    gtk_widget_override_background_color(GTK_WIDGET(dialog),GTK_STATE_FLAG_NORMAL,&bkgClr);
    gtk_window_set_resizable(GTK_WINDOW(dialog),FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

    g_signal_connect(G_OBJECT(dialog), "key-press-event",G_CALLBACK(dialog_key_press),NULL);

    //----------------------------------------
    //重新加载一起配置
    loadConfig();
    //----------------------------------------

    GtkBox* dialogBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_VERTICAL,0);

    GtkDrawingArea* titleArea = (GtkDrawingArea *)gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(titleArea) , "draw" ,
              G_CALLBACK(on_title_bar_paint) , NULL);

    gtk_widget_set_size_request(GTK_WIDGET(titleArea),30,STATSET_TITLE_HIGHT);
    gtk_box_pack_start(dialogBox,GTK_WIDGET(titleArea),FALSE,FALSE,0);

    //Container Frame
    GtkFrame* containBox = (GtkFrame *)gtk_frame_new(NULL);
    GtkFixed* container = (GtkFixed *)gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(containBox),GTK_WIDGET(container));

    gtk_frame_set_shadow_type(containBox,GTK_SHADOW_NONE);
    g_signal_connect(G_OBJECT(containBox) , "draw" ,
              G_CALLBACK(on_container_paint) , NULL);
    gtk_box_pack_start(dialogBox,GTK_WIDGET(containBox),TRUE,TRUE,0);

    PangoFontDescription* ctrlFont;
    ctrlFont = pango_font_description_from_string("SIMHEI 13");
    {
        GSList* choose_group = NULL;

        //使用已有观测点的单选按钮
        rdbChooseExists = (GtkRadioButton *)gtk_radio_button_new_with_label(NULL,"选择已有观测点");
        choose_group = gtk_radio_button_get_group(rdbChooseExists);
        gtk_widget_override_font(GTK_WIDGET(rdbChooseExists),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(rdbChooseExists),200,30);
        gtk_fixed_put(container,GTK_WIDGET(rdbChooseExists),150,STATSET_TITLE_HIGHT + 10);

        //添加下拉列表框
        //GtkCellRenderer* listRender = gtk_cell_renderer_text_new();
        cmbChooseStation = database_load_stations();
        gtk_widget_override_font(GTK_WIDGET(cmbChooseStation),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(cmbChooseStation),200,30);
        gtk_fixed_put(container,GTK_WIDGET(cmbChooseStation),150+30,STATSET_TITLE_HIGHT + 10 + 30);

        //创建新的观测点
        rdbCreateNew = (GtkRadioButton *)gtk_radio_button_new_with_label(choose_group,"创建新的观测点");
        gtk_widget_override_font(GTK_WIDGET(rdbCreateNew),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(rdbCreateNew),200,30);
        gtk_fixed_put(container,GTK_WIDGET(rdbCreateNew),150,STATSET_TITLE_HIGHT + 10 + 80);

        //添加观测点输入框
        txtCreateStation = (GtkEntry *)gtk_entry_new();
        gtk_widget_override_font(GTK_WIDGET(txtCreateStation),ctrlFont);
        gtk_widget_set_size_request(GTK_WIDGET(txtCreateStation),200,30);
        gtk_fixed_put(container,GTK_WIDGET(txtCreateStation),150+30,STATSET_TITLE_HIGHT + 120);

        //默认选择已有观测点
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rdbChooseExists),TRUE);

    }

    PangoFontDescription* btnFont;
    btnFont = pango_font_description_from_string("SIMHEI 16");
    //Bottom Button Box
    GtkBox* bottomBox = (GtkBox *)gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    {
        //Add Ensure Button
        GtkButton* btnEnsure = (GtkButton *)gtk_button_new_with_label("确定");
        gtk_widget_override_font(GTK_WIDGET(btnEnsure),btnFont);
        gtk_widget_set_size_request(GTK_WIDGET(btnEnsure),STATSET_DLG_WIDTH/3,50);
        gtk_box_pack_start(bottomBox,GTK_WIDGET(btnEnsure),FALSE,FALSE,50);
        g_signal_connect(G_OBJECT(btnEnsure) , "clicked" , G_CALLBACK(station_ensure) , NULL);
    }
    {
        //Add Cancel Button
        GtkButton* btnCancel = (GtkButton *)gtk_button_new_with_label("取消");
        gtk_widget_override_font(GTK_WIDGET(btnCancel),btnFont);
        gtk_widget_set_size_request(GTK_WIDGET(btnCancel),STATSET_DLG_WIDTH/3,50);
        gtk_box_pack_end(bottomBox,GTK_WIDGET(btnCancel),FALSE,FALSE,50);
        g_signal_connect(G_OBJECT(btnCancel) , "clicked" , G_CALLBACK(dialog_close) , NULL);
    }
    gtk_box_pack_end(dialogBox,GTK_WIDGET(bottomBox),FALSE,TRUE,20);

    gtk_container_add(GTK_CONTAINER(dialog),GTK_WIDGET(dialogBox));
    gtk_widget_set_size_request(GTK_WIDGET(dialog),STATSET_DLG_WIDTH,STATSET_DLG_HEIGHT);
    gtk_widget_set_opacity(GTK_WIDGET(dialog),0.95);
    gtk_widget_show_all(dialog);
}


