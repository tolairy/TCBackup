all:api.o 
api.o:api.c
#	cp lib/libbcs_sdk.so /lib
	g++ -O2 -g -c api.c

#clean_cloud:clean_cloud.c api.c
#	g++ -g -o clean_cloud  clean_cloud.c api.c -lbcs_sdk -L./lib -Wl,-rpath ./lib -lcrypto -lcurl
