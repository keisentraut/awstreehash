awstreehash: awstreehash.c
	#gcc -O0 -g -pedantic -Wall -fsanitize=address -fsanitize=undefined -o awstreehash awstreehash.c -lcrypto
	gcc -O3 -pedantic -Wall -o awstreehash awstreehash.c -lcrypto
	strip awstreehash

clean:
	rm -f awstreehash

