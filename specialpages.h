/*
 * headers.h
 *
 *  Created on: 04.06.2022
 *      Author: victor
 */

#ifndef SPECIALPAGES_H_
#define SPECIALPAGES_H_

const char* DEFAULT_FILE = "index.html";

const char* PAGE_404_HEAD = "HTTP/1.0 404 NOT FOUND\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n\r\n";
const char* PAGE_404_BODY = "<html><body><h1>404 Not Found</h1><hr>Simple httpd; http/1.0</body></html>";
const size_t PAGE_404_BODY_SIZE = sizeof(PAGE_404_BODY);

const char* PAGE_403_HEAD = "HTTP/1.0 403 FORBIDDEN\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n\r\n";
const char* PAGE_403_BODY = "<html><body><h1>403 Forbidden</h1><hr>Simple httpd; http/1.0</body></html>";
const size_t PAGE_403_BODY_SIZE = sizeof(PAGE_403_BODY);

const char* PAGE_408_HEAD = "HTTP/1.0 408 Request Timeout\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n\r\n";
const char* PAGE_408_BODY = "<html><body><h1>408 Request Timeout</h1><hr>Simple httpd; http/1.0</body></html>";
const size_t PAGE_408_BODY_SIZE = sizeof(PAGE_408_BODY);

#endif /* SPECIALPAGES_H_ */
