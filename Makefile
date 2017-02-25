awstreehash: awstreehash.c clean
	gcc -O3 -pedantic -Wall -o awstreehash awstreehash.c -lcrypto
	strip awstreehash

debug: awstreehash.c clean
	gcc -O0 -g -pedantic -Wall -fsanitize=address -fsanitize=undefined -o awstreehash awstreehash.c -lcrypto

clean:
	rm -f awstreehash

