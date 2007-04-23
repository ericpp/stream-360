#ifndef _XMLUTILS_H
#define _XMLUTILS_H

#include <upnp/ixml.h>
#include <string>

IXML_Element* makeElement(IXML_Document* doc, IXML_Node* parentNode, const char* tagName, const char* value);
std::string getChildValue(IXML_Element* element, const char* childTagName);

#endif
