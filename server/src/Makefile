SRCS= backup.c client.c dedupHandle.c index.c jcr.c recipe.c restore.c pipeline_backup.c

OBJS=$(SRCS:.c=.o)

%.o:%.c
	@echo "compling $<"
	@g++ -g -c  $<


all:libsrc.a
	@echo "=== Make of src is good==="
	@echo " "

libsrc.a:$(OBJS)
	@echo "Making $@"
	@ar rc $@ $^

