#ifndef OPTIONPANE_H_INCLUDED
#define OPTIONPANE_H_INCLUDED

#include <gtk/gtk.h>

//创建状态面板
GtkDrawingArea* build_option_pane();

//鼠标移动事件
void option_pane_move(double x,double y);

//鼠标单击事件
void option_pane_click(double x,double y);

//应用程序结束后释放内存
void option_pane_release();

#endif // OPTIONPANE_H_INCLUDED
