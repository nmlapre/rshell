bindir=./bin
opt=-Wall -Werror -ansi -pedantic
rshellout=-o ./bin/rshell
rshellsrc=./src/main.cpp
cpout = ./bin/cp
cpsrc = ./src/cp.cpp
lsout=-o ./bin/ls
lssrc=./src/ls.cpp
rmb=rm -rf ./bin

all:
	if [ -d "$(bindir)" ]; then  $(rmb); fi
	mkdir ./bin
	g++ $(opt) $(rshellsrc) $(rshellout)
	g++ $(opt) $(lssrc) $(lsout)
	g++ $(opt) $(cpsrc) $(cpout)

rshell:
	mkdir ./bin
	g++ $(opt) $(rshellsrc) $(rshellout)

ls:
	mkdir ./bin
	g++ $(opt) $(lssrc) $(lsout)
cp:
	mkdir ./bin
	g++ $(opt) $(cpsrc) $(cpout)
clean:
	$(rmb)
