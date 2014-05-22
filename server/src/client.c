#include "../global.h"

/* create a struct for client to keep user information */
DedupClient *create_client(int fd)
{
    DedupClient *c = NULL;
    c = (DedupClient *)malloc(sizeof(DedupClient));
    if(c == NULL)
        return NULL;
    memset(c->username, 0, sizeof(c->username));
    memcpy(c->username, "yangyf", strlen("yangyf"));
    memset(c->password, 0, sizeof(c->password));
    c->user_id = 11;
    /* tmp create here, if the login function is used, if can be moved */
    c->db = db_init_database();
    if(c->db == NULL){
        err_msg1("db_init_failed!");
        return NULL;
    }
    if(db_open_database(c->db) != 1) {
        err_msg1("db_open_failed!");
        return NULL;
    }
    //c->db = NULL;	
    c->client_fd = fd;

/*    if(server.max_clients && listLength(server.clients) > server.max_clients) {
        printf("max number of clients reached!\n");
        free_client(c);
        return NULL;
    }
*/
    pthread_mutex_lock(&server->server_mutex);
    listAddNodeTail(server->clients, c);
    server->stat_num_connections++;
    printf("%s,%d:new client, totoal client num is %d\n", __FILE__, __LINE__, server->stat_num_connections);
    pthread_mutex_unlock(&server->server_mutex);

    return c;
}

/* free client */
void free_client(DedupClient *c)
{
    listNode *ln;
    /* Remove from the list of clients */
    if(strlen(c->username) != 0){
        ln = listSearchKey(server->clients, c->username);
        if(ln == NULL)
            return;
        listDelNode(server->clients, ln);
    }
    if(c->db != NULL)
        db_close_database(c->db);
    pthread_mutex_lock(&server->server_mutex);
    server->stat_num_connections--;
    printf("%s,%d:free client, total client num is %d\n", __FILE__, __LINE__, server->stat_num_connections);
    pthread_mutex_unlock(&server->server_mutex);

    free(c);
}

/* match client,used for search user in the client list */
int client_match(void *ptr, void *key)
{
    DedupClient *c;
    c = (DedupClient *)ptr;
    const char *ClientName  = (const char *)key;

    return !strcmp(c->username, ClientName); 
}
