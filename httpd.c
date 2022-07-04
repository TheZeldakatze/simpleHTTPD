/*
 * httpd.c
 *
 *  Created on: 04.06.2022
 *      Author: victor
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "specialpages.h"
#include "filetypes.h"
#include "httpd.h"

#ifdef ENABLE_LUA_SCRIPTING
#include "lua_scripting.h"
#endif

int port = 2001;

char* httpRootDir;
int httpRootDirLen;

/* the listening point */
int socketfd;

void* handleConnection(void *fdptr) {
	/* the address that is pointed to here is the actual number */
	int fd = ((uintptr_t) fdptr);

	/* set a recive timeout */
	struct timeval reciveTimeout;
	reciveTimeout.tv_sec = 5;
	reciveTimeout.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &reciveTimeout, sizeof(reciveTimeout));

	/* get the request */
	char requestBuffer[65536];
	int requestStatus = recv(fd, requestBuffer, 65535, 0);

	/* parse the request */
	if(errno == EAGAIN || errno == EWOULDBLOCK) {
		/* set a send timeout */
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &reciveTimeout, sizeof(reciveTimeout));

		/* send the 408 page */
		dprintf(fd, PAGE_408_HEAD, PAGE_408_BODY_SIZE);
		dprintf(fd, PAGE_408_BODY);

	} else if(requestStatus < 0) {
		/* TODO: error handling */
	} else if(requestStatus == 0) {
		/* don't answer empty requests */
	} else if(memcmp(&requestBuffer, &"GET", 3) || 1) {
		char* filename = strtok((char*) (requestBuffer + 4), " ");
		char* parameters = strchr(requestBuffer, '?');

		if(filename == NULL || strlen(filename) < 2) {
			filename = (char*) DEFAULT_FILE;
		} else {
			filename++;
		}

		if(parameters != NULL) {

			parameters[0] = '\0'; // create a split at that point
			parameters++; // advance past the (former) question mark
		} else {
			parameters = alloca(sizeof(char));
			parameters[0] = '\0';
		}

		/* construct the path */
		char path[strlen(filename) + httpRootDirLen + 1];
		strcpy(path, httpRootDir);
		strcat(path, filename);
		filename = path;


		/* resolve the full path */
		unsigned char forbidden = 0;

		/* check if the path is in the httpd root dir */
		if(strstr(filename, "..") != NULL) {
			forbidden = 1;
		}

		/*
		 * read the file
		 * if the file does not exist, send a 404
		 */
		if(forbidden) {
			dprintf(fd, PAGE_403_HEAD, PAGE_403_BODY_SIZE);
			dprintf(fd, PAGE_403_BODY);
		} else if(access(filename, R_OK) != 0) {
			dprintf(fd, PAGE_404_HEAD, PAGE_404_BODY_SIZE);
			dprintf(fd, PAGE_404_BODY);
		} else {

			/* get the mime type */
			char* suffix = strrchr(filename, '.');
			if(suffix == NULL)
				suffix = "";
			else
				suffix++; /* remove the dot */
			char* mime = getFileType(suffix);
#ifdef ENABLE_LUA_SCRIPTING
			if(strcmp(mime, "serverscript/lua") == 0) {
				runLuaScript(fd, filename, parameters);
			} else {
#endif

			FILE *file;
			file = fopen(filename, "r");

			/* get the file length */
			fseek(file, 0 , SEEK_END);
			int dataSize = ftell(file);
			rewind(file);

			// print an ok response
			dprintf(fd, "HTTP/1.0 200 OK\r\n");
			dprintf(fd, "Content-Type: %s\r\n", mime);
			dprintf(fd, "Content-Length: %d\r\n\r\n", dataSize);

			// send the site
			int fileReadCount;
			char buf[1024];
			while((fileReadCount = fread(buf, sizeof(char), sizeof(buf), file))) {
				write(fd, buf, fileReadCount);
			}

			/* close the file */
			fclose(file);
#ifdef ENABLE_LUA_SCRIPTING
			}
#endif
		}
	} else {
		/* Post is currently unsupported */
		dprintf(fd, "POST requests are currently unsupported!");
	}

	close(fd);
	return 0;
}

void quit_handler(int signal) {
	close(socketfd);
	printf("Closed socket. Quitting!\n");
	exit(0);
}

int main(int argv, char* argc[]) {
	struct sockaddr_in servaddr;

	// map the shutdown handler
	signal(SIGINT, quit_handler);

	// initialize the socket
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd == -1) {
		perror("Couldn't create socket!\n");
		exit(1);
	}

	// bind the server
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	if ((bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		perror("Can't bind address!\n");
		exit(1);
	}

	// now actually listen
	if ((listen(socketfd, 1024)) != 0) {
		perror("Can't listen to the socket!\n");
		exit(1);
	}

	// check for the htdocs dir and initialize
	httpRootDir = HTDOCS_ROOT;
	httpRootDirLen = strlen(httpRootDir);

	// append a PATH_DELIMITER at the end if there isn't one
	if(httpRootDir[httpRootDirLen-1] != PATH_DELIMITER) {
		char* oldPath = httpRootDir;
		httpRootDir = malloc(httpRootDirLen + 2);
		strcpy(httpRootDir, oldPath);
		httpRootDir[httpRootDirLen] = PATH_DELIMITER;
		httpRootDir[httpRootDirLen + 1] = '\0';
	}

	// make sure the folder exists and all
	printf("The htdocs folder is %s", httpRootDir);
	if(access(httpRootDir, R_OK) != 0) {
		printf(", the folder does not exist or has wrong permissions, will try to create it!");
		if(mkdir(httpRootDir, 0777)) {
			/* if the creation fails, exit the server */
			printf("Could not create the htdocs directory! Exiting...\n");
			quit_handler(7);
		} else {
			struct stat statb;
			stat(httpRootDir, &statb);
			if(S_ISDIR(statb.st_mode) == 0) {
				printf("htdocs is not a directory! Exiting...\n");
				quit_handler(7);
			}
		}
	}
	printf("\n");

	// initialize the filetype database
	initFiletypeDatabase();

	// set some memory stuff for pthreads
	pthread_attr_t pthread_attr;
	pthread_attr_init(&pthread_attr);

#ifdef ENABLE_LUA_SCRIPTING
	printf("Initializing the lua script environment\n");
	init_lua_script_system();
#endif

	printf("Listening on Port: %d\n", port);

	// wait for incoming connections
	while(1) {
		struct sockaddr_in clientaddr;
		socklen_t clientaddrLen = sizeof(clientaddr);
		int client = accept(socketfd, (struct sockaddr *) &clientaddr, ((socklen_t*) &clientaddrLen));

		// start a handler for the client
		pthread_t thread;
		pthread_create(&thread, NULL, handleConnection, (void*)(uintptr_t) client);
		pthread_detach(thread);
	}


	// close the socket
	close(socketfd);
	printf("Closed socket. Quitting!\n");

	return 0;
}
