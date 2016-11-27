main: main.cc
	g++ -Werror -Wall -pthread main.cc -o main
.PHONY: run
run: main
	./main
