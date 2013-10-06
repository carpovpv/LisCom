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
