#include "../global.h"


DedupDb * db_init_database() 
{
	DedupDb *mdb;

	mdb = (DedupDb *) malloc(sizeof(DedupDb));
	memset(mdb, 0, sizeof(mdb));

	mdb->db_name = (char *)malloc(strlen(DB_NAME)+1);
	memset(mdb->db_name, 0, strlen(DB_NAME)+1);
	memcpy(mdb->db_name, DB_NAME, strlen(DB_NAME));

	mdb->db_user = (char *)malloc(strlen(DB_USER)+1);
	memset(mdb->db_user, 0, strlen(DB_USER)+1);
	memcpy(mdb->db_user, DB_USER, strlen(DB_USER));

       if(DB_PASSWORD) {
	   	mdb->db_password = (char *)malloc(strlen(DB_PASSWORD)+1);
		memset(mdb->db_password, 0, strlen(DB_PASSWORD)+1);
	       memcpy(mdb->db_password, DB_PASSWORD, strlen(DB_PASSWORD));
	}else{
	       mdb->db_password = NULL;
	}
	mdb->db_address = (char *)malloc(strlen(DB_ADDRESS)+1);
	memset(mdb->db_address, 0, strlen(DB_ADDRESS)+1);
	memcpy(mdb->db_address, DB_ADDRESS, strlen(DB_ADDRESS));

	mdb->db_socket = NULL;

	mdb->db_port = DB_PORT;

	mdb->cmd = (char *)malloc(1024);
	memset(mdb->cmd, 0, 1024);
	mdb->connected = false;
	return mdb;
}

int db_open_database(DedupDb *mdb) 
{
	int errstat;
	int retry;

	if (mdb->connected) { 
		return 1;
	}
	mdb->connected = false;

	/* connect to the database */
    #ifdef HAVE_EMBEDDED_MYSQL
	    mysql_server_init(0, NULL, NULL);
    #endif
	mysql_init(&(mdb->mysql));

	/* If connection fails, try at 5 sec intervals for 30 seconds. */
	for (retry = 0; retry < 6; retry++) {
		mdb->db = mysql_real_connect(&(mdb->mysql), /* db */
		mdb->db_address,    /* default = localhost */
		mdb->db_user,       /*  login name */
		mdb->db_password,   /*  password */
		mdb->db_name,       /* database name */
		mdb->db_port,       /* default port */
		mdb->db_socket,     /* default = socket */
		CLIENT_FOUND_ROWS); /* flags */

		if (mdb->db != NULL) {
			break;
		}
	}
	if (mdb->db == NULL) {
		printf("Unable to connect to MySQL server. \n"
				"Database=%s User=%s\n"
				"It is probably not running or your password is incorrect.\n",
				mdb->db_name, mdb->db_user);
		return 0;
	}

    #ifdef HAVE_THREAD_SAFE_MYSQL
	    my_thread_init();
    #endif
	mdb->connected = true;
	return 1;
}


void db_close_database(DedupDb *mdb) 
{
	if (!mdb) {
		return;
	}
	
    #ifdef HAVE_THREAD_SAFE_MYSQL
	    my_thread_end();
    #endif
	
	if (mdb->cmd) {
		free(mdb->cmd);
	}
    
	if (mdb->db_name) {
		free(mdb->db_name);
	}
	if (mdb->db_user) {
		free(mdb->db_user);
	}
	if (mdb->db_password) {
		free(mdb->db_password);
	}
	if (mdb->db_address) {
		free(mdb->db_address);
	}
	if (mdb->db_socket) {
		free(mdb->db_socket);
	}
	free(mdb);
}















