// Content Directory

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>

#include "directory.h"
#include "xmlutils.h"

using namespace std;

Directory::Directory() {
	//this->resources = NULL;
	this->globalID = 100; // start at 100: xbox has some reserved container IDs

	this->rootContainer = new Container(0, NULL, "root", "#root#");
	this->artists = new Container(6, this->rootContainer, "object.container.person.musicArtist", "#artists#");
	this->albums = new Container(7, this->rootContainer, "object.container.album.musicAlbum", "#albums#");
	this->songs = new Container(4, this->rootContainer, "object.item.audioItem", "#songs#");
	this->videos = new Container(15, this->rootContainer, "object.item.videoItem", "#videos#");
	//this->videos = new Container(8, this->rootContainer, "object.item.videoItem", "#videos#");

	this->rootContainer->addContainer(this->artists);
	this->rootContainer->addContainer(this->albums);
	this->rootContainer->addContainer(this->songs);
	this->rootContainer->addContainer(this->videos);
}

Directory::~Directory() {
	delete this->rootContainer;
	delete this->artists;
	delete this->albums;
	delete this->songs;
}

int Directory::addMusic(Music* res) {
	Container* artist;
	Container* album;

	artist = this->artists->getContainerByTitle(res->artist);
	if(artist == NULL) {
		artist = new Container(this->globalID++, this->artists, "object.container.person.musicArtist", res->artist);
		this->artists->addContainer(artist);
	}

	album = artist->getContainerByTitle(res->album);
	if(album == NULL) {
		album = new Container(this->globalID++, artist, "object.container.album.musicAlbum", res->album);
		artist->addContainer(album);
		this->albums->addContainer(album);
	}
	album->addResource(res);
	songs->addResource(res);

	return 1;
}

int Directory::addVideo(Video* res) {
	videos->addResource(res);

	return 1;
}

int Directory::addFolder(string path) {
	Resource* res;
	DIR* dir;
	struct dirent* dirent;
	char rpath[PATH_MAX];
        struct stat fStat;
	string curFolder;
	string filePath;

	// resolve the path
	realpath(path.c_str(), rpath);
	curFolder = string(rpath);

	cout << "searching " << curFolder << " for files..." << endl;

	// open the directory up (assuming it's a directory)
	dir = opendir(curFolder.c_str());
	if(dir != NULL) {
		while((dirent = readdir(dir)) != NULL) {
			if(strcmp(dirent->d_name,".") != 0 && strcmp(dirent->d_name,"..") != 0) {
				filePath = curFolder + "/" + string(dirent->d_name);
				stat(filePath.c_str(), &fStat);

				if(S_ISDIR(fStat.st_mode) && dirent->d_name[0] != '.') {
//cout << "Adding directory: " << filePath << endl;
					this->addFolder(filePath);
				}
				else if(S_ISREG(fStat.st_mode)) {
					if(filePath.find(".mp3") != string::npos || filePath.find(".wav") != string::npos || filePath.find(".wma") != string::npos) {
						res = new Music(this->globalID++, filePath);
						if(res != NULL) {
							this->addMusic((Music*)res);
						}
					}
					//else if((filePath.find(".wmv") != string::npos)) {
					else if((filePath.find(".wmv") != string::npos) || (filePath.find(".avi") != string::npos) || (filePath.find(".asf") != string::npos) || (filePath.find(".mov") != string::npos) || (filePath.find(".mpg") != string::npos)){
						res = new Video(this->globalID++, string(dirent->d_name), filePath);
						if(res != NULL) {
							this->addVideo((Video*)res);
						}
					}
				}
				else {
cout << "No access to file: " << filePath << endl;

				}
			}
		}
		closedir(dir);
	}
	return 1;
}

Resource* Directory::getResourceByID(unsigned resourceID) {
	Resource* res;

	res = this->songs->getResourceById(resourceID);

	if(res == NULL) {
		res = this->videos->getResourceById(resourceID);
	}

	return res;
}

map<string,string> Directory::getSearchCriteria(string criteria) {
	map <string,string> result;
	char* key = NULL;
	char* val = NULL;
	char* str;
	int found_key = -1;
	int found_val = -1;
	bool found_equals = false;

	str = strdup(criteria.c_str());
	if(str != NULL) {
		for(unsigned i = 0; i < criteria.size(); i++) {
			if(found_key == -1 && found_val == -1) { // haven't found anything yet
				if(str[i] == '(') {
					found_key = i+1;
				}
			}
			else if(found_key > -1 && found_val == -1) { // parsing a key and haven't found a val
				if(str[i] == ' ') { // end of key
					str[i] = '\0';
					key = (str+found_key);
					found_key = -2;
				}

			}
			else if(found_key == -2 && found_val == -1) { // parsed a key but no val yet
				if(found_equals == false && str[i] == '=') {
					found_equals = true;
				}
				else if(str[i] == '"') {
					found_val = i+1;
				}
			}
			else if(found_key == -2 && found_val > -1) { // parsed a key and now found a val
				if(str[i] == '"') {
					str[i] = '\0';
					val = (str+found_val);
					found_val = -2;
				}
			}
			else if(found_key == -2 && found_val == -2) {
				if(str[i] == ')') {
					if(key != NULL && val != NULL) {
						//printf("%s = %s\n", key, val);
						result[string(key)] = string(val);
					}
					found_key = -1;
					found_val = -1;
				}
			}
		}
	}

	return result;
}

int Directory::handleSearch(Upnp_Action_Request* request) {
	IXML_Document* actrequest;
	IXML_Document* didldoc;
	IXML_Document* doc;
	IXML_Element* parent;
	IXML_Element* element;
	IXML_NodeList* nodes;
	int numReturned = 0;

	//search(string containerID, string searchCriteria, string filter, int startIndex, int total, string sortCriteria) {
	string containerID;
	string searchCriteria;
	string filter;

	// process the request
	actrequest = request->ActionRequest;

	nodes = ixmlElement_getElementsByTagName((IXML_Element*)request->ActionRequest, "u:Search");
	element = (IXML_Element*) ixmlNodeList_item(nodes, 0);
	containerID = getChildValue(element, "ContainerID");
	searchCriteria = getChildValue(element, "SearchCriteria");
	filter = getChildValue(element, "Filter");


	cout << "searching" << endl;
	//vector<Resource*> results = contentDirectory->search(containerID, searchCriteria, filter, 0, 1000, "+dc:title");
	//vector<Resource*>::iterator resiterator;

	// construct the didl document
	cout << "making didl doc" << endl;
	didldoc = ixmlDocument_createDocument();
	parent = makeElement(didldoc, (IXML_Node*) didldoc, "DIDL-Lite", NULL);
	ixmlElement_setAttribute(parent, "xmlns:dc", "http://purl.org/dc/elements/1.1/");
	ixmlElement_setAttribute(parent, "xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");
	ixmlElement_setAttribute(parent, "xmlns", "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/");

	ostringstream temp;
	cout << "looping through results" << endl;
	int cID = atoi(containerID.c_str());
	map<string,string> sc = getSearchCriteria(searchCriteria);
	if(cID != -1) {
		Container* top;
		Container* con;
		Resource* res;
		string xml;
		IXML_Document* tdoc;
		IXML_Node* tnode;
		IXML_Node* cnode;

		top = NULL;

		if(cID < 100 && sc.find("upnp:class") != sc.end()) {
			if(sc["upnp:class"] == "object.container.person.musicArtist") {
				top = this->artists;
			}
			else if(sc["upnp:class"] == "object.container.album.musicAlbum") {
				top = this->albums;
			}
			else if(sc["upnp:class"] == "object.item.audioItem") {
				top = this->songs;
			}
			else if(sc["upnp:class"] == "object.item.videoItem") {
				top = this->videos;
			}
		}
		else {
			top = this->rootContainer->getContainerById(cID, true);
		}

		if(top != NULL) {
			if(top->numContainers() > 0) {
				map<unsigned,Container*> containers;
				map<unsigned,Container*>::iterator coniterator;

				containers = top->getContainers();
				coniterator = containers.begin();
				while(coniterator != containers.end()) {
					con = coniterator->second;
					if((sc.find("upnp:artist") == sc.end() || con->getTitle("object.container.person.musicArtist") == sc["upnp:artist"]) || (sc.find("upnp:album") == sc.end() || con->getTitle("object.container.album.musicAlbum") == sc["upnp:album"])) {
						xml = "<root>" + con->getXML() + "</root>";
						tdoc = ixmlParseBuffer((char*)xml.c_str());
						if(tdoc != NULL) {
							cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
							if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
								ixmlNode_appendChild((IXML_Node*)parent, tnode);
								numReturned++;
							}
							ixmlDocument_free(tdoc);
						}
					}
					coniterator++;
				}
			}
			if(top->numResources() > 0) {
				map<unsigned,Resource*> resources;
				map<unsigned,Resource*>::iterator resiterator;

				resources = top->getResources();
				resiterator = resources.begin();
				while(resiterator != resources.end()) {
					res = resiterator->second;
					if(res->getType() == "object.item.audioItem") {
						Music* mus = (Music*) res;
						if(!mus->title.empty() && (sc.find("dc:title") == sc.end() || sc["dc:title"] == mus->getTitle()) && (sc.find("upnp:artist") == sc.end() || sc["upnp:artist"] == mus->getArtist()) && (sc.find("upnp:album") == sc.end() || sc["upnp:album"] == mus->getAlbum())) {
							xml = "<root>" + mus->getXML() + "</root>";
							tdoc = ixmlParseBuffer((char*)xml.c_str());
							if(tdoc != NULL) {
								cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
								if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
									ixmlNode_appendChild((IXML_Node*)parent, tnode);
									numReturned++;
								}
								ixmlDocument_free(tdoc);
							}
						}
					}
					else if(res->getType() == "object.item.videoItem") {
						Video* vid = (Video*) res;
						if(!vid->title.empty() && (sc.find("dc:title") == sc.end() || sc["dc:title"] == vid->getTitle())) {
							xml = "<root>" + vid->getXML() + "</root>";
							tdoc = ixmlParseBuffer((char*)xml.c_str());
							if(tdoc != NULL) {
								cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
								if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
									ixmlNode_appendChild((IXML_Node*)parent, tnode);
									numReturned++;
								}
								ixmlDocument_free(tdoc);
							}
						}
					}
					resiterator++;
				}				
			}
		}
	}


	cout << "creating soapage" << endl;
	doc = ixmlDocument_createDocument();
	parent = ixmlDocument_createElementNS(doc, "urn:schemas-upnp-org:service:ContentDirectory:1", "u:SearchResponse");
	ixmlNode_appendChild((IXML_Node*) doc, (IXML_Node*) parent);
	makeElement(doc, (IXML_Node*) parent, "Result", ixmlPrintDocument(didldoc));
	temp.str(""); temp << numReturned;
	makeElement(doc, (IXML_Node*) parent, "NumberReturned", temp.str().c_str());
	makeElement(doc, (IXML_Node*) parent, "TotalMatches", temp.str().c_str());
	makeElement(doc, (IXML_Node*) parent, "UpdateID", "1");

	//printf("Search: %s", ixmlPrintDocument(request->ActionRequest));
	//printf("Response: %s", ixmlPrintDocument(doc));

	request->ActionResult = doc;

	return 1;
}

int Directory::handleBrowse(Upnp_Action_Request* request) {
	IXML_Document* actrequest;
	IXML_Document* didldoc;
	IXML_Document* doc;
	IXML_Element* parent;
	IXML_Element* element;
	IXML_NodeList* nodes;
	int numReturned = 0;

	string containerID;
	string browseFlag;
	string filter;

	// process the request
	actrequest = request->ActionRequest;

	nodes = ixmlElement_getElementsByTagName((IXML_Element*)request->ActionRequest, "u:Browse");
	element = (IXML_Element*) ixmlNodeList_item(nodes, 0);
	containerID = getChildValue(element, "ContainerID");
	browseFlag = getChildValue(element, "BrowseFlag");
	filter = getChildValue(element, "Filter");

	cout << "browsing" << endl;

	// construct the didl document
	cout << "making didl doc" << endl;
	didldoc = ixmlDocument_createDocument();
	parent = makeElement(didldoc, (IXML_Node*) didldoc, "DIDL-Lite", NULL);
	ixmlElement_setAttribute(parent, "xmlns:dc", "http://purl.org/dc/elements/1.1/");
	ixmlElement_setAttribute(parent, "xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");
	ixmlElement_setAttribute(parent, "xmlns", "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/");

	ostringstream temp;
	cout << "looping through results" << endl;
	int cID = atoi(containerID.c_str());
	if(cID != -1) {
		Container* top;
		Container* con;
		Resource* res;
		string xml;
		IXML_Document* tdoc;
		IXML_Node* tnode;
		IXML_Node* cnode;

		top = this->rootContainer->getContainerById(cID, true);

		if(top != NULL) {
			if(top->numContainers() > 0) {
				map<unsigned,Container*> containers;
				map<unsigned,Container*>::iterator coniterator;

				containers = top->getContainers();
				coniterator = containers.begin();
				while(coniterator != containers.end()) {
					con = coniterator->second;
					xml = "<root>" + con->getXML() + "</root>";
					tdoc = ixmlParseBuffer((char*)xml.c_str());
					if(tdoc != NULL) {
						cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
						if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
							ixmlNode_appendChild((IXML_Node*)parent, tnode);
							numReturned++;
						}
						ixmlDocument_free(tdoc);
					}
					coniterator++;
				}
			}
			if(top->numResources() > 0) {
				map<unsigned,Resource*> resources;
				map<unsigned,Resource*>::iterator resiterator;

				resources = top->getResources();
				resiterator = resources.begin();
				while(resiterator != resources.end()) {
					res = resiterator->second;
					if(res->getType() == "object.item.audioItem") {
						Music* mus = (Music*) res;
						xml = "<root>" + mus->getXML() + "</root>";
						tdoc = ixmlParseBuffer((char*)xml.c_str());
						if(tdoc != NULL) {
							cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
							if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
								ixmlNode_appendChild((IXML_Node*)parent, tnode);
								numReturned++;
							}
							ixmlDocument_free(tdoc);
						}
					}
					else if(res->getType() == "object.item.videoItem") {
						Video* vid = (Video*) res;
						if(!vid->title.empty()) {
							xml = "<root>" + vid->getXML() + "</root>";
							tdoc = ixmlParseBuffer((char*)xml.c_str());
							if(tdoc != NULL) {
								cnode = ixmlNode_getFirstChild((IXML_Node*) tdoc);
								if(ixmlDocument_importNode(didldoc, cnode, TRUE, &tnode) == IXML_SUCCESS) {
									ixmlNode_appendChild((IXML_Node*)parent, tnode);
									numReturned++;
								}
								ixmlDocument_free(tdoc);
							}
						}
					}
					resiterator++;
				}				
			}
		}
	}


	cout << "creating soapage" << endl;
	doc = ixmlDocument_createDocument();
	parent = ixmlDocument_createElementNS(doc, "urn:schemas-upnp-org:service:ContentDirectory:1", "u:BrowseResponse");
	ixmlNode_appendChild((IXML_Node*) doc, (IXML_Node*) parent);
	makeElement(doc, (IXML_Node*) parent, "Result", ixmlPrintDocument(didldoc));
	temp.str(""); temp << numReturned;
	makeElement(doc, (IXML_Node*) parent, "NumberReturned", temp.str().c_str());
	makeElement(doc, (IXML_Node*) parent, "TotalMatches", temp.str().c_str());
	makeElement(doc, (IXML_Node*) parent, "UpdateID", "1");

	//printf("Search: %s", ixmlPrintDocument(request->ActionRequest));
	//printf("Response: %s", ixmlPrintDocument(doc));

	request->ActionResult = doc;

	return 1;
}
