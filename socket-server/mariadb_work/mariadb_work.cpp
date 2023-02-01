#include "mariadb_work.h"

void mariadb::mysql_connection_setup()
{
    MYSQL *connection = mysql_init(NULL);
 
    if(!mysql_real_connect(connection, mysqlID.server, mysqlID.user, mysqlID.password, mysqlID.database, 0, NULL, 0)) {
 
        printf("Connection error : %s\n", mysql_error(connection));
        exit(1);
 
    }
    conn = connection;
}

void mariadb::mysql_perform_query(MYSQL *connection, char *sql_query)
{
    if(mysql_query(connection, sql_query)) {
 
        printf("MYSQL query error : %s\n", mysql_error(connection));
        exit(1);
 
    }
    res = mysql_use_result(connection);
}