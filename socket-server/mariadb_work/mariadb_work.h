#include <stdio.h>
#include <stdlib.h>
#include "/usr/include/mariadb/mysql.h"

struct connection_details {
    char *server;
    char *user;
    char *password;
    char *database;
};

class mariadb {
    public:
        void mysql_connection_setup(void);
        void mysql_perform_query(MYSQL *connection, char *sql_query);
        MYSQL *conn;
        MYSQL_RES *res;
        MYSQL_ROW row;
        connection_details mysqlID;
};