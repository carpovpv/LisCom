
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

#ifndef NACPPINTERFACE_H
#define NACPPINTERFACE_H

#include <string>

#define ERROR_NO              0
#define ERROR_COMMUNICATION   100
#define ERROR_TOO_BIG_MESSAGE 101
#define ERROR_HTTP_PARSER     102
#define ERROR_LOGIN           103
#define ERROR_SSLCONTEXT      104
#define ERROR_SOCKET          105
#define ERROR_SSLHANDLE       106
#define ERROR_SSLSETFD        107
#define ERROR_SSLCONNECT      108

#define ERROR_UNKNOWN_DICT    200
#define ERROR_MORE_POOL_NUM   201

#define ERROR_PDF_GENERATION  300
#define ERROR_PDF_CHECK_SUM   301
#define ERROR_PDF_FILE_CREATE 302

typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;

#ifdef WIN32
    #define DLLEXPORT  __declspec(dllexport)
    #include <windows.h>
#else
    #define DLLEXPORT
#endif

/**
    @brief Интерфейсный класс для протокола взаимодействия между МИС ЛПУ и
    Лабораторной Информационной Системой ООО НАКФФ
    @detailed Протокол описывает схему взаимодействия между
    МИС и ЛИС.
    Предоставляет возможность регистрация и
    получения результатов, в протоколе также предусмотрено
    несколько системных запросов на поддержание
    в актуальном состоянии справочной базы по исследованиям.
*/

class NacppInterface
{
public:
    /**
    @brief Получение справочника.
    @detailed Данная функция позволяет получить требуемый справочник.
    @param dict Тип справочника:
        * bio - биоматериалы;
        * tests - тесты и аналиты;
        * containertypes - типы контейнеров;
        * panels - панели испытаний.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с требуемым справочником.
    */
    virtual char* GetDictionary(LPCSTR dict, int* isError) = 0;

    /**
    @brief Получение пула номеров направлений.
    @detailed Данная функция сформирует свободные номера
    для правильного штрихкодирования проб в медицинском центре.
    @param num Количество направлений для передачи (не более 1000)
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с требуемым числом свободных номеров направлений.
    */
    virtual char* GetFreeOrders(int num, int* isError) = 0;

    /**
    @brief Регистрация нового направления.
    @detailed Данная функция позволяет получить требуемый справочник.
    @param message XML-файл, описывающий демографию пациента.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с ответом:
    В случае успеха будет передан сгенерированный номер направления,
    атрибут “статус” будет установлен в OK, в противном случае – в
    FAILED и в комментарии будет написана причина отказа в регистрации.
    */
    virtual char* CreateOrder(const char* message, int* isError) = 0;

    /**
    @brief Редактирование существующего направления.
    @detailed Данная функция позволяет редактировать направление
    после его регистрации. Редактирование возможно до тех пор, пока пробы
    не поступят в лабораторию. С этого момента любые изменения
    в направлении должны согласовываться с лабораторией напрямую.
    @param message XML-файл, описывающий демографию пациента, с изменениями.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с ответом:
    В случае успеха атрибут “статус” будет установлен в OK, в противном
    случае – в FAILED и в комментарии будет написана причина отказа в регистрации.
    */
    virtual char* EditOrder(const char* message, int* isError) = 0;

    /**
    @brief Исключение направления.
    @detailed Данная функция позволяет исключать целиком направление.
    @param folderno Номер направления.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с ответом:
    В случае успеха атрибут “статус” будет установлен в OK, в противном
    случае – в FAILED и в комментарии будет написана причина отказа в регистрации.
    */
    virtual char* DeleteOrder(const char* folderno, int* isError) = 0;

    /**
    @brief Получение результатов.
    @detailed Данная функция позволяет получить результат
    по заданному направлению.
    @param folderno Номер направления.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с результатами исследований.
    */
    virtual char* GetResults(const char* folderno, int* isError) = 0;

    /**
    @brief Получение списка ожидающих передачи направлений.
    @detailed Данная функция позволяет получить список.
    ожидающих передачи направлений.
    @param isError Сохраняет код ошибки, 0 - если ошибок нет.
    @return XML-сообщение с номерами проб, регистрация которых
    совершилась менее месяца назад от текущей даты и которые
    не были переданы в МИС.
    */
    virtual char* GetPending(int* isError) = 0;

    /**
    @brief Печатная версия PDF с результатами исследований по направлению.
    @detailed Данная функция позволяет получить печатную версию PDF
    с результатами исследований по заданному направлению.
    @param folderno Номер направления.
    @param filePath Путь сохранения файла.
    @return Код ошибки, 0 - если ошибок нет.
    */
    virtual int GetPrintResult(const char* folderno, const char *filePath ) = 0;

    /**
    @brief Переподключение к сервису в случае обрыва связи или
    первышения допустимого количества запросов в рамках одного соединения
    */
    virtual void Reconnect(int *isError) = 0;

    /*закрытие сессии и удаление объекта */
    virtual void Logout() = 0;

    /* очистка динамической памяти */
    virtual void FreeString(char *)=0;

    virtual ~NacppInterface() {}
};

extern "C"
{
    NacppInterface * DLLEXPORT login(const char * login, const char * password, int *isError);
    char * DLLEXPORT GetDictionary(NacppInterface *nacpp, const char* dict, int* isError);
    char * DLLEXPORT GetFreeOrders(NacppInterface *nacpp, int num, int* isError);
    char * DLLEXPORT GetResults(NacppInterface *nacpp, const char* folderno, int* isError);
    char * DLLEXPORT GetPending(NacppInterface *nacpp, int* isError);
    char * DLLEXPORT CreateOrder(NacppInterface *nacpp, const char* message, int* isError);
    char * DLLEXPORT DeleteOrder(NacppInterface *nacpp, const char* folderno, int* isError);
    char * DLLEXPORT EditOrder(NacppInterface *nacpp, const char* message, int* isError);
    int DLLEXPORT GetPrintResult(NacppInterface *nacpp, const char* folderno, const char * filePath);
    void DLLEXPORT logout(NacppInterface *nacpp);
    void DLLEXPORT reconnect(NacppInterface *nacpp,int *isError);
    void DLLEXPORT FreeString(char * buf);
}

#endif // NACPPINTERFACE_H
