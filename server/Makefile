CC = g++
CFLAGS = -Wall -O -g
TARGET = TCBackup_server 
LIBS = -lpthread -lcrypt -lcrypto -lcurl -lmysqlclient

subdirs = ./database ./lib ./src


SRCS = server.c global.c
OBJS = server.o global.o


%.o: %.c
	$(CC) $(CFLAGS) -c $<

all:
	@for I in ${subdirs}; \
			do echo "=== Enter $$I ===";  $(MAKE) -C $$I || exit 1; done	

	$(CC) -o $(TARGET) $(SRCS) -L/usr/lib/x86_64-linux-gnu  -L./src -L./lib -L./database -lsrc -ldatabase -llib $(LIBS)
	@chmod a+x $(TARGET)
	@echo "=== Make of $(TARGET) is good==="
	@echo " "


clean:
	@rm -f *.o
	@rm -f lib/*.o
	@rm -f src/*.o
	@rm -f database/*.o
	@rm -f $(TARGET)
