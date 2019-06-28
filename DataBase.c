#include <io.h>
#include <sqlite3.h>

#include "DataBase.h"
#include "Configuration.h"
#include "main.h"

extern volatile CONFIG_TYPE curr_config;

static uint8_t* database_url = NULL;
static sqlite3* connection = NULL;

static char* sql_create_table_stations =    "create table stations("    \
                                            "   [SID] integer primary key autoincrement,"  \
                                            "   [SNAME] varchar(64) unique);";

static char* sql_create_table_records =     "create table records("     \
                                            "   [DTIME] datetime primary key asc,"  \
                                            "   [STATION] integer not null,"    \
                                            "   [TEMP]	float,"   \
                                            "   [HUMID] float,"   \
                                            "   [PRESS] float,"   \
                                            "   [WD_3S] SMALLINT,"   \
                                            "   [WS_3S] float,"   \
                                            "   [WD_1M] SMALLINT,"   \
                                            "   [WS_1M] float,"   \
                                            "   [WD_TM] SMALLINT,"   \
                                            "   [WS_TM] float,"   \
                                            "   [RAIN] float,"   \
                                            "   [VOLTAGE] float);";

static char* sql_select_all_stations =      "select [SID],[SNAME] from stations;";

static char* sql_select_records_by_page =   "select * from records where [DTIME] between "          \
                                            "(:sdate||' 00:00:00') and (:sdate||' 23:59:59') "		\
                                            "and [STATION] = :station order by [DTIME] desc "       \
                                            "limit :numofrow offset :rowindex;";

static char* sql_select_export_by_date =    "select * from records where [DTIME] between "          \
                                            "(:sdate||' 00:00:00') and (:sdate||' 23:59:59') and "  \
                                            "[STATION] = :station order by [DTIME] asc;";

static char* sql_select_num_of_count =      "select count(*) from records where [DTIME] between"    \
                                            ":sdate||' 00:00:00' and :sdate||' 23:59:59' and [STATION] = :station;";

static char* sql_insert_new_stations =      "insert into stations([SNAME]) values (:sname);";

static char* sql_insert_new_records =       "insert into records([DTIME],[STATION],[TEMP],[HUMID],[PRESS],"     \
                                            "[WD_3S],[WS_3S],[WD_1M],[WS_1M],[WD_TM],[WS_TM],"      \
                                            "[RAIN],[VOLTAGE]) values (:time,:station,:tempt,:humid,:press,"    \
                                            ":wd_3s,:ws_3s,:wd_1m,:ws_1m,:wd_tm,:ws_tm,:rain,:voltage);";

static char* sql_select_last_rowid =        "select last_insert_rowid();";

static int stationIndex = 0;

static int select_stations_callback(void* arg,int argc,char** values,char** names)
{
    GtkComboBoxText* cmbChooseStation = (GtkComboBoxText *)arg;
    char stationName[64];
    //检查查询的结果集是否为两列
    if(argc == 2)
    {
        if((strcmp(names[0],"SID") == 0) && (strcmp(names[1],"SNAME") == 0))
        {
            memset(stationName,0,64);
            sprintf((char *)stationName,"%s|[%s]",values[1],values[0]);
            gtk_combo_box_text_append_text(cmbChooseStation,stationName);
            //-------------------------------------------------------
            //检查默认选中项
            if(curr_config.station_id == atoi(values[0]))
            {
                gtk_combo_box_set_active(GTK_COMBO_BOX(cmbChooseStation),stationIndex+1);       //第一项为默认观测点，所以+1
            }
            stationIndex ++;
        }
    }
    return 0;
}

void database_add_station(char* sname)
{
    int insert = 0;
    sqlite3_stmt* stmt = NULL;
    insert = sqlite3_prepare_v2(connection,sql_insert_new_stations,-1,&stmt,NULL);
    if(insert == SQLITE_OK)
    {
        int ci = sqlite3_bind_parameter_index(stmt,":sname");
        sqlite3_bind_text(stmt,ci,sname,-1,SQLITE_STATIC);

        insert = sqlite3_step(stmt);
        if((insert == SQLITE_DONE) || (insert == SQLITE_ROW))
        {
            PRINT_TRACE_INFO("Create New Station: %s",sname);
            //---------------------------------
            //更新配置文件
            memset((void *)curr_config.station_name,0,STATION_NAME_LEN);
            strcpy((char *)curr_config.station_name,sname);

        }else{
            PRINT_ERROR_INFO("Create Station Fault: %s",sname);
        }

        sqlite3_finalize(stmt);
    }
}

void database_add_record(char* dtime,int stationId,REALTIME_DATA_TYPE* data)
{
    if(dtime == NULL)
    {
        PRINT_ERROR_INFO("Insert record contains NULL time")
    }else{
        int insert = 0;
        sqlite3_stmt* stmt = NULL;
        insert = sqlite3_prepare_v2(connection,sql_insert_new_records,-1,&stmt,NULL);
        if(insert == SQLITE_OK)
        {
            int pi = 0;
            //当前选择的观测点
            int currStationId = curr_config.station_id;
            //时间
            pi = sqlite3_bind_parameter_index(stmt,":time");
            sqlite3_bind_text(stmt,pi,dtime,-1,SQLITE_STATIC);
            //观测点
            pi = sqlite3_bind_parameter_index(stmt,":station");
            sqlite3_bind_int(stmt,pi,currStationId);
            //温度
            pi = sqlite3_bind_parameter_index(stmt,":tempt");
            sqlite3_bind_double(stmt,pi,data->tempt);
            //湿度
            pi = sqlite3_bind_parameter_index(stmt,":humid");
            sqlite3_bind_double(stmt,pi,data->humid);
            //气压
            pi = sqlite3_bind_parameter_index(stmt,":press");
            sqlite3_bind_double(stmt,pi,data->press);
            //三秒风-风向
            pi = sqlite3_bind_parameter_index(stmt,":wd_3s");
            sqlite3_bind_int(stmt,pi,data->wind_dir_3s);
            //三秒风-风速
            pi = sqlite3_bind_parameter_index(stmt,":ws_3s");
            sqlite3_bind_double(stmt,pi,data->wind_speed_3s);
            //一分钟风-风向
            pi = sqlite3_bind_parameter_index(stmt,":wd_1m");
            sqlite3_bind_int(stmt,pi,data->wind_dir_1m);
            //一分钟风-风速
            pi = sqlite3_bind_parameter_index(stmt,":ws_1m");
            sqlite3_bind_double(stmt,pi,data->wind_speed_1m);
            //十分钟风-风向
            pi = sqlite3_bind_parameter_index(stmt,":wd_tm");
            sqlite3_bind_int(stmt,pi,data->wind_dir_10m);
            //十分钟风-风速
            pi = sqlite3_bind_parameter_index(stmt,":ws_tm");
            sqlite3_bind_double(stmt,pi,data->wind_speed_10m);
            //分钟雨量
            pi = sqlite3_bind_parameter_index(stmt,":rain");
            sqlite3_bind_double(stmt,pi,data->rain);
            //供电电压
            pi = sqlite3_bind_parameter_index(stmt,":voltage");
            sqlite3_bind_double(stmt,pi,data->voltage);

            /**
            *   执行SQL语句
            **/
            insert = sqlite3_step(stmt);
            if((insert == SQLITE_DONE) || (insert == SQLITE_ROW))
            {
                PRINT_TRACE_INFO("Insert New Record: %s",dtime);
            }else{
                PRINT_ERROR_INFO("Insert Record Fault: %s",dtime);
            }

            sqlite3_finalize(stmt);
        }
    }
}

static int select_rowid_callback(void* arg,int argc,char** values,char** names)
{
    int* rowId = (int *)arg;
    if(argc == 1)
    {
        *(int *)rowId = atoi(values[0]);
    }
    return 0;
}

int last_insert_rowid(void)
{
    int select = 0;
    int rowid = -1;
    char* err_msg = NULL;
    select = sqlite3_exec(connection,sql_select_last_rowid,select_rowid_callback,&rowid,&err_msg);
    if(select == SQLITE_OK)
    {
        //printf("Select RowID: %d \r\n",rowid);
    }
    if(err_msg != NULL)
    {
        sqlite3_free(err_msg);
    }
    return rowid;
}

GtkComboBoxText* database_load_stations()
{
    int select = 0;
    char* err_msg = NULL;

    GtkComboBoxText* cmbChooseStation = (GtkComboBoxText *)gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(cmbChooseStation,"默认观测点");

    stationIndex = 0;       //清空索引
    select = sqlite3_exec(connection,sql_select_all_stations,select_stations_callback,cmbChooseStation,&err_msg);
    if(select == SQLITE_OK)
    {
        //检查配置文件中的观测点
        if(curr_config.station_id == 0)      //未指定观测点
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(cmbChooseStation),0);
        }else{
        }
    }else{
        if(err_msg != NULL)
        {
            PRINT_ERROR_INFO("Select Stations Error: %s",err_msg);
        }
        sqlite3_free(err_msg);
    }
    return cmbChooseStation;

}

void database_search_by_page(char* date,int stationId,int pageIndex,int pageSize,HISTROY_DATA_TYPE* datas,int* numofPages)
{
    int insert = 0;
    int select = 0;
    int rowIndex = 0;

    //在查询数据并填充结果集前，要先将缓存的数据标记为无效
    for(rowIndex=0;rowIndex<pageSize;rowIndex++)
    {
        datas[rowIndex].isValid = 0;
    }
    //---------------------------------------------------------
    sqlite3_stmt* stmt = NULL;
    insert = sqlite3_prepare_v2(connection,sql_select_records_by_page,-1,&stmt,NULL);
    if(insert == SQLITE_OK)
    {
        int pi = 0;

        pi = sqlite3_bind_parameter_index(stmt,":sdate");
        sqlite3_bind_text(stmt,pi,date,-1,SQLITE_STATIC);

        pi = sqlite3_bind_parameter_index(stmt,":station");
        sqlite3_bind_int(stmt,pi,stationId);

        pi = sqlite3_bind_parameter_index(stmt,":numofrow");        //分页查询的页大小
        sqlite3_bind_int(stmt,pi,pageSize);

        pi = sqlite3_bind_parameter_index(stmt,":rowindex");
        sqlite3_bind_int(stmt,pi,pageIndex * pageSize);

        for(rowIndex = 0;rowIndex < pageSize;rowIndex ++)
        {
            if(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if(&datas[rowIndex] != NULL)
                {
                    //时间
                    const unsigned char* timeInfo = sqlite3_column_text(stmt,0);
                    memset(datas[rowIndex].dtime,0,32);
                    strcpy(datas[rowIndex].dtime,(char *)timeInfo);

                    //温度
                    float tempVal = sqlite3_column_double(stmt,2);
                    datas[rowIndex].record.tempt = tempVal;

                    //湿度
                    float humidVal = sqlite3_column_double(stmt,3);
                    datas[rowIndex].record.humid = humidVal;

                    //气压
                    float pressVal = sqlite3_column_double(stmt,4);
                    datas[rowIndex].record.press = pressVal;

                    //瞬时风
                    int wd_3s = sqlite3_column_int(stmt,5);
                    datas[rowIndex].record.wind_dir_3s = wd_3s;
                    float ws_3s = sqlite3_column_double(stmt,6);
                    datas[rowIndex].record.wind_speed_3s = ws_3s;

                    //一分钟风
                    int wd_1m = sqlite3_column_int(stmt,7);
                    datas[rowIndex].record.wind_dir_1m = wd_1m;
                    float ws_1m = sqlite3_column_double(stmt,8);
                    datas[rowIndex].record.wind_speed_1m = ws_1m;

                    //十分钟风
                    int wd_tm = sqlite3_column_int(stmt,9);
                    datas[rowIndex].record.wind_dir_10m = wd_tm;
                    float ws_tm = sqlite3_column_double(stmt,10);
                    datas[rowIndex].record.wind_speed_10m = ws_tm;

                    //雨量
                    float rainVal = sqlite3_column_double(stmt,11);
                    datas[rowIndex].record.rain = rainVal;

                    //--------------------------------
                    datas[rowIndex].isValid = 1;        //数据标记为有效
                }
            }
        }

        sqlite3_finalize(stmt);
    }
    //------------------------------------------------------------
    //执行第二次查询,返回记录总数
    select = sqlite3_prepare_v2(connection,sql_select_num_of_count,-1,&stmt,NULL);
    if(select == SQLITE_OK)
    {
        int pi = 0;

        pi = sqlite3_bind_parameter_index(stmt,":sdate");
        sqlite3_bind_text(stmt,pi,date,-1,SQLITE_STATIC);

        pi = sqlite3_bind_parameter_index(stmt,":station");
        sqlite3_bind_int(stmt,pi,stationId);

        if(sqlite3_step(stmt) == SQLITE_ROW)
        {
            int count = sqlite3_column_int(stmt,0);
            if(numofPages != NULL)
            {
                *(int *)numofPages = count;
            }
        }
    }
    sqlite3_finalize(stmt);


}

void database_export_by_date(char* date,int stationId,void(* callback)(void* args,FILE* pFile),void* params,FILE* fWriteTo)
{
    int select = 0;
    sqlite3_stmt* stmt = NULL;
    HISTROY_DATA_TYPE* curr_record = (HISTROY_DATA_TYPE *)params;
    select = sqlite3_prepare_v2(connection,sql_select_export_by_date,-1,&stmt,NULL);
    if(select == SQLITE_OK)
    {
        int pi = 0;

        pi = sqlite3_bind_parameter_index(stmt,":sdate");
        sqlite3_bind_text(stmt,pi,date,-1,SQLITE_STATIC);

        pi = sqlite3_bind_parameter_index(stmt,":station");
        sqlite3_bind_int(stmt,pi,stationId);

        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if(curr_record != NULL)
            {
                //时间
                const unsigned char* timeInfo = sqlite3_column_text(stmt,0);
                memset(curr_record->dtime,0,32);
                strcpy((char *)curr_record->dtime,(char *)timeInfo);

                //温度
                float tempVal = sqlite3_column_double(stmt,2);
                curr_record->record.tempt = tempVal;

                //湿度
                float humidVal = sqlite3_column_double(stmt,3);
                curr_record->record.humid = humidVal;

                //气压
                float pressVal = sqlite3_column_double(stmt,4);
                curr_record->record.press = pressVal;

                //瞬时风
                int wd_3s = sqlite3_column_int(stmt,5);
                curr_record->record.wind_dir_3s = wd_3s;
                float ws_3s = sqlite3_column_double(stmt,6);
                curr_record->record.wind_speed_3s = ws_3s;

                //一分钟风
                int wd_1m = sqlite3_column_int(stmt,7);
                curr_record->record.wind_dir_1m = wd_1m;
                float ws_1m = sqlite3_column_double(stmt,8);
                curr_record->record.wind_speed_1m = ws_1m;

                //十分钟风
                int wd_tm = sqlite3_column_int(stmt,9);
                curr_record->record.wind_dir_10m = wd_tm;
                float ws_tm = sqlite3_column_double(stmt,10);
                curr_record->record.wind_speed_10m = ws_tm;

                //雨量
                float rainVal = sqlite3_column_double(stmt,11);
                curr_record->record.rain = rainVal;

                curr_record->isValid = 1;

                //调用处理函数处理当前的记录
                callback(curr_record,fWriteTo);

            }
        }

        sqlite3_finalize(stmt);
    }
}

static void database_check(void)
{
    int create_ret = 0;
    char* err_msg = NULL;
    create_ret = sqlite3_exec(connection,sql_create_table_stations,NULL,NULL,&err_msg);
    if(create_ret != SQLITE_OK)
    {
        if(err_msg != NULL)     //Check if is exists
        {
            char* exists = NULL;
            exists = strstr(err_msg,"already exists");
            if(exists != NULL)
            {
                PRINT_TRACE_INFO("Table [stations] Already Exists");
            }
        }else{
            PRINT_ERROR_INFO("Create Table Error: %s",err_msg);
        }
        sqlite3_free(err_msg);
    }
    //-----------------------------------------------------
    create_ret = sqlite3_exec(connection,sql_create_table_records,NULL,NULL,&err_msg);
    if(create_ret != SQLITE_OK)
    {
        if(err_msg != NULL)     //Check if is exists
        {
            char* exists = NULL;
            exists = strstr(err_msg,"already exists");
            if(exists != NULL)
            {
                PRINT_TRACE_INFO("Table [records] Already Exists");
            }
        }else{
            PRINT_ERROR_INFO("Create Table Error: %s",err_msg);
        }
        sqlite3_free(err_msg);
    }
}

void database_init(int argc, char* argv[])
{
    int rc = 0;
    char* exePath = strrchr(argv[0],'\\'); //Find last separator
    if(exePath > argv[0])
    {
        database_url = malloc(DATABASE_PATH_LEN);
        memset(database_url,0,DATABASE_PATH_LEN);
        memcpy(database_url,argv[0],(exePath - argv[0] + 1));
        memcpy(&database_url[exePath - argv[0] + 1],DATABASE_DIR_NAME,strlen((char *)DATABASE_DIR_NAME));

        //Create Data Dir if not exists
        if(access((char *)database_url, F_OK) != 0)
        {
            mkdir((char *)database_url);
        }
        strcat((char *)database_url,"\\DATA.DB");

        //Check Database
        rc = sqlite3_open_v2((char *)database_url,&connection,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,NULL);
        if(rc == SQLITE_OK)
        {
            PRINT_TRACE_INFO("Open DataBase Success ...");

            database_check();
        }else{
            PRINT_TRACE_INFO("Open DataBase Fault: %s",database_url);
        }

        //--------------------------------------------------



    }

}

void database_close(void)
{
    sqlite3_close(connection);
    free(database_url);
}

