build-test:
			gcc -L/home/ilya/CloudSupporter/Google-C/out -L/home/ilya/CloudSupporter/Yandex-C/out -L/home/ilya/CloudSupporter/src -Wall -o src/test.o src/test.c -lyandex -lgoogle -lcloud -lcurl -ljansson -std=c99 -ldropbox
build-cloud:
			gcc -c -Wall -Werror -fpic src/cloud.c -o src/cloud.o
			gcc -shared -o src/libcloud.so src/cloud.o
clean:
			rm -rf src/*.o src/*.so
