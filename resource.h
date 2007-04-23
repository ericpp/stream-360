// Content Directory

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <id3tag.h>
#include <string>

class Resource {
	public:
		unsigned id;
		std::string type;
		std::string title;
		std::string filename;
		std::string mimetype;
		bool transcode;
	
		Resource(unsigned, std::string, std::string, std::string);
		~Resource();
		unsigned getID();
		std::string getTitle();
		std::string getType();
		std::string getMimeType();
		std::string getFile();
		long getFileSize();
		void print();
		std::string getXML();
};

#endif
