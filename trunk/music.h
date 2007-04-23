#ifndef _MUSIC_H
#define _MUSIC_H

#include <id3tag.h>
#include <string>

#include "resource.h"

class Music : public Resource {
	private:
		std::string getID3FrameString(struct id3_tag* tag, std::string);
		bool initFromMP3(std::string);
	public:
		std::string artist;
		std::string album;
		std::string genre;
		long length;

		Music(unsigned, std::string);
		~Music();
		std::string getArtist();
		std::string getAlbum();
		std::string getGenre();
		void print();
		std::string getXML();
};

#endif
