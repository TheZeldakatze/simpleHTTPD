/*
 * httpd.h
 *
 *  Created on: 18.06.2022
 *      Author: victor
 */

#ifndef HTTPD_H_
#define HTTPD_H_

#ifndef HTDOCS_ROOT
#define HTDOCS_ROOT "htdocs"
#endif

/* get the path delimiter */
#ifdef _WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

/* enable the lua interpeter, compile with -llua -lm -ldl */
#define ENABLE_LUA_SCRIPTING 1

#endif /* HTTPD_H_ */
