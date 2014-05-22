LIBSRCS=bnet.c bnet.h dlist.c dlist.h htable.c htable.h libz.c queue.c rabin.c sha1.c zmalloc.c lru_cache.c

LIBOBJS=$(LIBSRCS:.c=.o)

%.o:%.c
	@echo "compling $<"
	g++ -O2 -g -c  $<


all:liblib.a
	@echo "=== Make of lib is good==="
	@echo " "

liblib.a:$(LIBOBJS)
	@echo "Making $@"
	@ar rc $@ $^

