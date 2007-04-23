// Content Directory Resource

#include <id3tag.h>

#include <string>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "resource.h"

using namespace std;

Resource::Resource(unsigned id, string type, string title, string path) {
	this->id = id;
	this->type = type;
	this->title = title;
	this->filename = path;
	this->transcode = false;

	if(path.find(".mp3") != string::npos) {
		this->mimetype = "audio/mpeg";
	}
	else if(path.find(".wmv") != string::npos) {
		this->mimetype = "video/x-ms-wmv";
	}
	else if((path.find(".avi") != string::npos) || (path.find(".asf") != string::npos) || (path.find(".mov") != string::npos) || (path.find(".mpg") != string::npos)) {
		this->mimetype = "video/x-ms-wmv";
		this->transcode = true;
	}
	/*else if(path.find(".asf") != string::npos) {
		this->mimetype = "video/x-ms-asf";
	}
	else if(path.find(".mpg") != string::npos) {
		this->mimetype = "video/mpeg";
	}*/
}

Resource::~Resource() {}

unsigned Resource::getID() {
	return this->id;
}

string Resource::getType() {
	return this->type;
}

string Resource::getTitle() {
	return this->title;
}

string Resource::getMimeType() {
	return this->mimetype;
}

string Resource::getFile() {
	return this->filename;
}

long Resource::getFileSize() {
        struct stat fileStat;
	long ret = -1;

	if(stat(this->filename.c_str(), &fileStat) != -1) {
		ret = fileStat.st_size;
	}

	return ret;
}

void Resource::print() {
	cout << "===== Resource =====" << endl;
	cout << "File: " << this->filename << endl;
	cout << "Filesize: " << this->getFileSize() << endl;
	cout << "Title: " << this->getTitle() << endl;
	cout << endl;
}
