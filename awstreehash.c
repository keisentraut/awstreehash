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

// AWS uses a fixed CHUNKSIZE of 1 MiB
const size_t CHUNKSIZE = 1024*1024;

// linked list which is used to store the SHA256 hashes of the intermediate chunks
struct Chunk {
	void *digest;
	struct Chunk *next;
};


void printBinaryAsHex(const unsigned char* s, size_t len) {
	for (size_t i=0; i<len; ++i)
		printf("%02x", s[i]);
}

// calloc, but with hard program exit on failure
void* safeCalloc(size_t n, size_t size) {
	void *p = calloc(n, size);
	if (p == NULL) {
		fputs("out of memory", stderr);
		exit(EXIT_FAILURE);
	}
	return p;
}

struct Chunk *allocAndInitChunk() {
	struct Chunk *c = safeCalloc(1, sizeof(struct Chunk));
	c->digest = safeCalloc(1, SHA256_DIGEST_LENGTH);
	c->next = NULL;
	return c;
}

struct Chunk *freeChunk(struct Chunk* c) {
	if (c == NULL) {
		return NULL;
	}
	struct Chunk *next = c->next;
	if (c->digest != NULL) {
		free(c->digest);
	}
	free(c);
	// for convenience, return pointer to the next chunk
	return next;
}

void* awsTreeHash(FILE* input)
{
	struct Chunk *startChunk = allocAndInitChunk();
	struct Chunk *previousChunk = NULL;
	struct Chunk *currentChunk = startChunk;
	void *buffer = safeCalloc(1, CHUNKSIZE);
	void *result = NULL; // return value

	size_t alreadyRead = 0;
	int n;
	SHA256_CTX ctx;

	SHA256_Init(&ctx);
	// read input once and create linked list of intermediate chunk hashes
	while( (n = fread(buffer, 1, CHUNKSIZE - alreadyRead, input)) != 0 ) {
		if (ferror(input)) {
			// some kind of read error.
			perror("error");

			// cleanup
			free(buffer);
			while ( (startChunk = freeChunk(startChunk)) != NULL) { };

			return result;
		}
		alreadyRead += n;
		SHA256_Update(&ctx, buffer, n);
		if (feof(input)) {
			break;
		}
		if (alreadyRead == CHUNKSIZE) {
			// current chunk is finished
			SHA256_Final(currentChunk->digest, &ctx);
			SHA256_Init(&ctx);
			alreadyRead = 0;
			// advance in linked list
			previousChunk = currentChunk;
			currentChunk->next = allocAndInitChunk();
			currentChunk = currentChunk->next;
		}
	};
	free(buffer);
	SHA256_Final(currentChunk->digest, &ctx);

	// we have to consider one special case
	// if we read data with a length of exactly n * 1 MiB, the last chunk hash
	// is the hash of an empty input and must be removed from list of chunks.
	// it doesn't matter there was no data at all, though
	if (startChunk != currentChunk && alreadyRead == 0) {
		freeChunk(currentChunk);
		previousChunk->next = NULL;
	}

	while (startChunk->next != NULL) {
		// The chunk structs itself are reused for the new chain.
		// we need two pointers:
		// - currentOld points to the chunk in the "old" chain, advances 2 chunks per iteration
		// - cuurentNew points to the chunk in the "new" chain, advances only one chunk per iteration
		struct Chunk* currentOld = startChunk;
		struct Chunk* currentNew = startChunk;

		// iterate through linked list
		while (1) {
			if (currentOld->next == NULL) {
				// only one chunk left, we need to copy this last one
				memcpy(currentNew->digest, currentOld->digest, SHA256_DIGEST_LENGTH);
				currentOld = currentOld->next;
			} else {
				// at least two chunks left, hash the next two into a new one
				SHA256_Init(&ctx);
				SHA256_Update(&ctx, currentOld->digest, SHA256_DIGEST_LENGTH);
				currentOld = currentOld->next;
				SHA256_Update(&ctx, currentOld->digest, SHA256_DIGEST_LENGTH);
				currentOld = currentOld->next;
				SHA256_Final(currentNew->digest, &ctx);
			}
			if (currentOld != NULL) {
				// there are more than zero old chunk hashes left, we need another iteration
				currentNew = currentNew->next;
			} else {
				// free chunk structs at the end which are not needed anymore
				// because the linked list got shorter
				currentOld = currentNew->next;
				while ( currentOld != NULL) { currentOld = freeChunk(currentOld); };
				currentNew->next = NULL;
				break;
			}
		};

		// we don't need the remaining chunk structs anymore, free them!
		currentOld = currentNew->next;	
		while ((currentOld = freeChunk(currentOld)) != NULL) { /* nop */ };
		currentNew->next = NULL;
	};

	// cleanup last chunk struct and return result
	result = safeCalloc(1, SHA256_DIGEST_LENGTH);
	memcpy(result, startChunk->digest, SHA256_DIGEST_LENGTH);
	freeChunk(startChunk);
	return result;
}

void hashStdin()
{
	void *result = awsTreeHash(stdin);
	if (result == NULL) {
		fputs("error: while reading from stdin", stderr);
	} else {
		printBinaryAsHex(result, SHA256_DIGEST_LENGTH);
		puts("  -");
		free(result);
	}
}

void hashFile(const char *filename)
{
	FILE* fp = NULL;

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "error: while opening file %s\n", filename);
		return;
	}

	void *result = awsTreeHash(fp);
	fclose(fp);

	if (result == NULL) {
		fprintf(stderr, "error: while reading from file %s\n", filename);
	} else {
		printBinaryAsHex(result, SHA256_DIGEST_LENGTH);
		printf("  %s\n", filename);
		free(result);
	}
}


int main(int argc, char* argv[]) {
	int stdinDone = 0;
	if (argc > 1) {
		// iterate through command line arguments
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
	} else if (argc == 1) { // no command line arguments -> hash from stdin
		hashStdin();
		stdinDone = 1;
	}
	exit(EXIT_SUCCESS);
}
