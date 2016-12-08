#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <curl/easy.h>

static const char* GOOGLE_APP_CLIENT_ID = "YOUR GOOGLE APP CLIENT ID";
static const char* GOOGLE_APP_CLIENT_SECRET = "YOUR GOOGLE APP SECRET";

static const char* GOOGLE_AUTHORIZATION_LINK = "https://accounts.google.com/AccountChooser?continue=https://accounts.google.com/o/oauth2/v2/auth?scope%3Dhttps://www.googleapis.com/auth/drive%26response_type%3Dcode%26state%3D/profile%26redirect_uri%3Dhttp://localhost%26client_id%3D595393500053-fu5fbq4qq51m8jqc3cjc8csgmoa3hnfk.apps.googleusercontent.com%26from_login%3D1%26as%3D256e4d775bf1b4&ltmpl=nosignup&btmpl=authsub&scc=1&oauth=1";
static const char* GOOGLE_AUTH_URL = "https://www.googleapis.com/oauth2/v4/token";
static const char* GOOGLE_UPLOAD_URL = "https://www.googleapis.com/upload/drive/v3/files?uploadType=media";
static const char* GET_USER_FILES_ON_GOOGLE_DRIVE_URL = "https://www.googleapis.com/drive/v2/files";
static const char* GOOGLE_DELETE_FILE_FROM_DRIVE = "https://www.googleapis.com/drive/v2/files/";

struct url_data {
    size_t size;
    char* data;
};

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;

  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  retcode = fread(ptr, size, nmemb, stream);


  // fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
  //        " bytes from file\n", nread);

  return retcode;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);

#ifdef DEBUG
    fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif
    tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

char *handle_url(char* url) {
    CURL *curl;

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    data.data[0] = '\0';

    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);

    }
    return data.data;
}

static char* jsonGetStr(json_t* root, char* key) {
    char* pStr = NULL;
    if(json_unpack(root, "{ss}", key, &pStr) != -1) {
        pStr = strdup(pStr);
    }
    return pStr;
}

void write_google_token_to_the_file(char *token) {
    FILE *f = fopen("google-token.txt", "w");
    if (f == NULL)
    {
      printf("Error opening file!\n");
      exit(1);
    }

    fprintf(f, "%s", token);

    fclose(f);
}

void displayUserGoogleDriveFiles(json_t* root) {
    json_t* files_array = json_object_get(root, "items");
    /* array is a JSON array */
    size_t index;
    json_t *value;

    json_array_foreach(files_array, index, value) {
        printf("=====================================\n");
        char *id = jsonGetStr(value, "id");
        char *kind = jsonGetStr(value, "kind");
        char *title = jsonGetStr(value, "title");
        printf("Id: %s\nKind: %s\nTitle: %s\n", id, kind, title);
    }
    printf("=====================================\n");
}

char *get_id_by_filename(char *filename) {
	struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("google-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: Bearer %s", token);

    chunk = curl_slist_append(chunk, auth_header);

    CURL *curl;
    CURLcode res;

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    data.data[0] = '\0';

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        curl_easy_setopt(curl, CURLOPT_URL, GET_USER_FILES_ON_GOOGLE_DRIVE_URL);
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));

        // printf("%s\n", data.data);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    json_t *root = json_loads(data.data, 0, NULL);

    char *error = jsonGetStr(root, "error");
    if(error != NULL) {
        printf("Error: %s\n", error);
        return NULL;
    }

    json_t* files_array = json_object_get(root, "items");
    /* array is a JSON array */
    size_t index;
    json_t *value;

    json_array_foreach(files_array, index, value) {
        char *title = jsonGetStr(value, "title");
        if(strcmp (filename, title) == 0) {
        	printf("found\n");
        	return jsonGetStr(value, "id");
        }
    }

    json_decref(root);
    return NULL;
}

int google_drive_auth() {
    CURL *curl;
    CURLcode res;

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    data.data[0] = '\0';

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        char google_code[256];
        printf("Please, visit the link below to auth in our app.\n");
        printf("%s\n", GOOGLE_AUTHORIZATION_LINK);
        printf("Enter your code: \n");
        scanf("%s", google_code);

        char postfields[1024];
        sprintf(postfields, "code=%s&client_id=%s&client_secret=%s&redirect_uri=http://localhost&grant_type=authorization_code", google_code, GOOGLE_APP_CLIENT_ID, GOOGLE_APP_CLIENT_SECRET);

        curl_easy_setopt(curl, CURLOPT_URL, GOOGLE_AUTH_URL);
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));

        // printf("%s\n", data.data);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    json_t *root = json_loads(data.data, 0, NULL);

    char *error = jsonGetStr(root, "error");
    if(error != NULL) {
        char *error_description = jsonGetStr(root, "error_description");
        printf("Auth error: %s\n", error_description);
        return 0;
    }
    char *token = jsonGetStr(root, "access_token");
    write_google_token_to_the_file(token);

    json_decref(root);
    return 1;
}

int upload_to_google_drive(char *filename) {
	struct curl_slist *chunk = NULL;

	char auth_header[512];
	char token[256];

	FILE *f;
	f = fopen("google-token.txt", "r");
	fscanf(f, "%s", token);
	fclose(f);

	sprintf(auth_header, "Authorization: Bearer %s", token);

	chunk = curl_slist_append(chunk, auth_header);


	CURL *curl;
	CURLcode res;
	FILE * hd_src;
	struct stat file_info;

	/* get the file size of the local file */
	stat(filename, &file_info);

	/* get a FILE * of the same file, could also be made with
	 fdopen() from the previous descriptor, but hey this is just
	 an example! */
	hd_src = fopen(filename, "rb");

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) {
		/* we want to use our own read function */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	    /* HTTP PUT please */
	    curl_easy_setopt(curl, CURLOPT_PUT, 1L);

		/* specify target URL, and note that this URL should include a file
		   name, not only a directory */
		curl_easy_setopt(curl, CURLOPT_URL, GOOGLE_UPLOAD_URL);

		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

		/* provide the size of the upload, we specicially typecast the value
		   to curl_off_t since we must be sure to use the correct data size */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
		                 (curl_off_t)file_info.st_size);

		printf("dsadsa\n");
		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);
		printf("dsasd\n");
		/* Check for errors */
		if(res != CURLE_OK)
		  fprintf(stderr, "curl_easy_perform() failed: %s\n",
		          curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	fclose(hd_src); /* close the local file */

	curl_global_cleanup();
	return 1;
}

int download_file_from_google_drive() {
	return 1;
}

int delete_file_from_google_drive() {
	struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("google-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: Bearer %s", token);

    chunk = curl_slist_append(chunk, auth_header);

    CURL *curl;
    CURLcode res;

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    data.data[0] = '\0';

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        char filename[256];
        printf("Enter filename, you want to delete: \n");
        scanf("%s", filename);

        char *file_id = get_id_by_filename(filename);
        printf("%s\n", file_id);

        char url[512];
        sprintf(url, "%s%s", GOOGLE_DELETE_FILE_FROM_DRIVE, file_id);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));

        // printf("%s\n", data.data);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    json_t *root = json_loads(data.data, 0, NULL);

    char *error = jsonGetStr(root, "error");
    if(error != NULL) {
        printf("Delete error: %s\n", error);
        return 0;
    }

    json_decref(root);
    return 1;
}

int get_user_files_on_google_drive() {
	struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("google-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: Bearer %s", token);

    chunk = curl_slist_append(chunk, auth_header);

    CURL *curl;
    CURLcode res;

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    data.data[0] = '\0';

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        curl_easy_setopt(curl, CURLOPT_URL, GET_USER_FILES_ON_GOOGLE_DRIVE_URL);
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));

        // printf("%s\n", data.data);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    json_t *root = json_loads(data.data, 0, NULL);

    char *error = jsonGetStr(root, "error");
    if(error != NULL) {
        printf("Error: %s\n", error);
        return 0;
    }

    displayUserGoogleDriveFiles(root);

    json_decref(root);
    return 1;
}
