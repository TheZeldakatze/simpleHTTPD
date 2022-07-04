/*
 * lua_scripting.h
 *
 *  Created on: 01.07.2022
 *      Author: victor
 */

#ifndef LUA_SCRIPTING_H_
#define LUA_SCRIPTING_H_

extern void init_lua_script_system();
extern void runLuaScript(int socket, char* script, char* args);

#endif /* LUA_SCRIPTING_H_ */
