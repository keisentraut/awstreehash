/** awstreehash.c - Tool to calculate SHA256 Tree-Hash as used in AWS Glacier
 *
 * Copyright ©February 2017, Klaus Eisentraut
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <openssl/sha.h>

const size_t HASHLEN = SHA256_DIGEST_LENGTH;
const size_t BLOCKSIZE = 1024*1024;

void printBinAsHex(const unsigned char* s, size_t len) {
	for (size_t i=0; i<len; ++i)
		printf("%02x", s[i]);
}

void* safeCalloc(size_t n, size_t size) {
	void *p = calloc(n, size);
	if (p == NULL) {
		fputs("out of memory\n", stderr);
		exit(EXIT_FAILURE);
	}
	return p;
}

void hashStdin()
{
	fputs("hashStdin() is not yet implemented\n", stderr);
}

void hashFile(const char *filename)
{
	FILE* fp = NULL;
	unsigned char* buffer = NULL;
	unsigned char *chunks = NULL;
	size_t numberOfChunks = 0;

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "error: cannot open %s\n", filename);
		goto cleanup;
	}

	// find out size of file
	fseek(fp, 0L, SEEK_END);
	size_t n = ftell(fp);
	if (n == 0) {
		fprintf(stderr, "error: empty file %s\n", filename);
		goto cleanup;
	}
	rewind(fp);
	numberOfChunks = (n + BLOCKSIZE - 1 ) / BLOCKSIZE;

	// read file in blocks and calculate SHA256 sums of each chunk
	chunks = safeCalloc(numberOfChunks, HASHLEN);
	buffer = safeCalloc(1, BLOCKSIZE); 
	for (size_t i=0; i<n; i += BLOCKSIZE) {
		size_t remaining = n-i;
		size_t bytesToRead = (remaining >= BLOCKSIZE ? BLOCKSIZE : remaining);
		size_t blockNr = i/BLOCKSIZE;
		size_t read = fread(buffer, bytesToRead, 1, fp);
		if (read == 1) {
			//printf("pointer %p\n", chunks + i*HASHLEN);
			SHA256_CTX ctx;
			SHA256_Init(&ctx);		
			SHA256_Update(&ctx, buffer, bytesToRead);
			SHA256_Final(chunks + blockNr*HASHLEN, &ctx);
		} else {
			fprintf(stderr, "error: Error reading block starting at offset %lu of file %s.\n", i, filename);
			goto cleanup;
		}
	}

	// concat hashes
	while (numberOfChunks > 1) {
		// concat two hashes into one
		for (size_t i=0; i<numberOfChunks-1; i+=2) {
			SHA256_CTX ctx;
			SHA256_Init(&ctx);
			SHA256_Update(&ctx, chunks + i*HASHLEN, 2*HASHLEN);
			SHA256_Final(chunks + i/2*HASHLEN, &ctx);
		}
		if (numberOfChunks%2 == 1) {	
			// copy last block
			memcpy(chunks + (numberOfChunks/2)*HASHLEN, chunks + (numberOfChunks-1)*HASHLEN, HASHLEN);
			numberOfChunks+=1;
		}
		numberOfChunks/=2;
	};
	// output
	printBinAsHex(chunks, HASHLEN);
	printf("  %s\n", filename);

cleanup:
	if (chunks) free(chunks);
	if (buffer) free(buffer);
	if (fp) fclose(fp);
	numberOfChunks=0;
}

int main(int argc, char* argv[]) {
	int stdinDone = 0;
	if (argc > 1) {
		for (size_t i=1; i<argc; ++i) {
			if (strcmp(argv[i], "-") == 0) {
				if (stdinDone) {
					fputs("cannot read stdin twice, skipping!\n", stderr);
				}
				else {
					hashStdin();
					stdinDone = 1;
				}
			} 
			else {
				hashFile(argv[i]);
			}
		}
	} else if (argc == 1) {
		hashStdin();
		stdinDone = 1;
	}
	exit(EXIT_SUCCESS);
}
