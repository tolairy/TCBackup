SRCSRCS=recipe.c backup.c  chunkdedup.c   extreme_binning.c  filededup.c  handle.c  jcr.c  store.c container.c restore.c cfl.c pipeline_backup.c perfect_rewrite.c capping.c read_trace.c simulate.c overhead_test.c container_cache.c

SRCOBJS=$(SRCSRCS:.c=.o)
	
%.o:%.c
	@echo "compling $<"
	g++ -g -c $<
			
			
all:libsrc.a
	@echo "=== Make of src is good==="
	@echo " "
					
libsrc.a:$(SRCOBJS)
	@echo "Making $@"
	ar rc $@ $^
