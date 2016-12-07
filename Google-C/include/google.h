#ifndef google_h__
#define google_h__

extern int google_drive_auth();
extern int upload_to_google_drive(char *filename);
extern int download_file_from_google_drive();
extern int delete_file_from_google_drive();
extern int get_user_files_on_google_drive();

#endif