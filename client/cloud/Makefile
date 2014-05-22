#CXX = /usr/bin/g++
#AR = /usr/bin/ar
#RANLIB = ranlib

#CLOUDSRCS=cloud.c baidu/api.c

#CLOUDOBJS=cloud.o baidu/api.o

#%.o:%.c
#@echo "Compiling $<"
#	$(CXX) -c $<

all:libcloud.a
	@echo "===Make of Cloud is good==="
	@echo " "
libcloud.a:cloud.o baidu/api.o
	ar rc libcloud.a cloud.o baidu/api.o

cloud.o:cloud.c
	g++ -O2 -g -c cloud.c

baidu/api.o:baidu/api.c
	cd ./baidu && make

#libcloud.a:cloud.o baidu/api.o
#		@echo "Making $@ ..."
#		$(AR) rc  $@ $^
#		$(RANLIB) $@



