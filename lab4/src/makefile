CC=gcc
CFLAGS=-I.
TARGETS=parallel_min_max process_memory zombie parallel_sum

all : $(TARGETS)

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

parallel_sum : utils.o utils.h
	$(CC) -o psum utils.o parallel_sum.c $(CFLAGS)	

process_memory : 	
	$(CC) -o process_memory process_memory.c $(CFLAGS)

zombie :
	$(CC) -o zombie zombie.c $(CFLAGS)	

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean :
	rm  $(TARGETS)

.PHONY : all clean
