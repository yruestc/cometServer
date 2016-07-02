/*
 * =====================================================================================
 *
 *       Filename:  cometserver_db.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月19日 15时59分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __COMETSERVER_DB_H
#define __COMETSERVER_DB_H

#include<string>
#include<vector>

void exe_sql(const char *sql);
void query_sql(const char *sql);
void comet_query_sql(const char* sql,std::vector<std::string> &result_store);    //只查询一个字段content
void comet_query_seqmax_sql(int channel,int &seqmax);

#endif
