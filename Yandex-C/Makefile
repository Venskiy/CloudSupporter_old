build:
			gcc -c -Wall -Werror -I include -fpic src/yandex.c -o obj/yandex.o
			gcc -shared -o out/libyandex.so obj/yandex.o

clean:
			rm -rf obj/yandex.o out/libyandex.so
