// Content Directory

#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include <vector>
#include <string>
#include <map>

#include "container.h"
#include "resource.h"
#include "music.h"
#include "video.h"

class Directory {
	Container* rootContainer;
	Container* artists;
	Container* albums;
	Container* songs;
	Container* videos;
	unsigned int globalID;

	public:
		Directory();
		~Directory();
		int addMusic(Music*);
		int addVideo(Video*);
		int addFolder(std::string);
		Resource* getResourceByID(unsigned);
		//std::vector<Resource*> search(std::string containerID, std::string searchCriteria, std::string filter, int startIndex, int total, std::string sortCriteria);
		int handleRequest(Upnp_Action_Request* request);
		std::map<std::string,std::string> getSearchCriteria(std::string);

};

#endif
