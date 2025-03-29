OBJS	= c_src/myz.o c_src/myz_node_list.o c_src/myz_func.o
SOURCE	= c_src/myz.c c_src/myz_node_list.c c_src/myz_func.c
HEADER	= include/myz.h include/myz_node_list.h include/myz_func.h
CC		= gcc
FLAGS	= -g -c -I include

all: $(OBJS)
	$(CC) -g $(OBJS) -o myz

myz.o: c_src/myz.c
	$(CC) $(FLAGS) c_src/myz.c

myz_node_list.o: c_src/myz_node_list.c
	$(CC) $(FLAGS) c_src/myz_node_list.c

myz_func.o: c_src/myz_func.c
	$(CC) $(FLAGS) c_src/myz_func.c

valgrind: $(OBJS)
	$(CC) -g $(OBJS) -o myz
	valgrind /home/users/sdi2200169/Documents/OS/project4/myz -c 123.myz 
	valgrind /home/users/sdi2200169/Documents/OS/project4/myz -m 123.myz
	valgrind /home/users/sdi2200169/Documents/OS/project4/myz -x 123.myz

clean:
	rm -f $(OBJS) myz

run: 

#	./myz -j 123.myz 
	./myz -c 123.myz example
#	./myz -a 123.myz 
	./myz -m 123.myz 
	./myz -q 123.myz 
	./myz -p 123.myz
	./myz -x 123.myz 
