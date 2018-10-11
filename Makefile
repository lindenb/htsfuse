CFLAGS= -g -Wall  -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags`  `curl-config --cflags` `xml2-config --cflags`
LIBS=`pkg-config fuse --libs` `curl-config --libs` `xml2-config --libs`
.PHONY:all

all: htsfuse

htsfuse : htsfuse.cpp htsfuse.hpp
	g++ -o $@ $(CFLAGS) $< $(LIBS)

test:
	-fusermount -u tmp_fuse
	-rmdir tmp_fuse
	mkdir -p tmp_fuse
	chmod a+xw tmp_fuse
	./htsfuse test.xml -d ${PWD}/tmp_fuse 
	ls tmp_fuse


clean:
	-fusermount -u tmp_fuse
	-rmdir  tmp_fuse
	rm -f htsfuse
