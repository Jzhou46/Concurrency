all:
	gcc -pthread -g raceTest.c -lm -o raceTest

run:
	./raceTest
