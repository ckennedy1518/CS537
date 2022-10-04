CC = gcc
CFLAGS = -Wall -Wextra -Werror
DFLAGS = -g
DEPENDENCIES.C = read_ext2.c
EXEC = runscan
MAIN.C = runscan.c

default: main

clean:
	rm -f $(EXEC)
	rm -rf ./output/*
	rm -rf ./output*/

main: $(MAIN.C)
	$(CC) $(CFLAGS) $(DFLAGS) $(MAIN.C) $(DEPENDENCIES.C) -o $(EXEC)

run:
	./runscan disk_images/image-01 output/

test:
	/p/course/cs537-swift/tests/p5/test-p5.sh -t $(num)
