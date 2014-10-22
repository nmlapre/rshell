COMPILER = g++
FLAGS = -Wall -Werror -ansi -pedantic

all: rshell

rshell: 	
	[ -e ./bin ] || mkdir bin
	$(COMPILER) $(FLAGS) ./src/rshell.cpp -o ./bin/rshell

clean:
	rm -rf bin
