/*
 * lua_scripting.c
 *
 *  Created on: 01.07.2022
 *      Author: victor
 */

#include "lua_scripting.h"

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* you probably have to change the header paths */
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <lua5.4/lauxlib.h>

struct lua_script_frame {
	pthread_t threadID;
	int fd;
	char* script;

	char* headerText;

	unsigned char headerWritten;

	struct lua_script_frame *next;
};

/**
 * the lua thread list stuff
 *
 * this is necessary for lua callback functions
 * so they can access the necessary data in a threaded context
 * */
static struct lua_script_frame *first_script_frame;
static sem_t lua_script_frame_list_mutex;


/* allocates and initializes the struct frame  and adds it to the list */
static struct lua_script_frame *create_lua_script_frame(int fd, char* script) {
	struct lua_script_frame *newFrame = malloc(sizeof(struct lua_script_frame));

	newFrame->threadID = pthread_self();
	newFrame->fd = fd;
	newFrame->script = script;
	newFrame->headerWritten = 0;

	/* add it to the list */
	sem_wait(&lua_script_frame_list_mutex);
	newFrame->next = first_script_frame;
	first_script_frame = newFrame;
	sem_post(&lua_script_frame_list_mutex);

	return newFrame;
}

static void destroy_lua_script_frame() {
	pthread_t threadID = pthread_self();


	sem_wait(&lua_script_frame_list_mutex);
	struct lua_script_frame *prevFrame = NULL, *ownFrame = NULL;
	while(ownFrame != NULL) {
		if(ownFrame->threadID == threadID) {
			break;
		} else {
			prevFrame = ownFrame;
			ownFrame = ownFrame->next;
		}
	}

	if(ownFrame != NULL && ownFrame->threadID == threadID) {
		prevFrame->next = ownFrame->next; /* cut it out of the list */
		free(ownFrame);
	}

	sem_post(&lua_script_frame_list_mutex);
}

struct lua_script_frame *get_lua_script_frame() {
	pthread_t threadID = pthread_self();

	sem_wait(&lua_script_frame_list_mutex);
	struct lua_script_frame *curFrame = first_script_frame;

	while(curFrame != NULL) {
		if(curFrame->threadID == threadID)
			break;
		else
			curFrame = curFrame->next;
	}

	sem_post(&lua_script_frame_list_mutex);

	return curFrame;
}

/**
 * the lua callback functions
 * */
static int l_printSocket(lua_State *s) {
	struct lua_script_frame *scriptFrame = get_lua_script_frame();

	/* print the header if it is not yet written */
	if(scriptFrame->headerWritten == 0) {
		scriptFrame->headerWritten = 1;

		dprintf(scriptFrame->fd, "HTTP/1.0 200 OK\r\n");
		dprintf(scriptFrame->fd, "Content-Type: text/html\r\n\r\n");
	}

	int argc = lua_gettop(s);
	for(int i = 1; i<= argc; i++) {
		dprintf(scriptFrame->fd, "%s", lua_tostring(s, i));
	}

	return 0;
}


void runLuaScript(int fd, char* script, char* args) {
	/* allocate a frame */
	/*struct lua_script_frame *scriptFrame = */create_lua_script_frame(fd, script);

	/* initialize the lua state */
	lua_State *state = luaL_newstate();
	luaL_openlibs(state);

	/* initialize the function callbacks */
	lua_register(state, "print", l_printSocket);

	/* pass on the arguments */
	lua_newtable(state);

	char *savePtr, *savePtr2;
	char* token = strtok_r(args, "+", &savePtr);
	char *key, *val;
	while(token != NULL) {
		key = strtok_r(token, "=", &savePtr2);

		if(key != NULL) {
			val = strtok_r(NULL, "=", &savePtr2);
			lua_pushstring(state, val);
			lua_setfield(state, -2, key);
		}

		token = strtok_r(NULL, "+", &savePtr);
	}

	lua_setglobal(state, "_GET");

	/* run the file */
	luaL_dofile(state, script);

	lua_close(state);

	/* deallocate the script frame*/
	destroy_lua_script_frame();
}

void init_lua_script_system() {
	sem_init(&lua_script_frame_list_mutex, 0, 1);
	first_script_frame = NULL;
}
