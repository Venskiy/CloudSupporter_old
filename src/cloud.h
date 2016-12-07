#ifndef cloud_h__
#define cloud_h__

typedef struct {
    char* service;
} connection;

connection* createConnection(char* service);
int deleteFile(connection *conn);

#endif