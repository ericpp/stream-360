#ifndef _VIDEO_H
#define _VIDEO_H

#include <id3tag.h>
#include <string>

#include "resource.h"

class Video : public Resource {
	std::string author;
	long duration;

	public:
		Video(unsigned, std::string, std::string);
		~Video();
		void print();
		std::string getXML();
};

#endif
