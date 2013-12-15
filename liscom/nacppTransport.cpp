
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
    d = new PrivateNacpp(login, password, isError, true);
}

void NacppTransport::CacheOrders(int *isError)
{
    d->CacheOrders(isError);
}

NacppTransport::~NacppTransport()
{
    delete d;
}

char* NacppTransport::GetDictionary(const char* dict, int* isError)
{
    return d->GetDictionary(dict, isError);
}

char* NacppTransport::GetFreeOrders(int num, int* isError)
{
    return d->GetFreeOrders(num, isError);
}

char* NacppTransport::GetResults(const char* folderno, int* isError)
{
    return d->GetResults(folderno, isError);
}

char* NacppTransport::GetPending(int* isError)
{
    return d->GetPending(isError);
}

char* NacppTransport::CreateOrder(const char* message, int* isError)
{
    return d->CreateOrder(message, isError);
}

char* NacppTransport::DeleteOrder(const char* folderno, int* isError)
{
    return d->DeleteOrder(folderno, isError);
}

char* NacppTransport::EditOrder(const char* message, int* isError)
{
    return d->EditOrder(message, isError);
}

int NacppTransport::GetPrintResult(const char* folderno, const char * filePath)
{
    return d->GetPrintResult(folderno, filePath);
}

void NacppTransport::Reconnect(int *isError)
{
    d->Reconnect(isError);
}

void NacppTransport::Logout()
{
    delete d;
}

void NacppTransport::FreeString(char * buf)
{
    d->FreeString(buf);
}

NacppInterface * DLLEXPORT login(const char * login, const char * password, int *isError)
{
    return getTransport(login, password, isError);
}

char * DLLEXPORT GetDictionary(NacppInterface *nacpp, const char* dict, int* isError)
{
    return nacpp->GetDictionary(dict, isError);
}

char * DLLEXPORT GetFreeOrders(NacppInterface *nacpp, int num, int* isError)
{
    return nacpp->GetFreeOrders(num, isError);
}

char * DLLEXPORT GetResults(NacppInterface *nacpp, const char* folderno, int* isError)
{
    return nacpp->GetResults(folderno, isError);
}

char * DLLEXPORT GetPending(NacppInterface *nacpp, int* isError)
{
    return nacpp->GetPending(isError);
}

char * DLLEXPORT CreateOrder(NacppInterface *nacpp, const char* message, int* isError)
{
    return nacpp->CreateOrder(message, isError);
}

char * DLLEXPORT DeleteOrder(NacppInterface *nacpp, const char* folderno, int* isError)
{
    return nacpp->DeleteOrder(folderno, isError);
}

char * DLLEXPORT EditOrder(NacppInterface *nacpp, const char* message, int* isError)
{
    return nacpp->EditOrder(message, isError);
}

int DLLEXPORT GetPrintResult(NacppInterface *nacpp, const char* folderno, const char * filePath)
{
    return nacpp->GetPrintResult(folderno, filePath);
}

void DLLEXPORT logout(NacppInterface *nacpp)
{
    nacpp->Logout();
}

void DLLEXPORT reconnect(NacppInterface *nacpp,int *isError)
{
    nacpp->Reconnect(isError);
}

void DLLEXPORT FreeString(char * buf )
{
    if(buf != NULL)
        free(buf);
}

char * NacppTransport::GetNextOrder(int *isError)
{
    return d->GetNextOrder(isError);
}

char * DLLEXPORT GetNextOrder(NacppInterface *nacpp, int *isError)
{
    return nacpp->GetNextOrder(isError);
}
