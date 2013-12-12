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
#include "privateNacpp.h"

char * PrivateNacpp::copyString(const char *mes)
{
    char * p = strdup(mes);
    pool.push_back(p);
    return p;
}

PrivateNacpp::PrivateNacpp(const std::string &login,
                           const std::string & password,
                           int * isError)
{
    conn = NULL;
    int res = sslConnect(&conn);
    if(res != ERROR_NO)
        *isError = res;
    else
        *isError = LoginToNacpp(login, password);

    m_login = login;
    m_password = password;

}

PrivateNacpp::~PrivateNacpp()
{
    LogoutFromNacpp();
    sslDisconnect(conn);

    for(int i=0; i< pool.size(); i++)
       free(pool[i]);

#ifdef WIN32
    WSACleanup();
#endif

}

int PrivateNacpp::tcpConnect()
{

    int error;
    int handle;
    struct hostent * host;
    struct sockaddr_in server;

    host = gethostbyname (SERVER);
    handle = socket (AF_INET, SOCK_STREAM, 0);
    if (handle == -1)
    {
        fprintf(stderr, "Socket failed: %d %s.\n",
                errno, strerror(errno));
        handle = 0;
    }
    else
    {
        server.sin_family = AF_INET;
        server.sin_port = htons (PORT);
        server.sin_addr = *((struct in_addr *) host->h_addr);
        memset (&(server.sin_zero),0, 8);

        error = connect (handle, (struct sockaddr *) &server,
                         sizeof (struct sockaddr));
        if (error == -1)
        {
            fprintf(stderr, "Connect failed: %d %s.\n",
                    errno, strerror(errno));
            handle = 0;
        }
    }
    return handle;
}

int PrivateNacpp::sslConnect (Connection ** c)
{
#ifdef WIN32
    WSADATA wsaData = {0};
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0)
    {
        printf("WSAStartup failed: %d\n", res);
        return ERROR_SOCKET;
    }
#endif

    int handle = tcpConnect();
    if(handle == 0)
        return ERROR_SOCKET;

    SSL * sslHandle = NULL;
    SSL_CTX * sslContext = NULL;

    SSL_load_error_strings ();
    SSL_library_init ();

    sslContext = SSL_CTX_new (SSLv23_client_method ());
    if (sslContext == NULL)
    {
        //ERR_print_errors_fp (stderr);
        return ERROR_SSLCONTEXT;
    }

    sslHandle = SSL_new (sslContext);
    if (sslHandle == NULL)
    {
        //ERR_print_errors_fp (stderr);
        SSL_CTX_free (sslContext);

        return ERROR_SSLHANDLE;
    }
    if (!SSL_set_fd (sslHandle, handle))
    {
        //ERR_print_errors_fp (stderr);

        SSL_CTX_free (sslContext);
        SSL_free (sslHandle);

        return ERROR_SSLSETFD;
    }
    if (SSL_connect (sslHandle) != 1)
    {
        //ERR_print_errors_fp (stderr);

        SSL_CTX_free (sslContext);
        SSL_free (sslHandle);

        return ERROR_SSLCONNECT;
    }

    *c = (Connection * ) malloc (sizeof (Connection));

    assert(c);

    (*c)->socket = handle;
    (*c)->sslContext = sslContext;
    (*c)->sslHandle = sslHandle;

    return ERROR_NO;
}

void PrivateNacpp::sslDisconnect (Connection *c)
{

    if(c == NULL)
        return;

    if (c->sslHandle)
    {
        SSL_shutdown (c->sslHandle);
        SSL_free (c->sslHandle);
    }

    if (c->sslContext)
        SSL_CTX_free (c->sslContext);

    if (c->socket)
        close (c->socket);

    free (c);
}

int PrivateNacpp::sslRead (SSL * ssl, Request & params)
{

    char mes[BUFSIZ];
    ssize_t rcount;

    size_t meslen = 0;

    //Ставим ограничение на размер сообщения в 1Мб
    const size_t max_mes_len = 1024 * 1024 ;

    while(true)
    {
        if(params.isDone())
            break;

        rcount = SSL_read(ssl, mes, BUFSIZ-1);
        if(rcount <= 0)
            return ERROR_COMMUNICATION;

        mes[rcount] = '\0';
        int nparsed = http_parser_execute(params.parser,
                                          &params.settings,
                                          mes,
                                          rcount);
        if(nparsed != rcount)
            return ERROR_HTTP_PARSER;

        meslen += rcount;
        if(meslen > max_mes_len)
            return ERROR_TOO_BIG_MESSAGE;
    }

    return ERROR_NO;
}

int PrivateNacpp::LoginToNacpp(const std::string & login, const std::string & password)
{
    SSL *ssl = conn->sslHandle;

    assert(ssl);
    assert(!login.empty());
    assert(!password.empty());

    std::string request = "POST /login.php HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Content-Type: application/x-www-form-urlencoded\r\n"
                          "Content-Length:";
    std::string authorization = "login=" + login + "&password=" + password;

    char buf[20];
    sprintf(buf, "%d", authorization.length());

    request += std::string(buf) + "\r\n\r\n" + authorization;

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
        return ERROR_COMMUNICATION;

    Request params(ssl);
    int res = sslRead (ssl, params);

    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 302)
        {
            std::map<std::string, std::string>::const_iterator it;
            const std::map<std::string, std::string> & headers = params.getHeaders();
            for(it = headers.begin(); it != headers.end(); ++it)
            {
                if((*it).first == "Set-Cookie" &&
                        strncmp((*it).second.c_str(), "PHPSESSID",9) == 0)
                {
                    const char * start = (*it).second.c_str() + 10;
                    if(strlen(start) >= 32)
                    {
                        sessionId = std::string(start, 32);
                        return ERROR_NO;
                    }
                }
            }
        }
        return ERROR_LOGIN;
    }

    return res;
}

int PrivateNacpp::LogoutFromNacpp()
{
    if(conn == NULL)
        return 0;

    SSL *ssl = conn->sslHandle;
    std::string request = "GET /logout.php HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\n\r\n";

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
        return ERROR_COMMUNICATION;

    Request params(ssl);
    return sslRead (ssl, params);

}

void PrivateNacpp::Reconnect(int * isError)
{
    sslDisconnect(conn);

#ifdef WIN32
    WSACleanup();
#endif

    conn = NULL;
    int res = sslConnect(&conn);
    if(res != ERROR_NO)
        *isError = res;
    else
        *isError = LoginToNacpp(m_login, m_password);
}

char* PrivateNacpp::GetDictionary(const char* dict, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    bool isValidDict = false;
    std::string valid_dicts[] = {"bio", "tests", "containertypes", "panels"};
    for (int i = 0;
            i < sizeof(valid_dicts) / sizeof(std::string);
            i++)
    {
        if (dict == valid_dicts[i])
        {
            isValidDict = true;
            break;
        }
    }

    if(!isValidDict)
    {
        *isError = ERROR_UNKNOWN_DICT;
        return NULL;
    }

    std::string request = "GET /plugins/index.php?act=get-catalog&catalog=" + (std::string)dict + " HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\n\r\n";

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    else
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::GetFreeOrders(int num, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    if (num > 1000)
    {
        *isError = ERROR_MORE_POOL_NUM;
        return NULL;
    }

    char buf[20];
    sprintf(buf, "%d", num);

    std::string request = "GET /plugins/index.php?act=free-orders&n=" + std::string(buf) + " HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\n\r\n";

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::GetResults(const char* folderno, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    std::string request = "POST /plugins/index.php?act=request-result HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\nContent-Type: application/x-www-form-urlencoded\r\n";

    std::string message = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                          "<request>"
                          "<orderno>" + (std::string)folderno + "</orderno>"
                          "</request>";

    char buf[20];
    sprintf(buf, "%d", message.length());
    request += "Content-Length: " + std::string(buf) + "\r\n\r\n";

    request += message;

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::GetPending(int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    std::string request = "GET /plugins/index.php?act=pending HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\n\r\n";

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::CreateOrder(const char* message, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    std::string request = "POST /plugins/index.php?act=request-add HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\nContent-Type: application/x-www-form-urlencoded\r\n";

    char buf[20];
    sprintf(buf, "%d", strlen(message));
    request += "Content-Length: " + std::string(buf) + "\r\n\r\n";

    request += (std::string)message;

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::DeleteOrder(const char* folderno, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    std::string request = "POST /plugins/index.php?act=request-delete HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\nContent-Type: application/x-www-form-urlencoded\r\n";

    std::string message = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                          "<request>"
                          "<order orderno=\"" + (std::string)folderno + "\" action=\"delete\"/>"
                          "</request>";

    char buf[20];
    sprintf(buf, "%d", message.length());
    request += "Content-Length: " + std::string(buf) + "\r\n\r\n";

    request += (std::string)message;

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

char* PrivateNacpp::EditOrder(const char* message, int* isError)
{
    SSL *ssl = conn->sslHandle;
    std::string response;

    std::string request = "POST /plugins/index.php?act=request-edit HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\nContent-Type: application/x-www-form-urlencoded\r\n";

    char buf[20];
    sprintf(buf, "%d", strlen(message));
    request += "Content-Length: " + std::string(buf) + "\r\n\r\n";

    request += (std::string)message;

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
    {
        *isError = ERROR_COMMUNICATION;
        return NULL;
    }

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {
        int status = params.getStatus();
        if(status == 200)
        {
            response = params.getMessage();
            *isError = ERROR_NO;
        }
        else
        {
            *isError = ERROR_COMMUNICATION;
            return NULL;
        }
    }
    return copyString(response.c_str());
}

int PrivateNacpp::GetPrintResult(const char* folderno, LPCWSTR filePath)
{
    SSL *ssl = conn->sslHandle;
    std::string request = "GET /print.php?action=savereport&id=" + (std::string)folderno + "&logo" + " HTTP/1.1\r\n"
                          "Host: nacpp.info\r\n"
                          "Cookie: PHPSESSID=";
    request += sessionId +"\r\n\r\n";

    int cnt = request.size();
    int rcnt = SSL_write(ssl, request.c_str(), cnt);

    if(cnt != rcnt)
        return ERROR_COMMUNICATION;

    Request params(ssl);
    int res = sslRead (ssl, params);
    if(res == ERROR_NO)
    {

        std::map<std::string, std::string> headers = params.getHeaders();
        std::string Content_MD5;
        std::string Content_Length;

        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); it++)
        {
            if (it->first == "Content-MD5") Content_MD5 = it->second;
            if (it->first == "Content-Length") Content_Length = it->second;
        }

        // Zero length
        if (atoi(Content_Length.c_str()) == 0)
            return ERROR_PDF_GENERATION;

        // Compute MD5
        unsigned char hash[32];
        char calc_Content_MD5[32];
        MD5((unsigned char*)params.getMessage().data(), params.getMessage().length(), hash);
        sprintf(calc_Content_MD5, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
                hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
                hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);

        if (strcmp(Content_MD5.c_str(), calc_Content_MD5) != 0)
            return ERROR_PDF_CHECK_SUM;

        int status = params.getStatus();
        if(status == 200)
        {

            std::wstring filename = std::wstring(filePath) +
                                    L"report#" +
                                    std::wstring(folderno, folderno + strlen(folderno)) +
                                    L".pdf";
  #ifdef WIN32
            HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_WRITE,
                                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,  NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD errorCode = GetLastError();
                return errorCode;
            }
            else
            {
                DWORD rw;
                WriteFile(hFile,
                          params.getMessage().c_str(),
                          params.getMessage().length(),
                          &rw,
                          NULL);
            }
            CloseHandle(hFile);
#else

            std::string fn( filename.begin(), filename.end() );

            FILE * fp = fopen(fn.c_str(), "wb+");
            if(fp != NULL)
                fwrite(params.getMessage().data(), params.getMessage().length(), 1, fp);
            else
                return ERROR_PDF_FILE_CREATE;
            fclose(fp);

#endif
            res = ERROR_NO;
        }
        else
            return ERROR_COMMUNICATION;
    }
    return res;
}
