
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include "windows.h"
#else
#include <dlfcn.h>
#endif

#include "nacppInterface.h"

typedef NacppInterface* (*TRANSPORT_FACTORY)(const char*, const char*, int *);

#ifdef WIN32
HINSTANCE hModule;
#else
void * hModule;
#endif

//Загрузка динамической библиотеки
TRANSPORT_FACTORY LoadLisCom()
{
    TRANSPORT_FACTORY p_factory_function;
#ifdef WIN32
    hModule= LoadLibrary("liscom/libLisCom.dll");
    if (hModule)
        p_factory_function =
            (TRANSPORT_FACTORY)GetProcAddress(hModule, "getTransport");
#else
    hModule = dlopen("liscom/libLisCom.so", RTLD_LAZY);
    if (hModule)
    {
        dlerror();
        p_factory_function = (TRANSPORT_FACTORY) dlsym(hModule, "getTransport");
    }
#endif
    return p_factory_function;
}

//выгрузка динамической библиотеки
void UnloadLisCom()
{
#ifdef WIN32
    FreeLibrary(hModule);
#else
    dlclose(hModule);
#endif
}

//макрос только для упрощения кода
//проверяет код возвращения метода сервиса и в случае успеха печатает сообщение,
//все методы возвращают char *, удаление строк лежит на сороне клиента.

#define print(str) if(res == ERROR_NO) { std::cout << str << std::endl; nacpp->Free(str); }else std::cout << "ErrorCode: " << res << std::endl

int main ()
{
    NacppInterface * nacpp = NULL;
    TRANSPORT_FACTORY p_factory_function = LoadLisCom();
    if (p_factory_function != NULL)
    {
        //!!! Внимание. Это тестовый аккаунт. Для тестирования используйте логин и пароль,
        //полученный от менеджеров лаборатории.

        const char * login = "TESTER";
        const char * password = "Q3434";

        int res;
        nacpp = (*p_factory_function)(login, password, &res);
        if(res != 0)
        {
            //Ошибки здесь скорее всего сетевые: недоступен хост, неверно загружены SSL библиотеки и пр.
            fprintf(stderr, "Error in authorization/communication: %d.\n", res);

            delete nacpp;
            UnloadLisCom();

            return EXIT_FAILURE;
        }

        //получение направлений с результатами, ожидающих передачи в МИС
        //char * pending = nacpp->GetPending(&res);
        //print(pending);
        
        //получение справочников
        //char * dict = nacpp->GetDictionary("bio", &res);
        //print(dict);

        //получение новых номеров направлений из пула
        //char * pool = nacpp->GetFreeOrders(1, &res);
        //print(pool);

        //регистрация нового направления
        std::string message = ""
                              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                              "<request>"
                              "<personal>"
                              "<guid>d7f0fbbd-22cc-41e1-8f2a-146a47e89ad7</guid>"
                              "<surname>Карева</surname>"
                              "<name>Марина</name>"
                              "<patronimic>Павловна</patronimic>"
                              "<birthdate>1977/01/01</birthdate>"
                              "<gender>F</gender>"
                              "<clientcode>3434</clientcode>"
                              "<cardno>015</cardno>"
                              "<datecollect>05.12.2012 09:15</datecollect>"
                              "<department>гинеколгия</department>"
                              "<doctor>Зайцева Г.В.</doctor>"
                              "<diagnosis />"
                              "<comment />"
                              "<pregnancy />"
                              "<phase>f</phase>"
                              "<insurer>РОСНО</insurer>"
                              "<passno />"
                              "<passseries />"
                              "<address />"
                              "<phone />"
                              "<email />"
                              "<policy>1111222244</policy>"
                              "<cito>O</cito>"
                              "<diuresis />"
                              "<weight />"
                              "<height />"
                              "<antibiotics>ампициллин</antibiotics>"
                              "<antibstart>01.10.2012</antibstart>"
                              "<antibend>30.11.2012</antibend>"
                              "</personal>"
                              "<containers>"
                              "<container id=\"1\" external=\"01\" biomaterial=\"75\" "
                              "containertype=\"23\" />"
                              "<container id=\"2\" external=\"02\" biomaterial=\"76\" "
                              "containertype=\"1\" />"
                              "<container id=\"3\" external=\"03\" biomaterial=\"463\" "
                              "containertype=\"21\" />"
                              "<container id=\"4\" external=\"04\" biomaterial=\"118\" "
                              "containertype=\"29\" tubeno=\"3434-2564\" />"
                              "</containers>"
                              "<panels>"
                              "<panel code=\"10.100\" container=\"1\" action=\"add\"/>"
                              "<panel code=\"10.105\" container=\"1\" action=\"add\"/>"
                              "<panel code=\"16.100\" container=\"2\" action=\"add\"/>"
                              "<panel code=\"54.105\" container=\"3\" action=\"add\"/>"
                              "<panel code=\"52.150\" container=\"4\" action=\"add\"/>"
                              "</panels>"
                              "</request>";
        //char * resp = nacpp->CreateOrder(message.c_str(), &res);
        //print(resp);

        //получение печатной копии результатов исследований
        //в этом примере файл сохраняется в текущую директорию.
        //res = nacpp->GetPrintResult("0004543617", L"./");

        //получение XML версии с результатами для разбора
        //char * results = nacpp->GetResults("0004543617", &res);
        //print(results);

        delete nacpp;
	UnloadLisCom();

    }
    else
        std::cout << "Error Load function from dynamic module." << std::endl;

    return EXIT_SUCCESS;
}
