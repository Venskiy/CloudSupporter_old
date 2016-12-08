#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dropbox.h>
#include <memStream.h>
#include "../Yandex-C/include/yandex.h"
#include "../Google-C/include/google.h"
#include "cloud.h"

char *c_key    = "YOUR DROPBOX APP KEY";
char *c_secret = "YOUR DROPBOX APP SECRET";

char *t_key    = NULL;
char *t_secret = NULL;

void* output;
int err;
drbClient* cli;



connection* createConnection(char* service) {
	connection *conn = NULL;
	if((conn = malloc(sizeof(connection))) != NULL) {
		conn->service = strdup(service);
	}

	if(strcmp(conn->service, "google") == 0) {
		google_drive_auth();
	}
	else if(strcmp(conn->service, "yandex") == 0) {
		yandex_auth();
	}
	else if(strcmp(conn->service, "dropbox") == 0) {
	    drbInit();
	    cli = drbCreateClient(c_key, c_secret, t_key, t_secret);

	    if(!t_key || !t_secret) {
	        drbOAuthToken* reqTok = drbObtainRequestToken(cli);

	        if (reqTok) {
	            char* url = drbBuildAuthorizeUrl(reqTok);
	            printf("Please visit %s\nThen press Enter...\n", url);
	            free(url);
	            fgetc(stdin);
	            fgetc(stdin);

	            drbOAuthToken* accTok = drbObtainAccessToken(cli);

	            if (accTok) {
	                drbSetDefault(cli, DRBOPT_ROOT, DRBVAL_ROOT_AUTO, DRBOPT_END);
	            } else{
	                fprintf(stderr, "Failed to obtain an AccessToken...\n");
	                return NULL;
	            }
	        } else {
	            fprintf(stderr, "Failed to obtain a RequestToken...\n");
	            return NULL;
	        }
	    }
    }
	return conn;
}

int deleteFile(connection *conn) {
	if(strcmp(conn->service, "google") == 0) {
		delete_file_from_google_drive();
	}
	else if(strcmp(conn->service, "yandex") == 0) {
		delete_file_from_yandex_drive();
	}
	else if(strcmp(conn->service, "dropbox") == 0) {
		output = NULL;
		err = drbDelete(cli, &output, DRBOPT_PATH, "hello.txt", DRBOPT_END);

		if (err != DRBERR_OK) {
			printf("Get File error (%d): %s\n", err, (char*)output);
			free(output);
		} else {
			printf("File was successfully deleted!\n");
		}
	}
	return 1;
}
