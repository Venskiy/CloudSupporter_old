build:
	gcc -c -Wall -Werror -fpic src/google.c -o obj/google.o
	gcc -shared -o out/libgoogle.so obj/google.o

clean:
	rm -rf obj/google.o out/libgoogle.so