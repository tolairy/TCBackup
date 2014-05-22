LIBSRCS=adlist.c queue.c threadq.c zmalloc.c bloom.c bnet.c htable.c signal.c

LIBOBJS=$(LIBSRCS:.c=.o)

%.o:%.c
	@echo "compling $<"
	@g++ -g -c  $<


all:liblib.a
	@echo "=== Make of lib is good==="
	@echo " "

liblib.a:$(LIBOBJS)
	@echo "Making $@"
	@ar rc $@ $^

