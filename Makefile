awstreehash: awstreehash.c
	gcc -O3 -pedantic -Wall -o awstreehash awstreehash.c -lcrypto
	strip awstreehash

clean:
	rm -f awstreehash

