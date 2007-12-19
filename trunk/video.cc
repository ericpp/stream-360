#include <string>
#include <iostream>
#include <sstream>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

extern "C" {
#include <ffmpeg/avformat.h>
}

#include "video.h"

using namespace std;

Video::Video(unsigned id, string title, string path) : Resource(id, "object.item.videoItem", title, path) {
	AVFormatContext* context;

	if(av_open_input_file(&context, path.c_str(), NULL, 0, NULL) >= 0) {
		if(context->title[0] != '\0') {
			this->title = string(context->title);
		}
		this->author = string(context->author);
	}
}

Video::~Video() { }

string Video::getXML() {
	ostringstream ss;

	ss << "<item id=\"" << this->getID() << "\" parentID=\"0\" restricted=\"1\">" << endl;
	ss << "  <dc:title>" << this->title << "</dc:title>" << endl;
	ss << "  <upnp:author>" << this->author << "</upnp:author>" << endl;
	ss << "  <upnp:class>" << this->getType() << "</upnp:class>" << endl;

	ss << "  <res size=\"" << this->getFileSize() << "\" protocolInfo=\"http-get:*:video/x-ms-wmv:*\">" << "http://" << UpnpGetServerIpAddress() << ":" << UpnpGetServerPort() << "/content/" << this->getID() << "</res>";

	ss << "</item>" << endl;

	return ss.str();
}
