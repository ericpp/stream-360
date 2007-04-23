#include <iostream>

#include "xmlutils.h"

using namespace std;

IXML_Element* makeElement(IXML_Document* doc, IXML_Node* parentNode, const char* tagName, const char* value) {
	IXML_Element* element;
	IXML_Node* text;

	element = ixmlDocument_createElement(doc, (char*)tagName);

	if(value != NULL) {
		text = ixmlDocument_createTextNode(doc, (char*)value);
		ixmlNode_appendChild((IXML_Node*) element, text);
	}

	ixmlNode_appendChild(parentNode, (IXML_Node*) element);

	return element;
}

string getChildValue(IXML_Element* element, const char* childTagName) {
	IXML_NodeList* nodes;
	IXML_Node* child;
	IXML_Node* grandchild;
	string content;

	nodes = ixmlElement_getElementsByTagName(element, (char*)childTagName);
	if(ixmlNodeList_length(nodes) > 0) {
		child = ixmlNodeList_item(nodes, 0);

		if(ixmlNode_hasChildNodes(child)) {
			nodes = ixmlNode_getChildNodes((IXML_Node*) child);
			for(unsigned long i = 0; i < ixmlNodeList_length(nodes); i++) {
				grandchild = ixmlNodeList_item(nodes, i);
				if(grandchild->nodeType == eTEXT_NODE) {
					content.append(grandchild->nodeValue);
				}
			}
		}

		cout << childTagName << "=" << content << endl;

		return content;
	}

	return NULL;
}

