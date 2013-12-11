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

#include "nacppTransport.h"
#include "privateNacpp.h"

NacppTransport::NacppTransport(const std::string & login,
                               const std::string & password,
                               int * isError)
{
    d = new PrivateNacpp(login, password, isError);
}

NacppTransport::~NacppTransport()
{
    delete d;
}

char* NacppTransport::GetDictionary(const char* dict, int* isError)
{
    d->GetDictionary(dict, isError);
}

char* NacppTransport::GetFreeOrders(int num, int* isError)
{
    d->GetFreeOrders(num, isError);
}

char* NacppTransport::GetResults(const char* folderno, int* isError)
{
    d->GetResults(folderno, isError);
}

char* NacppTransport::GetPending(int* isError)
{
    d->GetPending(isError);
}

char* NacppTransport::CreateOrder(const char* message, int* isError)
{
    d->CreateOrder(message, isError);
}

char* NacppTransport::DeleteOrder(const char* folderno, int* isError)
{
    d->DeleteOrder(folderno, isError);
}

char* NacppTransport::EditOrder(const char* message, int* isError)
{
    d->EditOrder(message, isError);
}

int NacppTransport::GetPrintResult(const char* folderno, LPCWSTR filePath)
{
    d->GetPrintResult(folderno, filePath);
}

void NacppTransport::Free(char *mem)
{
    if(mem != NULL)
        free(mem);
}

#ifdef WIN32

NacppTransport * nakff = NULL;

extern "C" __declspec(dllexport) void login(const char * login, const char * password, int * isError)
{
    nakff = new NacppTransport(login, password, isError);
}

extern "C" __declspec(dllexport) char* GetDictionary(const char* dict, int* isError)
{
    if(nakff)
        nakff->GetDictionary(dict, isError);
}

extern "C" __declspec(dllexport) char* GetFreeOrders(int num, int* isError)
{
    if(nakff)
        nakff->GetFreeOrders(num, isError);
}

extern "C" __declspec(dllexport) char* GetResults(const char* folderno, int* isError)
{
    if(nakff)
        nakff->GetResults(folderno, isError);
}

extern "C" __declspec(dllexport) char* GetPending(int* isError)
{
    if(nakff)
        nakff->GetPending(isError);
}

extern "C" __declspec(dllexport) char* CreateOrder(const char* message, int* isError)
{
    if(nakff)
        nakff->CreateOrder(message, isError);
}

extern "C" __declspec(dllexport) char* DeleteOrder(const char* folderno, int* isError)
{
    if(nakff)
        nakff->DeleteOrder(folderno, isError);
}

extern "C" __declspec(dllexport) char* EditOrder(const char* message, int* isError)
{
    if(nakff)
        nakff->EditOrder(message, isError);
}

extern "C" __declspec(dllexport) int GetPrintResult(const char* folderno, LPCWSTR filePath)
{
    if(nakff)
        nakff->GetPrintResult(folderno, filePath);
}

extern "C" __declspec(dllexport) void Free(char * mem)
{
    if(nakff)
        nakff->Free(mem);
}

extern "C" __declspec(dllexport) void logout()
{
    if(nakff)
    {
        delete nakff;
        nakff = NULL;
    }
}

#endif
