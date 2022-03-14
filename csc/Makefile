makefile: ./code/main.c ./code/fibo.c
	gcc -c -I ./include ./code/main.c 
	gcc -c -I ./include ./code/fibo.c
	mv fibo.o ./code/fibo.o
	mv main.o ./code/main.o
	gcc -o fibo ./code/main.o ./code/fibo.o
clean:
	rm ./code/fibo.o
	rm ./code/main.o
