/*
 * client.h
 *
 *  Created on: 6.24, 2013
 *      Author: Yifan Yang
 */

#ifndef CLIENT_H_
#define CLIENT_H_

/* server port */
#define SERVER_PORT 8888
#define SERVER_OTH_PORT 8889

/* define finger length */
#define FINGER_LENGTH 20

/* define the length of chunk address */
#define CHUNK_ADDRESS_LENGTH 64

/* define the length of filename */
#define FILE_NAME_LEN 256


/* define the length of username */
#define USERNAME_LENGTH 50

/* define the length of password */
#define PASSWORD_LENGTH 50


#define SUCCESS 1
#define FAILURE 0
#define ERROR -100

#define FILE_HASH 1
#define FILE_RECIPE 2

#define HASH_END -1
#define RECIPE_END -2

#define FILE_END -5
#define STREAM_END -6

#define SYSTEM_QUIT -7

/* define the type of fingerprint */
typedef unsigned char Fingerprint[FINGER_LENGTH];

/* define the type of chunk address */
typedef char Chunkaddress[CHUNK_ADDRESS_LENGTH]; 


typedef struct Client{
	char username[USERNAME_LENGTH];
    char password[PASSWORD_LENGTH];
	/*used to send file hashs and recv results in pipeline mode*/
	int fd;
	/*used to update recipe in pipeline mode*/
	int recipe_fd;
    JCR *jcr;
}Client;

/* functions */
void test_data(int socket,char *filename);
void usage(char *program);
Client *create_client();
void free_client(Client *c);


#endif 
/* CLIENT_H_ */
