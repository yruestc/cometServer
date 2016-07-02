/*
 * =====================================================================================
 *
 *       Filename:  cometserver_db.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月19日 16时02分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include"cometserver_db.h"

#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<mysql.h>
#include<boost/lexical_cast.hpp>

#define HOST	"localhost"
#define USER	"allen"
#define PASSWORD	""
#define DATABASE	"cometserver"

extern bool _verbose ;		//是否显示详细信息

void exe_sql(const char *sql)       //执行写入数据库操作
{
	MYSQL connection;	//数据库连接句柄
	int result;

	mysql_init(&connection);	//初始化mysql连接connection

	if (mysql_real_connect
	    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
	     CLIENT_FOUND_ROWS)) {
        if(_verbose)
    		printf("connect mysql success!\n");

		mysql_query(&connection, "set names utf8");	//设置查询编码为utf8，这样就可以支持中文了

		result = mysql_query(&connection, sql);

		if (result)	//返回非0,标识查询失败
		{
			fprintf(stderr, "mysql query failed!\n");
			fprintf(stderr, "Query error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
			mysql_close(&connection);	//关闭mysql连接
		} else {	//返回0，标识查询成功
            if(_verbose)
    			printf("%d rows affected\n",
    			       (int)mysql_affected_rows(&connection));
			mysql_close(&connection);	//关闭连接
		}

	} else {
		fprintf(stderr, "mysql_real_connect failed!\n");
		if (mysql_error(&connection))	//显示错误原因
		{
			fprintf(stderr, "Connection error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
		}
	}
}

void query_sql(const char *sql)         //将查询结果显示
{
	MYSQL connection;
	int result;
	MYSQL_RES *res_ptr;
	MYSQL_FIELD *field;
	MYSQL_ROW result_row;
	int column, row;

	mysql_init(&connection);

	if (mysql_real_connect
	    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
	     CLIENT_FOUND_ROWS)) {
        if(_verbose)
    		printf("mysql connnect success!\n");
		mysql_query(&connection, "set names utf8");

		result = mysql_query(&connection, sql);

		if (result)	//返回非0，失败
		{
			fprintf(stderr, "mysql query failed!\n");
			fprintf(stderr, "Query error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
			mysql_close(&connection);	//关闭mysql连接
		} else {	//返回0，成功
			res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

			if (res_ptr) {
				column = mysql_num_fields(res_ptr);	//结果集中的列数
				row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

                if(_verbose)
				    printf("query %lu column\n", row);
    
				for (int k = 0; field = mysql_fetch_field(res_ptr); k++)	//输出结果集中的字段名
				{
					printf("%s\t", field->name);
				}
				printf("\n");
			}

			for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
			{
				result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
				for (int j = 0; j < column; j++)
					printf("%s\t", result_row[j]);	//显示该行每一列信息
				printf("\n");
			}

			mysql_free_result(res_ptr);	//释放结果集
			mysql_close(&connection);	//查询完毕关闭连接

		}
	} else {
		fprintf(stderr, "mysql_real_connect failed!\n");
		if (mysql_error(&connection))	//显示错误原因
		{
			fprintf(stderr, "Connection error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
		}
	}

}

void comet_query_sql(const char *sql, std::vector < std::string > &result_store)	//只查询一个字段content
{
	MYSQL connection;
	int result;
	MYSQL_RES *res_ptr;
	MYSQL_ROW result_row;
	int column, row;

	mysql_init(&connection);

	if (mysql_real_connect
	    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
	     CLIENT_FOUND_ROWS)) {
		mysql_query(&connection, "set names utf8");

		result = mysql_query(&connection, sql);

		if (result)	//返回非0，失败
		{
			fprintf(stderr, "mysql query failed!\n");
			fprintf(stderr, "Query error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
			mysql_close(&connection);	//关闭mysql连接
		} else {	//返回0，成功
			res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

			if (res_ptr) {
				column = mysql_num_fields(res_ptr);	//结果集中的列数
				row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

				for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
				{
					result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
					for (int j = 0; j < column; j++) {
						result_store.push_back(result_row[j]);	//将content存放到容器中
					}
				}
			}

			mysql_free_result(res_ptr);	//释放结果集
			mysql_close(&connection);	//查询完毕关闭连接

		}
	} else {
		fprintf(stderr, "mysql_real_connect failed!\n");
		if (mysql_error(&connection))	//显示错误原因
		{
			fprintf(stderr, "Connection error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
		}
	}

}


void comet_query_seqmax_sql(int channel,int &seqmax)    //查询指定通道内消息序列号的最大值
{
	MYSQL connection;
	int result;
	MYSQL_RES *res_ptr;
	MYSQL_ROW result_row;
	int column, row;
    char sql[255]="";
    char* str_seqmax=NULL;

    sprintf(sql,"select MAX(seq) from message where channel=%d;",channel);  //查询指定通道内的最大消息序列号

	mysql_init(&connection);

	if (mysql_real_connect
	    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
	     CLIENT_FOUND_ROWS)) {
		mysql_query(&connection, "set names utf8");

		result = mysql_query(&connection, sql);

		if (result)	//返回非0，失败
		{
			fprintf(stderr, "mysql query failed!\n");
			fprintf(stderr, "Query error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
			mysql_close(&connection);	//关闭mysql连接
		} else {	//返回0，成功
			res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

			if (res_ptr) {
				column = mysql_num_fields(res_ptr);	//结果集中的列数
				row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

				for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
				{
					result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
					for (int j = 0; j < column; j++) {
					    //printf("%s\t", result_row[j]);	//显示该行每一列信息
                        str_seqmax=result_row[j];
                        if(str_seqmax!=NULL)
                        {
                            seqmax=boost::lexical_cast<int>(str_seqmax);
                            printf("%d\n",seqmax);
                        }
					}
				}
			}

			mysql_free_result(res_ptr);	//释放结果集
			mysql_close(&connection);	//查询完毕关闭连接

		}
	} else {
		fprintf(stderr, "mysql_real_connect failed!\n");
		if (mysql_error(&connection))	//显示错误原因
		{
			fprintf(stderr, "Connection error %d: %s\n",
				mysql_errno(&connection),
				mysql_error(&connection));
		}
	}

}
