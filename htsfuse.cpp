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
#include <iostream>
#include <set>
#include "htsfuse.hpp"

// http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
//https://github.com/Labs22/BlackServerOS/blob/8a6191f732d97e34adba5f2da343fed6f02a56a3/cryptography/android-fde-master/android-fde-master/read_emmc/usb.cpp
//https://github.com/cordalace/ffmc/blob/077af9bc9c43443e82255194b634ea2bcb0d9a75/main.c
// http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
using namespace std;

#define ROOT_NODE ((FSDirectory*) fuse_get_context()->private_data)

#define LOG(a) do { std::cerr << "[HTSFUSE]"<< __FUNCTION__<< "[" << __LINE__ << "]:" << a << std::endl;} while(0)

#ifndef NDEBUG
#define DEBUG(a) LOG(a)
#else
#define DEBUG(a) do {  } while(0)
#endif

FSNode::FSNode(xmlDocPtr dom,xmlNodePtr root,FSNode* parent):parent(parent),user(0),password(0) {
    this->user =  ::xmlGetProp(root, BAD_CAST "user");
    this->password =  ::xmlGetProp(root, BAD_CAST "password");
    }
		    
FSNode::~FSNode() {
    ::xmlFree(this->user);
    ::xmlFree(this->password);
    }

bool FSNode::is_directory() { return !is_file();}
bool FSNode::is_root() { return parent==0;}

		

FSDirectory::FSDirectory(xmlDocPtr dom,xmlNodePtr root,FSNode* parent):FSNode(dom,root,parent) {
	if(::strcmp((const char*)root->name,"directory")!=0) {
		LOG("[FATAL] Not directory <"<< (const char*)root->name << ">.");
		abort();
		}
		
	 if(parent==0)
	    	{
	    	if(::xmlHasProp(root, BAD_CAST "name")!=NULL)
	    		{
	    		LOG("[WARN]@name ignored in root node.");
	    		}
	    	this->path.assign("/");
	    	}
	    else
	    	{
	    	this->path.assign(parent->path);
	    	
		 xmlChar *s = ::xmlGetProp(root, BAD_CAST "name");
		 if(s==NULL) {
			LOG("[FATAL]@name is missing in <" << (const char*)root->name << ">");
			abort();
		    	}
		    this->token.assign((char*)s);
		    ::xmlFree(s);
		    if(parent!=0 && parent->parent!=0) this->path.append("/");
		    this->path.append(token);
	    	}
		
	xmlNodePtr cur_node;
	set<string> seen;
	for (cur_node = xmlFirstElementChild(root); cur_node!=NULL; cur_node = xmlNextElementSibling(cur_node)) {
		FSNode* c = NULL;
		if(strcmp((const char*)cur_node->name,"directory")==0) {
			c = new FSDirectory(dom,cur_node,this);
			}
		else if(strcmp((const char*)cur_node->name,"file")==0) {
			c = new FSFile(dom,cur_node,this);
			}
		else 
			{
			LOG("ignoring <" << (const char*)cur_node->name << ">.");
			continue;
			}
		children.push_back(c);
		if(seen.find(c->token)!=seen.end()) {
			LOG("duplicate entry " << c->token << " under " << this->path);
			abort();
			}
		seen.insert(c->token);
		}
	}

bool FSDirectory::is_file() { return false;}
		
FSDirectory::~FSDirectory() {
	for(size_t i=0;i< children.size();++i) {
		delete children[i];
		}
	children.clear();
	}

FSNode* FSDirectory::find(const char* pathstr) {
	//DEBUG("searching \"" << pathstr << "\" current is :" << this->path);
	if(this->path.compare(pathstr)==0) {
		//DEBUG("found " << pathstr);
		return this;
		}
	
	for(size_t i=0;i< children.size();++i) {
		FSNode* n = children[i]->find(pathstr);
		if(n!=0) {
			return n;
			}
		}
	return 0;
	}
	
int FSDirectory::readdir(void *buffer, fuse_fill_dir_t filler) {
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	for(size_t i=0;i< children.size();++i) {
		FSNode* n = children[i];
		if(n->is_file() && ((FSFile*)n)->bad_flag) continue;
		filler(buffer,n->token.c_str(), NULL, 0);
		}
	return 0;
	}

int FSDirectory::getattr(struct stat *stbuf) {
	std::memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
	return 0;
	}

	
FSFile::FSFile(xmlDocPtr dom,xmlNodePtr root,FSNode* parent):FSNode(dom,root,parent),content_length_ptr(0),last_modified((time_t)-1),bad_flag(false) {
	if(strcmp((char*)root->name,"file")!=0) {
		LOG("[FATAL] Not file <" << (const char*)root->name << ">.\n");
		abort();
		}
	 xmlChar *s = ::xmlGetProp(root, BAD_CAST "url");
	 if(s==NULL) s= ::xmlGetProp(root, BAD_CAST "href");
	 if(s==NULL) s= ::xmlGetProp(root, BAD_CAST "src");
	 if(s==NULL) {
	 	LOG("[FATAL]@url is missing in" << (const char*)root->name << ">.\n");
		abort();
	    	}
	 this->url.assign((char*)s);
	 ::xmlFree(s);
	 

	  
	 s = ::xmlGetProp(root, BAD_CAST "name");
	 if(s!=NULL) {
	    this->token.assign((char*)s);
	    ::xmlFree(s);
	    }
	 else
	 	{
	 	string::size_type n= this->url.find_last_of('/');
	 	if(n==string::npos || n+1==this->url.size())
	 		{
	 		LOG("[FATAL]@name is missing in" << (const char*)root->name << "> and I cannot find '/' in url \n");
			abort();
	 		}
	 	this->token.assign(this->url.substr(n+1));
	 	}

	 this->path.assign(parent->path);
	 if(parent!=0 && parent->parent!=0) this->path.append("/");
	 this->path.append(token);
	 
	 
	}
		
bool FSFile::is_file() { return true;}

FSFile::~FSFile() {
if(content_length_ptr!=NULL) delete content_length_ptr;
}


size_t read_content_callback(char *buffer,   size_t size,   size_t nitems,   void *userdata) {
	std::string* content=(string*)userdata;
	content->append(buffer,size*nitems);
	return nitems * size;
	}


size_t FSFile::length() {
	if(this->bad_flag) return 0UL;
	if(content_length_ptr==NULL) {
		 string header;
		 CURL* curl = this->create_curl();
		 if(curl!=0)
		 	{
			 curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
			 curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
			 
			 curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
			 curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, read_content_callback);
		
			  CURLcode res = curl_easy_perform(curl);
			 /* Check for errors */
			 if(res != CURLE_OK) {
			 	LOG("curl_easy_perform() failed " << curl_easy_strerror(res));
			 	}
			 else
			 	{
			 	std::istringstream iss(header);
			 	std::string line;
			 	std::string content_length_token("Content-Length:");
				while (std::getline(iss, line))
					{
					if(line.find("Last-Modified: ")==0 && this->last_modified==(time_t)-1)
						{
						struct tm tmptmp;
						strptime(&line.c_str()[15], "%a, %d %B %Y %H:%M:%S %z",&tmptmp);
						this->last_modified = ::mktime(&tmptmp);
						}
					else if(line.find("Location:")==0)
						{
						DEBUG("[WARN] resource has moved: " << url << " " << line);
						this->bad_flag = true;
						}
					else if(line.find(content_length_token)==0)
						{
						line.erase(0,content_length_token.size());
						long content_length= strtoul(line.c_str(),NULL,10);
						content_length_ptr = new  size_t;
						*content_length_ptr = content_length;
						break;
						}
					}
			 	}
			curl_easy_cleanup(curl);
			}
		
		if(content_length_ptr==NULL) {
			LOG("Cannot get length for " <<  url.c_str());
			content_length_ptr = new  size_t;
			*content_length_ptr = 0UL;
			}
		}
	return *content_length_ptr;
	}

FSNode* FSFile::find(const char* pathstr) {
    if(this->bad_flag) return 0;
	if(this->path.compare(pathstr)==0) return this;
	return 0;
	}

int FSFile::readdir(void *buffer, fuse_fill_dir_t filler) {
	LOG("[LOG]readdir asked for file "<< path);
	return -ENOTDIR;
	}

int FSFile::getattr(struct stat *stbuf) {
	if(this->bad_flag) return -ENOENT;
	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;
	stbuf->st_size = this->length();
	if(this->last_modified > (time_t)0)
		{
		stbuf->st_atime = this->last_modified;
		stbuf->st_mtime = this->last_modified;
		stbuf->st_ctime = this->last_modified;	
		}
	LOG(path << "size: "<< stbuf->st_size);
	return 0;
	}


FSFileReader::FSFileReader(FSFile* f):fsFile(f),pos(0UL) {
}

FSFileReader::~FSFileReader() {
}




CURL* FSFile::create_curl() {
	 CURL *curl = ::curl_easy_init();
	 if(curl==NULL) {
		LOG("[ERROR]::curl_easy_init failed.");
		return NULL;
		}
	 
	 ::curl_easy_setopt(curl, CURLOPT_URL, this->url.c_str());
	 
	 FSNode* curr=this;
	 while(curr!=NULL)
	 	{
	 	if(curr->user!=NULL)
	 		{
	 		DEBUG("got username " << (const char*)curr->user);
			::curl_easy_setopt(curl, CURLOPT_USERNAME,(const char*)curr->user);
			break;
			}
		curr = curr->parent;
		}
	curr=this;
	 while(curr!=NULL)
	 	{
	 	if(curr->password!=NULL)
	 		{
	 		DEBUG("got password " << (const char*)curr->user);
			curl_easy_setopt(curl, CURLOPT_PASSWORD,(const char*)curr->password);
			break;
			}
		curr = curr->parent;
		}
	 curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:62.0) Gecko/20100101 Firefox/62.0");
	 curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	 

	return curl;
	}

int FSFile::read(char *buffer, size_t size, off_t offset) {
	if(this->bad_flag) return 0;
	DEBUG("reading " << path << " length=" << this->length());
	 if(offset+size> this->length())
	 	{
	 	size = this->length()-offset;
	 	}
	 if(size==0UL) return 0;
	 
	 CURL* curl = this->create_curl();
	 if(curl==0) return -1; 
	 curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	 curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
	 string content;
	 curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	 curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_content_callback);
	 //curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE );
	 char tmp_range[1000];
	 sprintf(tmp_range,"%lld-%lld",offset,offset+(size-1));
	 curl_easy_setopt(curl, CURLOPT_RANGE, tmp_range);
	  CURLcode res = curl_easy_perform(curl);
	  /* Check for errors */
	 if(res != CURLE_OK) {
	  	DEBUG("curl_easy_perform() failed: " << curl_easy_strerror(res));
	  	::curl_easy_cleanup(curl);
	  	return -EIO;
	 	}
	 curl_easy_cleanup(curl);
	 if( size > content.size()) size = content.size();
	 memcpy(buffer,content.data(),size);
	return size;
	}



static int htsfuse_getattr(const char *path, struct stat *stbuf) {
  DEBUG(path);
  FSNode* node = ROOT_NODE->find(path);
  if(node==NULL) {
  	DEBUG("File not found " << path);
  	return -ENOENT;
	}
  node->getattr(stbuf);
  return 0;
}

static int htsfuse_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi) {
  DEBUG(path);
  FSNode* node = ROOT_NODE->find(path);
  if(node==NULL) {
  	DEBUG("File not found " << path);
  	return -ENOENT;
	}
  node->readdir(buffer,filler);
  return 0;
}



static int htsfuse_open(const char *path,struct fuse_file_info *fi) {
  DEBUG(path);
  FSNode* node = ROOT_NODE->find(path);
  if(node==NULL || !node->is_file()) {
  	DEBUG("File not found " << path);
  	return -ENOENT;
	}
  fi->fh = (uint64_t) new FSFileReader((FSFile*)node);
  return 0;
}

static int htsfuse_release(const char *path,struct fuse_file_info *fi) {
  DEBUG(path);
  FSFileReader* r = (FSFileReader*)fi->fh;
  if(r!=0) delete r;
  return 0;
}

static int htsfuse_read(const char *path, char *buffer, size_t size, off_t offset,struct fuse_file_info *fi) {
  DEBUG(path);
  FSFileReader* r = (FSFileReader*)fi->fh;
  if(r==0) {
  	DEBUG("pointer is null for " << path << "??");
  	return -1;
  	}
  return r->fsFile->read(buffer,size,offset);
}


int main(int argc,char** argv)
	{
	int ret=0;
	LIBXML_TEST_VERSION
	 curl_global_init(CURL_GLOBAL_ALL);
	if(argc<3) {
       		fprintf(stderr,"XML file / MOUNT_DIR .\n");
       		return EXIT_FAILURE;
    		}
	 xmlDoc *doc = ::xmlReadFile(argv[1], NULL, 0);

	if (doc == NULL) {
       		fprintf(stderr,"error: could not parse XML file %s\n", argv[1]);
       		return EXIT_FAILURE;
    		}
    	FSDirectory* fs_root = new FSDirectory(doc,xmlDocGetRootElement(doc),0);
    	
	::xmlFreeDoc(doc);
	::xmlCleanupParser();
	
	
	struct fuse_operations operations;
	memset((void*)&operations,0,sizeof(struct fuse_operations));
	operations.open = htsfuse_open;
	operations.read = htsfuse_read;
	operations.release = htsfuse_release;
	operations.readdir = htsfuse_readdir;
	operations.getattr = htsfuse_getattr;
	
	
	ret= fuse_main( argc-1, &argv[1], &operations,fs_root );
	//delete fs_root;
	
	return ret;
	}
