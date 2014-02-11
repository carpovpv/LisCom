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
#include <signal.h>

#ifdef WIN32

     #include <winbase.h>
     #include <windows.h>

#endif

//multithreaded support for SSL

struct CRYPTO_dynlock_value
{
    pthread_mutex_t mutex;
};

static pthread_mutex_t *mutex_buf = NULL;
static void locking_function(int mode, int n, const char *file, int line)
{
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&mutex_buf[n]);
    } else {
        pthread_mutex_unlock(&mutex_buf[n]);
    }
}
static unsigned long id_function(void)
{
    #ifdef WIN32
        return GetCurrentThreadId();
    #else
        return pthread_self();
    #endif
}

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int
line)
{
    struct CRYPTO_dynlock_value *value;

    value = (struct CRYPTO_dynlock_value *)
        malloc(sizeof(struct CRYPTO_dynlock_value));
    if (!value) {
        goto err;
    }
    pthread_mutex_init(&value->mutex, NULL);

    return value;

  err:
    return (NULL);
}

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,
                              const char *file, int line)
{
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&l->mutex);
    } else {
        pthread_mutex_unlock(&l->mutex);
    }
}

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l,
                                 const char *file, int line)
{
    pthread_mutex_destroy(&l->mutex);
    free(l);
}

int tls_init(void)
{
    int i;


    mutex_buf = (pthread_mutex_t*)malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    if (mutex_buf == NULL) {
        return (-1);
    }
    for (i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_init(&mutex_buf[i], NULL);
    }

    CRYPTO_set_locking_callback(locking_function);
    (id_function);

    CRYPTO_set_dynlock_create_callback(dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);

    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();

    RAND_load_file("/dev/urandom", 1024);

    return (0);
}

int tls_cleanup(void)
{
    int i;

    if (mutex_buf == NULL) {
        return (0);
    }

    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);

    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_destroy(&mutex_buf[i]);
    }
    free(mutex_buf);
    mutex_buf = NULL;

    return (0);
}

void start()
{

#ifdef WIN32
    WSADATA wsaData = {0};
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0)
    {
        printf("WSAStartup failed: %d\n", res);
    }
#endif

    SSL_load_error_strings ();
    SSL_library_init ();


    tls_init();
}

void stop()
{

    tls_cleanup();
}

#ifdef WIN32

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            start();
            break;

        case DLL_PROCESS_DETACH:

            stop();
            break;

    }

    return TRUE;
}

#else

void dllstart() __attribute__ ((constructor));
void dllfinish() __attribute__ ((destructor));

void dllstart()
{
    start();
}

void dllfinish()
{
    stop();
}

#endif

PrivateNacpp::PrivateNacpp(const std::string &login,
                           const std::string & password,
                           int * isError,
                           bool needCache)
{

    info = new struct ConnectInfo;
    info->login = login;

    info->password = password;
    info->work = true;

    conn = NULL;
    info->db = NULL;

    //значение текущего потока гарантировано не совпадет
    //со значение идентификатора нового потока, поэтому
    //используется в качестве инициализирующего значения
    thread = pthread_self();

    //необходимы для синхронизации с потоком получения пула
    pthread_mutex_init(&info->mutex, NULL);
    pthread_cond_init(&info->cond, NULL);

    int res = sslConnect(&conn);
    if(res != ERROR_NO)
        *isError = res;
    else
        *isError = LoginToNacpp(info->login, info->password);

    if(*isError == ERROR_NO && needCache)
    {
        //подключаемся к базе SQLite
        //запускаем потом получения пула номеров

        fprintf(stderr, "Create thread!\n");

        int rc = sqlite3_open("orders.db", &info->db);
        if(rc == 0)
        {
            std::string sql = "create table if not exists orders (orderno varchar(10) primary key);";
            rc = sqlite3_exec(info->db, sql.c_str(), NULL, 0, NULL);
            if( rc == SQLITE_OK )            
                 pthread_create(&thread, NULL, cacher, info);
            //в противном случае, мы не можем работать с кешем.
            //сама база будет закрыта в деструкторе.
        }
        else
            info->db = NULL;

    }
}

PrivateNacpp::~PrivateNacpp()
{
    if(info->db != NULL)
         sqlite3_close(info->db);

    info = NULL;
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


    int handle = tcpConnect();
    if(handle == 0)
        return ERROR_SOCKET;

    SSL * sslHandle = NULL;
    SSL_CTX * sslContext = NULL;   

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

    if(!pthread_equal(thread,pthread_self()))
    {
        //у нас есть поток
        info->work = false;

        //сигналим - бип-бип...
        pthread_mutex_lock(&info->mutex);
        pthread_cond_signal(&info->cond);
        pthread_mutex_unlock(&info->mutex);

        fprintf(stderr, "Start waiting...\n");
        //ждемс...
        pthread_join(thread, NULL);
        fprintf(stderr, "End waiting...\n");

        //все потока уже нет, продолжаем закругляться...

        pthread_mutex_destroy(&info->mutex);
        pthread_cond_destroy(&info->cond);

        fprintf(stderr, "Thread terminated!\n");
    }

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
    sslRead (ssl, params);

    sslDisconnect(conn);
    return 0;
}

void PrivateNacpp::Reconnect(int * isError)
{
    sslDisconnect(conn);

    conn = NULL;
    int res = sslConnect(&conn);
    if(res != ERROR_NO)
        *isError = res;
    else
        *isError = LoginToNacpp(info->login, info->password);
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
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
    return strdup(response.c_str());
}

int PrivateNacpp::GetPrintResult(const char* folderno, const char * filePath)
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
        /*
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
        */
        int status = params.getStatus();
        if(status == 200)
        {

            std::string filename = std::string(filePath) +
                    "report#" +
                    std::string(folderno, folderno + strlen(folderno)) +
                    ".pdf";
#ifdef WIN32

            HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
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

void PrivateNacpp::FreeString(char *buf)
{
    if(buf != NULL)
        free(buf);
}

//используется всего два запроса для callback
// 1. получение текущего количества свободных номеров
// 2. получение следующего номера

static int call_sqlite(void *data,
                       int, char **argv,
                       char **)
{
   char * buf = (char *) data;
   strcpy(buf, argv[0]);
   return 0;
}

char * PrivateNacpp::GetNextOrder(int *isError)
{
    if(info->db == NULL)
    {
        *isError = ERROR_CACHE_UNAVAIL;
        return NULL;
    }

    //пробуем получить из базы
    char buf[126];
    std::string sql = "select orderno from orders order by orderno asc limit 1";

    buf[0] = '\0';
    int rc = sqlite3_exec(info->db, sql.c_str(),
                          call_sqlite,
                          (void*)buf, NULL);
    if( rc == SQLITE_OK && buf[0] != '\0')
    {
        //получили, ура.
        std::string orderno = buf;
        sql = "delete from orders where orderno ='" + orderno +"'";

        rc = sqlite3_exec(info->db, sql.c_str(),
                                  NULL,
                                  (void*)buf, NULL);
        if(rc == SQLITE_OK)
        {
            //все в порядке, номер получен и удален из кэша
            *isError = ERROR_NO;
            return strdup(buf);
        }
    }
    else
    {
        //пытаемся запросить сервис...
        char * res = GetFreeOrders(1, isError);
        if(*isError == ERROR_NO)
        {
            char * p = strstr(res, "<orderno>");
            if(p != NULL)
            {
                  p += 9;
                  std::string orderno = std::string(p, 10);

                  FreeString(res);
                  *isError = ERROR_NO;

                  return strdup(orderno.c_str());
            }
            else
            {
                *isError = ERROR_COMMUNICATION;
                FreeString(res);

                return NULL;
            }
        }
    }
    return NULL;
}

void * cacher(void * inf)
{

    //получение номеров в кэш через отдельный поток и
    //отдельное подключение

    struct ConnectInfo * info = (struct ConnectInfo *) inf;
    int isError = 0;

    while(info->work)
    {
        pthread_mutex_lock(&info->mutex);

        char buf[126];

        std::string sql = "select count(*) as cnt from orders";
        int rc = sqlite3_exec(info->db, sql.c_str(),
                              call_sqlite,
                              (void*)buf, NULL);
        if( rc == SQLITE_OK )
        {

            const int N = 500;    //максимально за раз
            const int nmin = 100;  //минимальное количество

            fprintf(stderr, "Number of folders %s\n", buf);

            int rn = atoi(buf);
            if(rn < nmin)
            {

                int need = N;
                PrivateNacpp * nacpp = new PrivateNacpp(info->login,
                                                        info->password, &isError);
                char * res = nacpp->GetFreeOrders(need, &isError);
                if(isError == ERROR_NO)
                {
                    char * p = res;
                    char * e = res + strlen(res);
                    while( (p = strstr(p, "<orderno>")) != NULL)
                    {
                          p += 9;
                          if(p + 10 <= e)
                          {
                              std::string orderno = std::string(p, 10);
                              std::string sql = "insert into orders(orderno) values('" + orderno + "');";

                              rc = sqlite3_exec(info->db, sql.c_str(), NULL, 0, NULL);
                              if( rc != SQLITE_OK )
                              {
                                  //что бы здесь сделать
                                  //....................
                              }

                          }
                    }

                    nacpp->FreeString(res);
                }

                nacpp->LogoutFromNacpp();
                delete nacpp;
            }


        }

        //периодически чистим кэш

        std::string vacuum = "vacuum;";
        sqlite3_exec(info->db, vacuum.c_str(), NULL, 0, NULL);

        struct timespec ts;

        ts.tv_sec  = time(NULL);
        ts.tv_nsec = 0;
        ts.tv_sec += 10;

        rc = pthread_cond_timedwait(&info->cond,
                               &info->mutex,
                               &ts);
        if(rc == ETIMEDOUT)
        {
            pthread_mutex_unlock(&info->mutex);
            continue;
        }

        pthread_mutex_unlock(&info->mutex);
        break;
    }

    return NULL;
}
