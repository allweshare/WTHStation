#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>

#include "protocol.h"

#define DATABASE_PATH_LEN       256
#define DATABASE_DIR_NAME       "DATA"

void database_init(int argc, char* argv[]);

void database_add_station(char* sname);     //添加新的观测点

void database_add_record(char* dtime,int stationId,REALTIME_DATA_TYPE* data);

GtkComboBoxText* database_load_stations();

/**
*   分页查询指定观测点的数据,按照日期分页查询
*   date: 要查询的日期,如 2017-07-21
*   pageIndex: 当前查询的页
*   pageSize: 分页查询,每页显示的数据量
*   datas: 输出的数据结果集
*   numofPages: 返回分页查询的总页数
**/
void database_search_by_page(char* date,int stationId,int pageIndex,int pageSize,HISTROY_DATA_TYPE* datas,int* numofPages);

/**
*   导出指定日期的数据
**/
void database_export_by_date(char* date,int stationId,void(* callback)(void* args,FILE* pFile),void* params,FILE* fWriteTo);

int last_insert_rowid(void);                //获取自增列的ID

void database_close(void);

#endif // DATABASE_H_INCLUDED
