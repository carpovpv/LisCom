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

    int GetPrintResult(const char* folderno, LPCWSTR filePath = L"");
    void Free(char * mem);

private:
    PrivateNacpp *d;

};

#ifdef WIN32
extern "C" __declspec(dllexport)
NacppInterface * getTransport(const char* login,
                              const char* password,
                              int  * isError)
{
    return new NacppTransport(login, password, isError);
}

extern "C" __declspec(dllexport) void login(const char * login, const char * password, int *isError);
extern "C" __declspec(dllexport) char* GetDictionary(const char* dict, int* isError);
extern "C" __declspec(dllexport) char* GetFreeOrders(int num, int* isError);
extern "C" __declspec(dllexport) char* GetResults(const char* folderno, int* isError);
extern "C" __declspec(dllexport) char* GetPending(int* isError);
extern "C" __declspec(dllexport) char* CreateOrder(const char* message, int* isError);
extern "C" __declspec(dllexport) char* DeleteOrder(const char* folderno, int* isError);
extern "C" __declspec(dllexport) char* EditOrder(const char* message, int* isError);
extern "C" __declspec(dllexport) int GetPrintResult(const char* folderno, LPCWSTR filePath = L"");
extern "C" __declspec(dllexport) void Free(char * mem);
extern "C" __declspec(dllexport) void logout();

#else
extern "C" NacppInterface * getTransport(const char* login,
        const char* password,
        int * isError)
{                                     
    return new NacppTransport(login, password, isError);
}
#endif


#endif // NACPPTRANSPORT_H
