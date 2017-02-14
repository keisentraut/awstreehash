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


const size_t BLOCKSIZE = 1024*1024;

// linked list which is used to store the SHA256 hashes of the intermediate chunks 
struct Chunk {
	void *digest;	
	struct Chunk *nextChunk;
};


void printBinaryAsHex(const unsigned char* s, size_t len) {
	for (size_t i=0; i<len; ++i)
		printf("%02x", s[i]);
}

// calloc, but with hard program exit on failure
void* safeCalloc(size_t n, size_t size) {
	void *p = calloc(n, size);
	if (p == NULL) {
		fputs("out of memory\n", stderr);
		exit(EXIT_FAILURE);
	}
	return p;
}

struct Chunk *allocAndInitChunk() {
	struct Chunk *c = safeCalloc(1, sizeof(struct Chunk));
	c->digest = safeCalloc(1, SHA256_DIGEST_LENGTH);
	c->nextChunk = NULL;
	return c;
}

struct Chunk *freeChunk(struct Chunk* c) {
	if (c == NULL) {
		return NULL;
	}
	struct Chunk *nextChunk = c->nextChunk;
	if (c->digest != NULL) {
		free(c->digest);
	}
	free(c);
	// for convenience, return pointer to the next chunk
	return nextChunk;
}

void printChunks(struct Chunk* startChunk) {
	puts("----- CURRENT CHUNK LIST -----");
	while (startChunk) {
		printBinaryAsHex(startChunk->digest, SHA256_DIGEST_LENGTH);
		puts("");
		startChunk = startChunk->nextChunk;
	}
	puts("------------------------------");
	puts("");
}

void* awsTreeHash(FILE* input)
{
	void *result = safeCalloc(1, SHA256_DIGEST_LENGTH);

	struct Chunk *startChunk = allocAndInitChunk();
	struct Chunk *previousChunk = NULL;
	struct Chunk *currentChunk = startChunk;
	void *buffer = safeCalloc(1, BLOCKSIZE); 

	size_t alreadyRead = 0;
	int n;
	SHA256_CTX ctx;

	SHA256_Init(&ctx);
	// read input once and create linked list of intermediate chunk hashes
	while( (n = fread(buffer, 1, BLOCKSIZE - alreadyRead, input)) != 0 ) {
		if (ferror(input)) {
			// some kind of read error.
			perror("error");
			free(result);
			return NULL;			
		}
		alreadyRead += n;
		SHA256_Update(&ctx, buffer, n);
		if (feof(input)) {
			break;
		}
		if (alreadyRead == BLOCKSIZE) {
			// current chunk is finished
			SHA256_Final(currentChunk->digest, &ctx);
			SHA256_Init(&ctx);
			alreadyRead = 0;
			// advance in linked list
			previousChunk = currentChunk;
			currentChunk->nextChunk = allocAndInitChunk();
			currentChunk = currentChunk->nextChunk;			
		}
	};
	free(buffer);
	SHA256_Final(currentChunk->digest, &ctx);

	// we have to consider one special case
	// if we read data with a length of exactly n * 1 MiB, the last chunk hash 
	// is the hash of an empty input and must be removed from list of chunks.
	// it doesn't matter for empty input, though
	if (startChunk != currentChunk && alreadyRead == 0) {
		freeChunk(currentChunk);
		previousChunk->nextChunk = NULL;
	}
			

	while (startChunk->nextChunk != NULL) { // concat chunks as long as there is only one left
		// start a new chain which will replace the old chain later on
		// old chain starts at "startChunk"
		// new chain starts at "startChunkNew"
		struct Chunk* currentChunkOld = startChunk;
		struct Chunk* startChunkNew   = allocAndInitChunk();
		struct Chunk* currentChunkNew = startChunkNew;

		while (currentChunkOld != NULL) { // iterate through old list
			if (currentChunkOld->nextChunk != NULL) {
				// two or more chunks left, hash two old ones into a new one
				SHA256_Init(&ctx);
				SHA256_Update(&ctx, currentChunkOld->digest, SHA256_DIGEST_LENGTH);
				currentChunkOld = freeChunk(currentChunkOld);
				SHA256_Update(&ctx, currentChunkOld->digest, SHA256_DIGEST_LENGTH);
				currentChunkOld = freeChunk(currentChunkOld);
				SHA256_Final(currentChunkNew->digest, &ctx);
				if (currentChunkOld == NULL) { 
					currentChunkNew->nextChunk = NULL;
				} else {
					// there is at least one more left, so we need another element in the new list
					currentChunkNew->nextChunk = allocAndInitChunk();
					currentChunkNew = currentChunkNew->nextChunk;
				}
			} else {
				// only one chunk left, we need to copy this last one
				memcpy(currentChunkNew->digest, currentChunkOld->digest, SHA256_DIGEST_LENGTH);
				currentChunkNew->nextChunk = NULL;
				currentChunkOld = freeChunk(currentChunkOld); // == NULL
			}
		};
		startChunk = startChunkNew;
	};

	memcpy(result, startChunk->digest, SHA256_DIGEST_LENGTH);
	freeChunk(startChunk);
	return result;
}

void hashStdin()
{
	void *result = awsTreeHash(stdin);
	if (result == NULL) {
		fprintf(stderr, "error: while reading from stdin");
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
		fprintf(stderr, "error: while reading from file %s", filename);
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
