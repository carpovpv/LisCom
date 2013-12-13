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

#ifndef NACPPTRANSPORT_H
#define NACPPTRANSPORT_H

#include "../nacppInterface.h"
#include <string>
#include <time.h>
#include <fstream>

class PrivateNacpp;
class NacppTransport : public NacppInterface
{
public:
    NacppTransport(const std::string & login,
                   const std::string & password,
                   int * isError);
    ~NacppTransport();

    char* GetDictionary(const char* dict, int* isError);
    char* GetFreeOrders(int num, int* isError);
    char* GetResults(const char* folderno, int* isError);
    char* GetPending(int* isError);
    char* CreateOrder(const char* message, int* isError);
    char* DeleteOrder(const char* folderno, int* isError);
    char* EditOrder(const char* message, int* isError);

    int GetPrintResult(const char* folderno, const char * filePath);
    void Reconnect(int *isError);

    void Logout();
    void FreeString(char *);

private:
    PrivateNacpp *d;

};

extern "C" DLLEXPORT NacppInterface * getTransport(const char* login,
                              const char* password,
                              int  * isError)
{
    return new NacppTransport(login, password, isError);
}

#endif // NACPPTRANSPORT_H
