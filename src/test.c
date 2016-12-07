#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dropbox.h>
#include <memStream.h>
#include "../Yandex-C/include/yandex.h"
#include "../Google-C/include/google.h"
#include "cloud.h"


void displayAccountInfo(drbAccountInfo *info);
void displayMetadata(drbMetadata *meta, char *title);
void displayMetadataList(drbMetadataList* list, char* title);

int main(int argc, char **argv)
{
    char *c_key    = "i58xh5qigf55ma1";
    char *c_secret = "a9iwynwq94nyuca";

    char *t_key    = NULL;
    char *t_secret = NULL;

    while(1) {
        int service_choice;

        printf("Chose service\n0. Demonstrate cloud\n1. Dropbox\n2. Yandex drive\n3. Google drive\n4. Exit\n");
        scanf("%d", &service_choice);
        if(service_choice == 0) {
            connection *conn = createConnection("dropbox");
            deleteFile(conn);
        }
        else if(service_choice == 1) {
            void* output;
            int err;
            drbInit();
            drbClient* cli = drbCreateClient(c_key, c_secret, t_key, t_secret);

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
                        while(1) {
                            printf("1. Get account info\n2. Download file\n3. Create file\n4. Delete file\n5. Return\n");

                            int choice;
                            scanf("%d", &choice);

                            if(choice == 1) {
                                output = NULL;
                                err = drbGetAccountInfo(cli, &output, DRBOPT_END);
                                if (err != DRBERR_OK) {
                                    printf("Account info error (%d): %s\n", err, (char*)output);
                                    free(output);
                                } else {
                                    drbAccountInfo* info = (drbAccountInfo*)output;
                                    displayAccountInfo(info);
                                    drbDestroyAccountInfo(info);
                                }
                            }
                            else if(choice == 2) {
                                // char filename[256];
                                // printf("Enter filename: \n");
                                // scanf("%s", filename);

                                // char path[256];
                                // printf("Enter the path to dropbox file: \n");
                                // scanf("%s", path);

                                FILE *file = fopen("example.txt", "w"); // Write it in this file
                                output = NULL;
                                err = drbGetFile(cli, &output,
                                                 DRBOPT_PATH, "/hello.txt",
                                                 DRBOPT_IO_DATA, file,
                                                 DRBOPT_IO_FUNC, fwrite,
                                                 DRBOPT_END);
                                fclose(file);

                                if (err != DRBERR_OK) {
                                    printf("Get File error (%d): %s\n", err, (char*)output);
                                    free(output);
                                } else {
                                    displayMetadata(output, "Get File Result");
                                    drbDestroyMetadata(output, true);
                                }
                            }
                            else if(choice == 3) {
                                char filename[256];
                                printf("Enter filename, you want to create: \n");
                                scanf("%s", filename);

                                char content[1024];
                                printf("Enter the content of your file: \n");
                                scanf("%s", content);

                                memStream stream; memStreamInit(&stream);
                                stream.data = content;
                                stream.size = strlen(stream.data);
                                err = drbPutFile(cli, NULL,
                                                 DRBOPT_PATH, filename,
                                                 DRBOPT_IO_DATA, &stream,
                                                 DRBOPT_IO_FUNC, memStreamRead,
                                                 DRBOPT_END);
                            }
                            else if(choice == 4) {
                                output = NULL;
                                err = drbDelete(cli, &output, DRBOPT_PATH, "/please.txt", DRBOPT_END);

                                if (err != DRBERR_OK) {
                                    printf("Get File error (%d): %s\n", err, (char*)output);
                                    free(output);
                                } else {
                                    displayMetadata(output, "Get File Result");
                                    drbDestroyMetadata(output, true);
                                }
                            }
                            else if(choice == 5) {
                                break;
                            }
                        }
                    } else{
                        fprintf(stderr, "Failed to obtain an AccessToken...\n");
                        return EXIT_FAILURE;
                    }
                } else {
                    fprintf(stderr, "Failed to obtain a RequestToken...\n");
                    return EXIT_FAILURE;
                }
            }
        }
        else if(service_choice == 2) {
            if(yandex_auth()) {
                while (1) {
                    printf("Chose action\n1. Get files on yandex drive\n2. Download file from yandex drive\n"
                        "3. Upload file to yandex drive\n4. Delete file from yandex drive\n5. Return\n");

                    int choice = 0;
                    scanf("%d", &choice);

                    if(choice == 1) {
                        if(!get_user_files_on_yandex_drive()) {
                            printf("Error!\n");
                        }
                    }
                    else if(choice == 2) {
                        if(download_file_from_yandex_drive()) {
                            printf("File successfully downloaded\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    else if(choice == 3) {
                        char filename[256];
                        printf("Enter filename, you want to upload: \n");
                        scanf("%s", filename);
                        if(upload_to_yandex_disk(filename)) {
                            printf("File successfully uploaded to you yandex drive!\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    else if(choice == 4) {
                        if(delete_file_from_yandex_drive()) {
                            printf("File successfully deleted from you yandex drive!\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    else if(choice == 5) {
                        break;
                    }
                }
            }
            else {
                printf("Authorization error. Try again later!\n");
            }
        }
        else if(service_choice == 3) {
            if(google_drive_auth()) {
                while(1) {
                    printf("1. Upload file to the google drive.\n2. Download file from the google drive\n"
                           "3. Delete file from the google drive\n4. Get user files on google drive\n5. Return\n");

                    int choice;
                    scanf("%d", &choice);

                    if(choice == 1) {
                        if(upload_to_google_drive("piw.txt")) {
                            printf("File is successfully uploaded!\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    if(choice == 2) {
                        if(download_file_from_google_drive()) {
                            printf("File is successfully downloaded!\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    if(choice == 3) {
                        if(delete_file_from_google_drive()) {
                            printf("File is successfully deleted!\n");
                        }
                        else {
                            printf("Error!\n");
                        }
                    }
                    if(choice == 4) {
                        if(!get_user_files_on_google_drive()) {
                            printf("Error!\n");
                        }
                    }
                    else if(choice == 5) {
                        break;
                    }
                }
            }
            else {
                printf("Authorization error. Try again later!\n");
            }
        }
        else if(service_choice == 4) {
            break;
        }
        else {
            printf("No such option, try again\n");
        }
    }

    // yandex_auth();
    //upload_to_yandex_disk("quickstart.py");
    // download_file_from_yandex_drive();
    //delete_file_from_yandex_drive();
    // get_user_files_on_yandex_drive();
    return 0;
}

char* strFromBool(bool b) { return b ? "true" : "false"; }


/*!
 * \brief  Display a drbAccountInfo item in stdout.
 * \param  info    account informations to display.
 * \return  void
 */
void displayAccountInfo(drbAccountInfo* info) {
    if(info) {
        printf("---------[ Account info ]---------\n");
        if(info->referralLink)         printf("referralLink: %s\n", info->referralLink);
        if(info->displayName)          printf("displayName:  %s\n", info->displayName);
        if(info->uid)                  printf("uid:          %d\n", *info->uid);
        if(info->country)              printf("country:      %s\n", info->country);
        if(info->email)                printf("email:        %s\n", info->email);
        if(info->quotaInfo.datastores) printf("datastores:   %u\n", *info->quotaInfo.datastores);
        if(info->quotaInfo.shared)     printf("shared:       %u\n", *info->quotaInfo.shared);
        if(info->quotaInfo.quota)      printf("quota:        %u\n", *info->quotaInfo.quota);
        if(info->quotaInfo.normal)     printf("normal:       %u\n", *info->quotaInfo.normal);
    }
}

void displayMetadata(drbMetadata* meta, char* title) {
    if (meta) {
        if(title) printf("---------[ %s ]---------\n", title);
        if(meta->hash)        printf("hash:        %s\n", meta->hash);
        if(meta->rev)         printf("rev:         %s\n", meta->rev);
        if(meta->thumbExists) printf("thumbExists: %s\n", strFromBool(*meta->thumbExists));
        if(meta->bytes)       printf("bytes:       %d\n", *meta->bytes);
        if(meta->modified)    printf("modified:    %s\n", meta->modified);
        if(meta->path)        printf("path:        %s\n", meta->path);
        if(meta->isDir)       printf("isDir:       %s\n", strFromBool(*meta->isDir));
        if(meta->icon)        printf("icon:        %s\n", meta->icon);
        if(meta->root)        printf("root:        %s\n", meta->root);
        if(meta->size)        printf("size:        %s\n", meta->size);
        if(meta->clientMtime) printf("clientMtime: %s\n", meta->clientMtime);
        if(meta->isDeleted)   printf("isDeleted:   %s\n", strFromBool(*meta->isDeleted));
        if(meta->mimeType)    printf("mimeType:    %s\n", meta->mimeType);
        if(meta->revision)    printf("revision:    %d\n", *meta->revision);
        if(meta->contents)    displayMetadataList(meta->contents, "Contents");
    }
}

/*!
 * \brief  Display a drbMetadataList item in stdout.
 * \param  list    list to display.
 * \param   title   display the title before the list.
 * \return  void
 */
void displayMetadataList(drbMetadataList* list, char* title) {
    if (list){
        printf("---------[ %s ]---------\n", title);
        for (int i = 0; i < list->size; i++) {

            displayMetadata(list->array[i], list->array[i]->path);
        }
    }
}
