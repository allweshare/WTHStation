#include "TitleBar.h"
#include "main.h"
#include <gtk/gtk.h>

//绘制标题栏的回调函数
static gboolean on_title_bar_paint(GtkWidget* widget,
                GdkEventAny* event,
                gpointer data)
{
    cairo_t* cr;
    const char* titleName = APP_TITLE_NAME;
    double startX = 0,startY = 0;
    guint width,height;
    cairo_text_extents_t font_extent;

    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_select_font_face(cr,"SIMHEI",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr,30);
    cairo_text_extents(cr,titleName,&font_extent);

    cairo_set_source_rgb (cr, 1, 1, 1);

    //根据显示内容的宽度计算位置
    startX = width/2 - (font_extent.width/2 + font_extent.x_bearing);
    startY = height/2 - (font_extent.height/2 + font_extent.y_bearing);
    //居中显示软件标题
    cairo_move_to(cr,startX,startY);
    cairo_show_text(cr,titleName);

    cairo_fill(cr);
    cairo_destroy(cr);

    return FALSE;
}

//Create Title Bar
GtkDrawingArea* build_title_bar()
{
    GtkDrawingArea* titleBar = (GtkDrawingArea *)gtk_drawing_area_new();

    gtk_widget_set_size_request(GTK_WIDGET(titleBar),APP_TITLE_HEIGHT,APP_TITLE_HEIGHT);

    g_signal_connect(G_OBJECT(titleBar) , "draw" ,
              G_CALLBACK(on_title_bar_paint) , NULL);

    return titleBar;
}


