#ifndef __MYSQL_H_
#define __MYSQL_H_


#define DB_USER "root"

#define DB_PASSWORD ""

#define DB_NAME "TCBackup"

#define DB_PORT 0

#define DB_ADDRESS "127.0.0.1"


typedef struct dedupDb {
	MYSQL mysql;
	MYSQL *db;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int status;
	my_ulonglong numRows;
	char *db_name;
	char *db_user;
	char *db_password;
	char *db_address;
	char *db_socket;
	int db_port;
	bool connected;
	char *cmd;
	int64_t UsrId;
}DedupDb;

DedupDb * db_init_database();

int db_open_database(DedupDb *mdb);

void db_close_database(DedupDb *mdb);

#endif
