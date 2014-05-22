CC = g++
CFLAGS = -Wall -O -g -O2 
TARGET = TCBackup_client 
LIBS = -lpthread -lcrypt -lcrypto -lcurl -lglib-2.0

subdirs = ./cloud ./lib ./src


SRCS = client.c global.c
OBJS = client.o global.o

ERR = 1 

%.o: %.c
	$(CC) $(CFLAGS) -c $<


all: 
	@for I in ${subdirs}; \
		do echo "=== Enter $$I ===";  $(MAKE) -C $$I || exit 1; done
	
	$(CC) -O2 -o $(TARGET) $(SRCS) -L./lib -L./src -L./cloud -L./cloud/baidu/lib -Wl,-rpath ./cloud/baidu/lib -lsrc -llib -lcloud -lbcs_sdk $(LIBS)
	@chmod a+x $(TARGET) 
	@echo "=== Make of $(TARGET) is good==="
	@echo " "

clean_cloud:clean_cloud.c cloud/baidu/api.c
	g++ -g -O2  -o clean_cloud  clean_cloud.c cloud/baidu/api.c  -L./cloud/baidu/lib -L./cloud/json/lib -Wl,-rpath,./cloud/json/lib:./cloud/baidu/lib -ljson -lbcs_sdk -lcrypto -lcurl

clean:
	@rm -f *.o
	@rm -f lib/*.o
	@rm -f lib/*.a
	@rm -f src/*.o
	@rm -f src/*.a
	@rm -f cloud/*.o
	@rm -f cloud/*.a
	@rm -f cloud/baidu/*.o
	@rm -f $(TARGET)
