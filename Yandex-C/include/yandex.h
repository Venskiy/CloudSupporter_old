#ifndef yandex_h__
#define yandex_h__

extern int yandex_auth();
extern int upload_to_yandex_disk(char *filename);
extern int download_file_from_yandex_drive();
extern int delete_file_from_yandex_drive();
extern int get_user_files_on_yandex_drive();

#endif
