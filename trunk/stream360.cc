#include <stdio.h>
#include <stdlib.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <vlc/vlc.h>
#include <ffmpeg/avformat.h>

#include "directory.h"
#include "resource.h"
#include "transcoder.h"

#include <iostream>
#include <sstream>

#define FIFO_NAME "/tmp/lawlercode.wmv"

using namespace std;

UpnpDevice_Handle device_handle;
UpnpDevice_Handle Cookie;

static char* ip_address = NULL;
static unsigned short int port = 0;
Directory* contentDirectory;

struct file_handle {
	int fd;
	Transcoder* tc;
};

Resource* httpd_get_resource(const char* fakefilepath) {
	int num;
	int ret;
	Resource* res = NULL;

	printf("Request for URL: %s\n", fakefilepath);

	ret = sscanf(fakefilepath, "/content/%d", &num);
	if(ret != -1) {
		res = contentDirectory->getResourceByID(num);
	}

	return res;
}

int httpd_file_info(const char* filename, struct File_Info* info) {
	FILE* fp;
	struct stat fstat;
	Resource* res;
	string realfile;
	string realmime;

	res = httpd_get_resource(filename);

	if(res != NULL) {
		realfile = res->getFile();
		realmime = res->getMimeType();

		printf("File requested: %s\n", realfile.c_str());

		if(stat(realfile.c_str(), &fstat) != -1) {
			if(!res->transcode) {
				info->file_length = fstat.st_size;
			}
			else {
				info->file_length = -1;
			}
			info->last_modified = fstat.st_mtime;
			info->is_directory = S_ISDIR(fstat.st_mode);

			fp = fopen(realfile.c_str(), "r");
			if(fp != NULL) {
				info->is_readable = 1;
				fclose(fp);
			}

			info->content_type = ixmlCloneDOMString(realmime.c_str());

			printf("file length=%d isdir=%d type=%s\n", info->file_length, info->is_directory, info->content_type);

			return 0;
		}
	}

	return -1;
}

UpnpWebFileHandle httpd_file_open(const char* filename, enum UpnpOpenFileMode Mode) {
	//FILE* fp = NULL;
	struct file_handle* fh = NULL;
	Resource* res;

	res = httpd_get_resource(filename);
	if(res != NULL) {
		printf("Opening %s\n", res->getFile().c_str());

		fh = (struct file_handle*)calloc(1, sizeof(struct file_handle));
		if(fh != NULL) {
			fh->fd = -1;
			fh->tc = NULL;

			//if(!res->transcode) {
				if(Mode == UPNP_READ) {
					fh->fd = open(res->getFile().c_str(), O_RDONLY);
				}
				else if(Mode == UPNP_WRITE) {
					fh->fd = open(res->getFile().c_str(), O_WRONLY);
				}
			//}
			/*else if(Mode == UPNP_READ) {
				fh->fd = mkfifo(FIFO_NAME, S_IRWXU);

				fh->tc = new Transcoder();
				fh->tc->setInputFile(res->getFile());
				fh->tc->setOutputFile(FIFO_NAME, CODEC_ID_WMV2, CODEC_ID_NONE);
				fh->tc->startTranscoder();
			}*/


			if(fh->fd == -1) {
				free(fh);
				fh = NULL;
			}
		}
	}

	return fh;
}

int httpd_file_read(UpnpWebFileHandle fileHnd, char* buf, size_t buflen) {
	int ret;
	ret = read(((struct file_handle*)fileHnd)->fd, buf, buflen);
	printf("read %x: %d / %d bytes\n", (int)fileHnd, ret, buflen);

	return ret;
}

int httpd_file_write(UpnpWebFileHandle fileHnd, char* buf, size_t buflen) {
	//printf("write %x: %d bytes\n", fileHnd, buflen);
	return write(((struct file_handle*)fileHnd)->fd, buf, buflen);
}

int httpd_file_seek(UpnpWebFileHandle fileHnd, long offset, int origin) {
	//printf("lseek %x: %d %d\n", fileHnd, offset, origin);
	return lseek(((struct file_handle*)fileHnd)->fd, offset, origin);
}

int httpd_file_close(UpnpWebFileHandle fileHnd) {
	struct file_handle* fh = (struct file_handle*) fileHnd;
	int ret = -1;

	close(fh->fd);

	if(fh->tc != NULL) {
		fh->tc->stopTranscoder();
		delete fh->tc;
		fh->tc = NULL;
		unlink(FIFO_NAME);
	}

	return ret;

}

int handleActionRequest(struct Upnp_Action_Request* act_request) {
	printf("UPNP_CONTROL_ACTION_REQUEST: %s\n", act_request->ActionName);

	if(strcmp(act_request->ActionName, "IsAuthorized") == 0) {
		// yes, you are authorized...
		printf("IsAuthorized -> yes\n");
		act_request->ActionResult = ixmlParseBuffer("<m:IsAuthorizedResponse xmlns:m=\"urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1\"><Result>1</Result></m:IsAuthorizedResponse>");
	}
	else if(strcmp(act_request->ActionName, "IsValidated") == 0) {
		// yeah sure, you are validated
		printf("IsValidated -> yes\n");
		act_request->ActionResult = ixmlParseBuffer("<m:IsValidatedResponse xmlns:m=\"urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1\"><Result>1</Result></m:IsValidatedResponse>");
	}
	else if(strcmp(act_request->ActionName, "Search") == 0) {
		contentDirectory->handleSearch(act_request);
	}
	else if(strcmp(act_request->ActionName, "Browse") == 0) {
		contentDirectory->handleBrowse(act_request);
	}
	else {
		printf("Request: %s", ixmlPrintDocument(act_request->ActionRequest));
	}
	return act_request->ErrCode;
}

int handleUpnpEvent(Upnp_EventType EventType, void* Event, void* Cookie) {
	int ret = UPNP_E_SUCCESS;

	switch(EventType) {
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			printf("UPNP_EVENT_SUBSCRIPTION_REQUEST\n");
		case UPNP_CONTROL_GET_VAR_REQUEST:
			printf("UPNP_CONTROL_GET_VAR_REQUEST\n");
		case UPNP_CONTROL_ACTION_REQUEST:
			ret = handleActionRequest((struct Upnp_Action_Request*) Event);
	        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
			printf("UPNP_EVENT_SUBSCRIPTION_REQUEST\n");
			
	        case UPNP_DISCOVERY_SEARCH_RESULT:
			printf("UPNP_DISCOVERY_SEARCH_RESULT\n");
	        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
			printf("UPNP_DISCOVERY_SEARCH_TIMEOUT\n");
	        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
			printf("UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE\n");
	        case UPNP_CONTROL_ACTION_COMPLETE:
			printf("UPNP_CONTROL_ACTION_COMPLETE\n");
	        case UPNP_CONTROL_GET_VAR_COMPLETE:
			printf("UPNP_CONTROL_GET_VAR_COMPLETE\n");
	        case UPNP_EVENT_RECEIVED:
			printf("UPNP_EVENT_RECEIVED\n");
	        case UPNP_EVENT_RENEWAL_COMPLETE:
			printf("UPNP_EVENT_RENEWAL_COMPLETE\n");
	        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
			printf("UPNP_EVENT_SUBSCRIBE_COMPLETE\n");
	        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
			printf("UPNP_EVENT_UNSUBSCRIBE_COMPLETE\n");
		case UPNP_EVENT_AUTORENEWAL_FAILED:
			printf("UPNP_EVENT_AUTORENEWAL_FAILED\n");
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
			printf("UPNP_EVENT_SUBSCRIPTION_EXPIRED\n");
	}

	return ret;
}

int initUpnpServer() {
	int ret = UPNP_E_SUCCESS;
	char web_dir_path[] = "./web";
	char desc_doc_url[1024];
	int default_advr_expire = 100;
	struct UpnpVirtualDirCallbacks callbacks;

	ret = UpnpInit(ip_address, port);
	if(ret != UPNP_E_SUCCESS) {
		printf("Error with UpnpInit -- %d\n", ret);
		UpnpFinish();
		return ret;
	}

	if(ip_address == NULL) {
		ip_address = UpnpGetServerIpAddress();
	}

	if(port == 0) {
		port = UpnpGetServerPort();
	}

	snprintf(desc_doc_url, 1024, "http://%s:%d/description.xml", ip_address, port);

	ret = UpnpSetWebServerRootDir(web_dir_path);
	if(ret != UPNP_E_SUCCESS) {
		printf("Error specifying webserver root directory -- %s: %d", web_dir_path, ret);
		UpnpFinish();
		return ret;
	}

	callbacks.get_info = httpd_file_info;
	callbacks.open = httpd_file_open;
	callbacks.read = httpd_file_read;
	callbacks.write = httpd_file_write;
	callbacks.seek = httpd_file_seek;
	callbacks.close = httpd_file_close;

	ret = UpnpSetVirtualDirCallbacks(&callbacks);
	if(ret != UPNP_E_SUCCESS) {
		printf("Error setting webserver file handlers: %d\n", ret);
	}

	UpnpAddVirtualDir("content");


	ret = UpnpRegisterRootDevice(desc_doc_url, handleUpnpEvent, &Cookie, &device_handle);
	if(ret != UPNP_E_SUCCESS) {
		printf("Error registering the rootdevice %s: %d\n", desc_doc_url, ret);
		UpnpFinish();
		return ret;
	}

	printf("%s\n", desc_doc_url);

	ret = UpnpSendAdvertisement(device_handle, default_advr_expire);
	if(ret != UPNP_E_SUCCESS) {
		printf("Error sending advertisements : %d\n", ret);
		UpnpFinish();
		return ret;
	}

	return UPNP_E_SUCCESS;
}

int trim(char* s, int len) {
	int c = 0;

	// remove any newline characters
	for(int i = len-1; i >= 0 && (s[i] == '\r' || s[i] == '\n'); i--) {
		s[i] = '\0';
		c++;
	}

	return c;
}

int main(int argc, char* argv[]) {
	FILE* fp;
	char buf[4096];

	av_register_all(); // init ffmpeg

	// create the content directory
	contentDirectory = new Directory();

	// search these paths for files
	fp = fopen("stream360.cfg", "r");
	if(fp == NULL) {
		fprintf(stderr, "Error loading stream360.cfg\n");
		return 1;
	}

	while(!feof(fp)) {
		if(fgets(buf, 4096, fp) != NULL && buf[0] != '#') {
			trim(buf, strlen(buf));
			contentDirectory->addFolder(buf);
		}
	}

	if(initUpnpServer() != UPNP_E_SUCCESS) {
		fprintf(stderr, "Error starting Upnp\n");
		return 1;
	}

	while(1) {
		// should probably be some type of thread join
		sleep(60);
	}

	return 0;
}

