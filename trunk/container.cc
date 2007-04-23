
#include <algorithm>
#include <iostream>
#include <sstream>

#include "container.h"
#include "xmlutils.h"

using namespace std;

Container::Container(unsigned id, Container* parent, string type, string title) {
	this->id = id;
	this->type = type;
	this->title = title;
	this->parent = parent;
	this->sorted = false;
}

Container::~Container() {}

unsigned Container::getID() {
	return this->id;
}

Container* Container::getParent() {
	return this->parent;
}

unsigned Container::getParentID() {
	if(this->parent == NULL) {
		return -1;
	}
	else {
		return this->parent->getID();
	}
}

string Container::getTitle(string type) {
	Container* tmp;
	string title = "";

	if(type != "") {
		tmp = this;
		while(tmp != NULL && title == "") {
			if(type == tmp->getType()) {
				title = tmp->getTitle();
			}
			tmp = tmp->getParent();
		}
	}
	else {
		title = this->title;
	}

	return title;
}

string Container::getType() {
	return this->type;
}

std::map<unsigned,Container*> Container::getContainers() {
	return this->idContainers;
}

std::map<unsigned,Resource*> Container::getResources() {
	return this->resources;
}

unsigned Container::numContainers() {
	return this->idContainers.size();
}

unsigned Container::numResources() {
	return this->resources.size();
}

void Container::addContainer(Container* container) {
	this->sorted = false;
	this->idContainers[container->getID()] = container;
	this->titleContainers[container->getTitle()] = container;
}

void Container::addResource(Resource* res) {
	this->sorted = false;
	this->resources[res->getID()] = res;
}

Container* Container::getContainerById(unsigned id, bool recursive) {
	map<unsigned,Container*>::iterator iterator;
	Container* tmp;
	Container* ret = NULL;

	iterator = this->idContainers.find(id);
	if(iterator != this->idContainers.end()) {
		ret = iterator->second;
	}

	if(ret == NULL && recursive == true) {
		iterator = this->idContainers.begin();
		while(iterator != this->idContainers.end() && ret == NULL) {
			tmp = iterator->second;
			if(tmp != NULL) {
				ret = tmp->getContainerById(id, recursive);
			}
			iterator++;
		}
	}

	return ret;
}

Container* Container::getContainerByTitle(string title, bool recursive) {
	map<string,Container*>::iterator iterator;
	Container* tmp;
	Container* ret = NULL;

	iterator = this->titleContainers.find(title);
	if(iterator != this->titleContainers.end()) {
		ret = iterator->second;
	}

	if(ret == NULL && recursive == true) {
		iterator = this->titleContainers.begin();
		while(iterator != this->titleContainers.end() && ret == NULL) {
			tmp = iterator->second;
			if(tmp != NULL) {
				ret = tmp->getContainerByTitle(title, recursive);
			}
			iterator++;
		}
	}

	return ret;	
}

Resource* Container::getResourceById(unsigned id) {
	map<unsigned,Resource*>::iterator iterator;
	Resource* ret = NULL;

	iterator = this->resources.find(id);
	if(iterator != this->resources.end()) {
		ret = iterator->second;
	}

	return ret;
}

bool Container::cmp(Container* con1, Container* con2) {
	return (con1->getTitle() < con2->getTitle());
}

string Container::getXML() {
	ostringstream ss;

	ss << "<container id=\"" << this->getID() << "\" parentID=\"" << this->getParentID() << "\" restricted=\"1\">" << endl;
	ss << "  <dc:title>" << this->getTitle() << "</dc:title>" << endl;
	ss << "  <upnp:class>" << this->getType() << "</upnp:class>" << endl;
	ss << "</container>" << endl;

	return ss.str();
}

/*
vector<Container*> Container::getContainers() {
	if(!this->sorted) {
		sort(this->containers.begin(), this->containers.end(), Container::cmp);
		this->sorted = true;
	}
	return this->containers;
}

vector<Resource*> Container::getResources() {
	if(!this->sorted) {
		sort(this->resources.begin(), this->resources.end(), Resource::cmp);
		this->sorted = true;
	}
	return this->resources;
}
*/
