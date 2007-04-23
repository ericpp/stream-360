#include <string>
#include <iostream>
#include <sstream>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>
//#include <ffmpeg/avformat.h>
#include <id3tag.h>

#include "music.h"

using namespace std;

Music::Music(unsigned id, string path) : Resource(id, "object.item.audioItem", "", path) {
	this->initFromMP3(path);
}

Music::~Music() { }

bool Music::initFromMP3(string path) {
	struct id3_file* id3file;
	struct id3_tag* tag;
	string fstring;
	bool ret = false;

	id3file = id3_file_open(path.c_str(), ID3_FILE_MODE_READONLY);
	if(id3file != NULL) {
		tag = id3_file_tag(id3file);
		if(tag != NULL) {
			this->filename = path;
			this->title = this->getID3FrameString(tag, ID3_FRAME_TITLE);
			this->artist = this->getID3FrameString(tag, ID3_FRAME_ARTIST);
			this->album = this->getID3FrameString(tag, ID3_FRAME_ALBUM);
			this->genre = this->getID3FrameString(tag, ID3_FRAME_GENRE);

			fstring = this->getID3FrameString(tag, "TLEN");
			if(!fstring.empty()) {
				this->length = atol(fstring.c_str());
			}
			else {
				this->length = 0;
			}

			ret = true;
		}
		else {
			cout << "ID3Tag: getting tag from file" << path << endl;
		}

		id3_file_close(id3file);
	}
	else {
		cout << "ID3Tag: error opening file: " << path << endl;
	}

	return ret;	
}

/*bool Music::initFromMP3(string path) {
	AVFormatContext* context;
	bool ret = false;

	if(av_open_input_file(&context, path.c_str(), NULL, 0, NULL) >= 0) {
		this->filename = path;

		this->title = string(context->title);
		this->artist = string(context->author);
		this->album = string(context->album);
		this->length = context->duration;
		this->genre = string(context->genre);
cout << path << " = " << this->length << endl;
		ret = true;
	}

	return ret;	
}*/

string Music::getID3FrameString(struct id3_tag* tag, string sframeID) {
	struct id3_frame* frame;
	union id3_field* field;
	int nstrings;
	id3_latin1_t* cstring;
	string ccstring;
	const char* frameID = sframeID.c_str();
	
	ccstring = string();

	frame = id3_tag_findframe(tag, frameID, 0);
	if(frame != NULL) {
		field = id3_frame_field(frame, 1);
		nstrings = id3_field_getnstrings(field);
		if(nstrings > 0) {
			cstring = id3_ucs4_latin1duplicate(id3_field_getstrings(field, 0));
			ccstring = string((const char*)cstring);
		}
	}

	return ccstring;
}

string Music::getArtist() {
	return this->artist;
}

string Music::getAlbum() {
	return this->album;
}

string Music::getGenre() {
	return this->genre;
}

string Music::getXML() {
	ostringstream ss;

	ss << "<item id=\"" << this->getID() << "\" parentID=\"0\" restricted=\"1\">" << endl;
	ss << "  <dc:title>" << this->getTitle() << "</dc:title>" << endl;
	ss << "  <upnp:artist>" << this->getArtist() << "</upnp:artist>" << endl;
	ss << "  <upnp:album>" << this->getAlbum() << "</upnp:album>" << endl;
	ss << "  <upnp:class>" << this->getType() << "</upnp:class>" << endl;

	ss << "  <res size=\"" << this->getFileSize() << "\" protocolInfo=\"http-get:*:audio/mpeg:*\">" << "http://" << UpnpGetServerIpAddress() << ":" << UpnpGetServerPort() << "/content/" << this->getID() << "</res>";

	ss << "</item>" << endl;

	return ss.str();
}
