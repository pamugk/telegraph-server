all: telegraph-server

telegraph-server: main.o dblayer.o
	gcc obj/main.o obj/dblayer.o -lpq -o debug/telegraph-server.exe

main.o: src/main.c
	gcc -c src/main.c -o ./obj/main.o

dblayer.o: src/dblayer.c
	gcc -c src/dblayer.c -o ./obj/dblayer.o

clean:
	rm -rf *.o *.gch