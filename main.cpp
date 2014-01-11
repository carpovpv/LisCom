
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

typedef NacppInterface* (*TRANSPORT_FACTORY)(const char*, const char*, bool, int *);

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
        nacpp = (*p_factory_function)(login, password, false, &res);
        if(res != 0)
        {
            //Ошибки здесь скорее всего сетевые: недоступен хост, неверно загружены SSL библиотеки и пр.
            fprintf(stderr, "Error in authorization/communication: %d.\n", res);

            nacpp->Logout();
            UnloadLisCom();

            return EXIT_FAILURE;
        }

        /*std::string req = "<?xml version=\"1.0\"?><request><personal><guid>{E955F461-0736-42E0-ADAD-C007B14903D3}</guid><surname>Тест</surname><name>200</name><patronimic></patronimic><birthdate>10.01.1990</birthdate><gender>M</gender><clientcode>0471</clientcode><datecollect>24.12.2013 10:54</datecollect></personal><containers><container id=\"1\" external=\"123456789\" biomaterial=\"75\"/></containers><panels><panel code=\"10.100\" container=\"1\"/></panels></request>";
        char * r = nacpp->CreateOrder(req.c_str(),
                                      &res);
        printf("%s\n", r);
        return 0;*/
        //получение справочника биоматериалов
        char * dict = nacpp->GetDictionary("bio", &res);
        if(res == ERROR_NO)
        {
            printf("Result: %s\n", dict);
            nacpp->FreeString(dict);
            //дальнейшая обработка

#ifdef WIN32
            Sleep(2000);
#else
            sleep(2);
#endif

            char *orderno = nacpp->GetNextOrder(&res);
            if(res == ERROR_NO)
            {
                fprintf(stderr, "Orderno: %s\n", orderno);
                nacpp->FreeString(orderno);
            }
        }
        else if(res == ERROR_COMMUNICATION)
        {
             nacpp->Reconnect(&res);
             if(res != ERROR_NO)
             {
                 fprintf(stderr, "Failed reconnect!\n");
                 nacpp->Logout();
                 UnloadLisCom();

                 return EXIT_FAILURE;
             }
             else
             {
                 fprintf(stderr, "Reconnect success!\n");
                 //можно пытаться продолжать выполнять запросы.
             }
        }

        nacpp->Logout();
	UnloadLisCom();
    }
    else
        std::cout << "Error Load function from dynamic module." << std::endl;

    return EXIT_SUCCESS;
}
