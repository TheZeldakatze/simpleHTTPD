/*
 * filetypes.c
 *
 *  Created on: 17.06.2022
 *      Author: victor
 */

#include "filetypes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* store the file types as a b-tree */
struct node {
	struct node* leftChild;
	struct node* rightChild;
	char* fileSuffix;
	char* mimeType;
};

struct node* mimeTreeRoot;

static struct node* createNode(char* mimeType, char* fileSuffix) {
	/* allocate memory */
	struct node* newNode = malloc(sizeof(struct node));

	/* apply the values */
	newNode->leftChild = NULL;
	newNode->rightChild = NULL;
	newNode->mimeType = mimeType;
	newNode->fileSuffix = fileSuffix;

	return newNode;
}

static void insertMimeType(char* mimeType, char* fileSuffix) {

	/* if the root is NULL, then create a new node */
	if(mimeTreeRoot == NULL) {
		mimeTreeRoot = createNode(mimeType, fileSuffix);
		return;
	}

	/* otherwise look for the first free good spot */
	struct node* currentNode = mimeTreeRoot;

	while(1) {
		/* compare the node and the file suffix */
		int cmp = strcasecmp(fileSuffix, currentNode->fileSuffix);

		/* they should never match */
		if(cmp == 0) {
			printf("WARNING: file type %s with suffix %s is already in the file type tree, skipping!\n",
					mimeType, fileSuffix);
			return;
		} else if(cmp < 0) {
			/* left */
			if(currentNode->leftChild == NULL) {
				/* create a new node */
				currentNode->leftChild = createNode(mimeType, fileSuffix);
				return;
			} else {
				currentNode = currentNode->leftChild;
			}
		} else {
			/* right */
			if(currentNode->rightChild == NULL) {
				/* create a new node */
				currentNode->rightChild = createNode(mimeType, fileSuffix);
				return;
			} else {
				currentNode = currentNode->rightChild;
			}
		}

	}
}

char* getFileType(char* suffix) {
	if(mimeTreeRoot != NULL && strlen(suffix) > 0) {
		struct node* currentNode = mimeTreeRoot;

		while(currentNode != NULL) {
			int cmp = strcasecmp(suffix, currentNode->fileSuffix);

			if(cmp == 0) {
				/* match */
				return currentNode->mimeType;
			} else if(cmp < 0) {
				/* left */
				currentNode = currentNode->leftChild;
			} else {
				currentNode = currentNode->rightChild;
			}
		}
	}

	/* TODO: return a failsafe */
	return "application/octet-stream";
}

void initFiletypeDatabase() {
	FILE* filetypeFile = fopen("filetypes.txt", "r");

	/* read each line */
	char buf[1024];
	unsigned char state = 0;
	char* currentFiletype;

	int mimeTypeCount = 0, fileExtensionCount = 0;
	while(feof(filetypeFile) == 0) {
		fgets(buf, sizeof(buf), filetypeFile);
		int buflen = strlen(buf);

		/* just be safe */
		if(strlen < 1)
			continue;

		/* remove the newline */
		if(buf[buflen-1] == '\n') {
			buf[buflen-1] = '\0';
			buflen--;
		}

		if(state == 0) { /* seek next filetype */
			if(buflen != 0) { /* if empty: ignore line */
				/* allocate space for a new file type */
				currentFiletype = malloc(buflen);
				strcpy(currentFiletype, buf);
				state = 1;
				mimeTypeCount++;
			}
		} else {
			if(buflen < 1) /* an empty line marks a new file type block */
				state = 0;
			else {
				char* newExt = malloc(buflen);
				strcpy(newExt, buf);
				insertMimeType(currentFiletype, newExt);
				fileExtensionCount++;
			}
		}
	}

	fclose(filetypeFile);

	/* print a result message */
	printf("Initialized the file type database with %d mime types and %d file extensions!\n",
			mimeTypeCount, fileExtensionCount);
}
