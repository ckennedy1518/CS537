# C Compiler
CC=gcc

# -Wall: all warnings
# -Werror: warnings -> errors
# -O[x]: optimize to degree [x]
# -g: debug info for gdb
# -I. specifices dir
# -pthread: threads 
CFLAGS=-Wall -Werror -O3 -g -I. -pthread

# rule for .o files requiring an update of .c
#OBJS = main mapreduce hashmap
#$(OBJS): %: %.c
#	$(CC) -o $@ $^ $(CFLAGS)

#all: $(OBJS)

mapreduce: mapreduce.c mapreduce.h hashmap.c hashmap.h main.c
	$(CC) -o $@ main.c mapreduce.c hashmap.c $(CFLAGS)

clean:
	rm *.out