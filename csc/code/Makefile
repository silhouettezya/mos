all: main.c fibo.c 
	gcc -c -I ../include main.c
	gcc -c -I ../include fibo.c
	gcc -O3 main.o fibo.o -o fibo
	mv fibo ../fibo
