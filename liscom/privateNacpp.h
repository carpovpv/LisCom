/*****************************************************************
    Интерфейсный класс для взаимодействия сторонних медицинских
    информационных систем с лабораторной информационной системой
    лаборатории ООО Национальное Агентство Клинической Фармакологии
    и Фармации (НАКФФ). Цель класса - упростить реализацию проктола
    взаимодействия. Постоянный адрес протокола:
            https://nacpp.info/api.pdf
    IT-отдел лаборатории.
    Ответственный: Карпов П.В.
    E-mail: carpovpv@mail.ru
******************************************************************/

#ifndef PRIVATENACPP_H
#define PRIVATENACPP_H

#include <sys/types.h>

#ifndef WIN32
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#include "request.h"
#include "http_parser.h"
#include "uricodec.h"

#include "../nacppInterface.h"
#include <iostream>

#define SERVER  "nacpp.info"
#define PORT 443

class PrivateNacpp
{
public:

    PrivateNacpp(const std::string & login, const std::string & password, int *isError);
    ~PrivateNacpp();

    int LoginToNacpp(const std::string & login, const std::string & password);
    int LogoutFromNacpp();

    char* GetDictionary(const char* dict, int* isError);
    char* GetFreeOrders(int num, int* isError);
    char* GetResults(const char *folderno, int* isError);
    char* GetPending(int* isError);
    char* CreateOrder(const char* message, int* isError);
    char* DeleteOrder(const char* folderno, int* isError);
    char* EditOrder(const char* message, int* isError);

    int GetPrintResult(const char* folderno, LPCWSTR filePath = L"");
private:
    typedef struct {
        int socket;
        SSL *sslHandle;
        SSL_CTX *sslContext;
    } Connection;

    Connection* conn;
    std::string sessionId;

    int tcpConnect();

    int sslConnect(Connection ** c);
    void sslDisconnect (Connection *c);
    int sslRead (SSL * ssl, Request & params);
};


#endif // PRIVATENACPP_H
