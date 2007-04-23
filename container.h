#ifndef _CONTAINER_H
#define _CONTAINER_H

#include <string>
#include <map>

#include "resource.h"

class Container {
	unsigned id;
	Container* parent;
	std::string title;
	std::string type;
	std::map<unsigned,Container*> idContainers;
	std::map<std::string,Container*> titleContainers;
	std::map<unsigned,Resource*> resources;
	bool sorted;

	public:
		Container(unsigned id, Container* parent, std::string type, std::string title);
		~Container();
		unsigned getID();
		Container* getParent();
		unsigned getParentID();
		std::string getTitle(std::string="");
		std::string getType();
		std::map<unsigned,Container*> getContainers();
		std::map<unsigned,Resource*> getResources();
		unsigned numContainers();
		unsigned numResources();
		Container* getContainerById(unsigned,bool=false);
		Container* getContainerByTitle(std::string,bool=false);
		Resource* getResourceById(unsigned);
		void addContainer(Container*);
		void addResource(Resource*);
		static bool cmp(Container*, Container*);
		std::string getXML();
};

#endif
