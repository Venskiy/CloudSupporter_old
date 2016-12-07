#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <curl/easy.h>

static const char* YANDEX_APP_CLIENT_ID = "ae423f4f7c8f441caf771a8705152647";
static const char* YANDEX_APP_CLIENT_SECRET = "6df4dc63b3e444289fb6df82d29922c9";

static const char* YANDEX_AUTHORIZATION_LINK = "https://oauth.yandex.ru/authorize?response_type=code&client_id=ae423f4f7c8f441caf771a8705152647";
static const char* YANDEX_AUTH_URL = "https://oauth.yandex.ru/token";
static const char* YANDEX_GET_UPLOAD_LINK_URL = "https://cloud-api.yandex.net/v1/disk/resources/upload?path=%2F";
static const char* YANDEX_GET_DOWNLOAD_LINK_URL = "https://cloud-api.yandex.net/v1/disk/resources/download?path=%2F";
static const char* YANDEX_DELETE_FILE_FROM_DRIVE = "https://cloud-api.yandex.net/v1/disk/resources?path=%2F";
static const char* GET_USER_FILES_ON_YANDEX_DRIVE_URL = "https://cloud-api.yandex.net/v1/disk/resources/files";

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

size_t write_data_to_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
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

void displayUserFiles(json_t* root) {
    json_t* files_array = json_object_get(root, "items");
    /* array is a JSON array */
    size_t index;
    json_t *value;

    json_array_foreach(files_array, index, value) {
        printf("=====================================\n");
        char *name = jsonGetStr(value, "name");
        char *created = jsonGetStr(value, "created");
        char *modified = jsonGetStr(value, "modified");
        char *path = jsonGetStr(value, "path");
        char *type = jsonGetStr(value, "type");
        char *mime_type = jsonGetStr(value, "mime_type");

        printf("Name: %s\nCreated: %s\nModified: %s\nPath: %s\nType: %s\nMime_type: %s\n",
            name, created, modified, path, type, mime_type);
    }
    printf("=====================================\n");
}

void write_token_to_the_file(char *token) {
    FILE *f = fopen("yandex-token.txt", "w");
    if (f == NULL)
    {
      printf("Error opening file!\n");
      exit(1);
    }

    fprintf(f, "%s", token);

    fclose(f);
}

int yandex_auth() {
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
        char yandex_code[7];
        printf("Please, visit the link below to auth in our app.\n");
        printf("%s\n", YANDEX_AUTHORIZATION_LINK);
        printf("Enter your code: \n");
        scanf("%s", yandex_code);

        char postfields[256];
        sprintf(postfields, "grant_type=authorization_code&code=%s&client_id=%s&client_secret=%s", yandex_code, YANDEX_APP_CLIENT_ID, YANDEX_APP_CLIENT_SECRET);

        curl_easy_setopt(curl, CURLOPT_URL, YANDEX_AUTH_URL);
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
    write_token_to_the_file(token);

    json_decref(root);
    return 1;
}

char* get_upload_file_url() {
  struct curl_slist *chunk = NULL;

  char auth_header[512];
  char token[256];

  FILE *f;
  f = fopen("yandex-token.txt", "r");
  fscanf(f, "%s", token);
  fclose(f);

  sprintf(auth_header, "Authorization: OAuth %s", token);

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
    char filename[256];
    printf("Enter filename on yandex drive: \n");
    scanf("%s", filename);

    char url[512];
    sprintf(url, "%s%s", YANDEX_GET_UPLOAD_LINK_URL, filename);
    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl, CURLOPT_URL, url);
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

  char* pStr = jsonGetStr(root, "href");

  return pStr;
}

int upload_to_yandex_disk(char *filename) {
  CURL *curl;
  CURLcode res;
  FILE * hd_src;
  struct stat file_info;

  char *url;

  url = get_upload_file_url();
  if(url == NULL) {
      return 0;
  }

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

    /* enable uploading */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* HTTP PUT please */
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);

    /* specify target URL, and note that this URL should include a file
       name, not only a directory */
    curl_easy_setopt(curl, CURLOPT_URL, url);

    /* now specify which file to upload */
    curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

    /* provide the size of the upload, we specicially typecast the value
       to curl_off_t since we must be sure to use the correct data size */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);

    /* Now run off and do what you've been told! */
    res = curl_easy_perform(curl);
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

char* get_download_file_url() {
    struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("yandex-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: OAuth %s", token);

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
      char filename[256];
      printf("Enter filename on yandex drive: \n");
      scanf("%s", filename);

      char url[512];
      sprintf(url, "%s%s", YANDEX_GET_DOWNLOAD_LINK_URL, filename);
      /* First set the URL that is about to receive our POST. This URL can
         just as well be a https:// URL if that is what should receive the
         data. */
      curl_easy_setopt(curl, CURLOPT_URL, url);
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

    char* pStr = jsonGetStr(root, "href");

    return pStr;
}

int download_file_from_yandex_drive() {
    CURL *curl;
    FILE *fp;
    char *url = get_download_file_url();

    if(url == NULL) {
        return 0;
    }

    char outfilename[512];

    printf("Enter the name of the downloaded file: \n");
    scanf("%s", outfilename);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 1;
}

int delete_file_from_yandex_drive() {
    struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("yandex-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: OAuth %s", token);

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
        printf("Enter filename on yandex drive, you want to delete: \n");
        scanf("%s", filename);

        char url[512];
        sprintf(url, "%s%s", YANDEX_DELETE_FILE_FROM_DRIVE, filename);

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

int get_user_files_on_yandex_drive() {
    struct curl_slist *chunk = NULL;

    char auth_header[512];
    char token[256];

    FILE *f;
    f = fopen("yandex-token.txt", "r");
    fscanf(f, "%s", token);
    fclose(f);

    sprintf(auth_header, "Authorization: OAuth %s", token);

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
        curl_easy_setopt(curl, CURLOPT_URL, GET_USER_FILES_ON_YANDEX_DRIVE_URL);
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

    displayUserFiles(root);

    json_decref(root);
    return 1;
}
