COMPILER = g++
FLAGS = -Wall -Werror -ansi -pedantic

all:
	[ -e ./bin ] || mkdir bin

rshell: 
	$(COMPILER) $(FLAGS) ./src/rshell.cpp -o ./bin/rshell

clean:
	rm -rf bin
