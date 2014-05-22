SRCS=db.c mysql.c mysql_find.c mysql_delete.c

OBJS=$(SRCS:.c=.o)

%.o:%.c
	@echo "compling $<"
	@g++ -g -c  $<


all:libdatabase.a
	@echo "=== Make of database is good==="
	@echo " "

libdatabase.a:$(OBJS)
	@echo "Making $@"
	@ar rc $@ $^

