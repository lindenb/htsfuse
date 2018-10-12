/*
The MIT License (MIT)

Copyright (c) 2018 Pierre Lindenbaum

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*/
#ifndef HTSFUSE_HPP
#define HTSFUSE_HPP
#include <cstdlib>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <ctime>
#include <vector>
#include <cerrno>
#include <curl/curl.h> 
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif
#include <fuse.h>
#include <libxml/parser.h>
#include <libxml/tree.h>





class FSNode {
	public:
		std::string  token;
		FSNode* parent;
		std::string path;
		xmlChar *user;
		xmlChar *password;
		FSNode(xmlDocPtr dom,xmlNodePtr root,FSNode* parent);
		virtual ~FSNode();
		virtual bool is_file()=0;
		virtual bool is_directory();
		virtual bool is_root();
		virtual FSNode* find(const char* pathstr)=0;
		virtual int readdir(void *buffer, fuse_fill_dir_t filler)=0;
		virtual int getattr(struct stat *stbuf) = 0;
		
	};

class FSDirectory:public FSNode {
	private:
		std::vector<FSNode*> children;
	public:	
		FSDirectory(xmlDocPtr dom,xmlNodePtr root,FSNode* parent);
		virtual ~FSDirectory();
		virtual bool is_file();
		virtual FSNode* find(const char* pathstr);
		virtual int readdir(void *buffer, fuse_fill_dir_t filler);
		virtual int getattr(struct stat *stbuf);
	};

class FSFile:public FSNode {
	private:
		CURL *create_curl();
	public:
		std::string  url;
		size_t* content_length_ptr;
		struct tm* last_modified;
		FSFile(xmlDocPtr dom,xmlNodePtr root,FSNode* parent);
		size_t length();
		virtual bool is_file();
		virtual ~FSFile();
		virtual FSNode* find(const char* pathstr);
		virtual int readdir(void *buffer, fuse_fill_dir_t filler);
		virtual int getattr(struct stat *stbuf);
		int read(char *buffer, size_t size, off_t offset);
		
		
	};
class FSFileReader {
	public:
		FSFile* fsFile;
		off_t pos;
		FSFileReader(FSFile* fs);
		~FSFileReader();
		
	};


#endif


