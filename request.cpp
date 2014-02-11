
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

#include "request.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "uricodec.h"

Request::Request(SSL *socket) :
    done(false),
    last_was_value(true)
{

    m_socket = socket;

    //параметры парсера загловка
    settings.on_header_field = ::on_header_field;
    settings.on_header_value = ::on_header_value;
    settings.on_message_begin = ::on_message_begin;
    settings.on_url = ::on_url;
    settings.on_headers_complete = ::on_headers_complete;
    settings.on_body = ::on_body;
    settings.on_message_complete = ::on_message_complete;

    parser = (http_parser *) malloc(sizeof(http_parser));
    assert(parser);

    http_parser_init(parser, HTTP_RESPONSE);
    parser->data = this;

}

//Возвращает значение из массива пост по ключу,
//в случае неудачи помещает в ok false.

const std::string & Request::getVal(const std::string key,
                                    bool * ok) const
{
    static std::string empty;
    std::map<std::string, std::string>::const_iterator it;
    it = post.find(key);
    if(it == post.end())
    {
        *ok = false;
        return empty;
    }
    *ok = true;
    return (*it).second;
}


Request::~Request()
{
    free(parser);
}


void Request::parseUrl()
{
    const char * curl = url.c_str();
    const char * p = strchr(curl, '?');
    if(p==NULL)
    {
        //в запросе нет параметров
        route = url;
        return;
    }

    //какие-то параметры есть.
    //сохраняем маршрут
    route = std::string(curl, p - curl);

    const char *e = curl + strlen(curl) + 1;
    do
    {
        //пропускаем разделитель поля
        p++;

        //начало наименования параметра
        const char * fs = p;
        p = strchr(p, '=');
        if(p == NULL)
        {
            //если нет символа равно, значит это поле
            //без параметра, что нас не устраивает
            break;
        }

        //конец наименования параметра
        const char * fe = p;

        std::string name(fs, fe - fs);

        //начало значения
        p++;
        fs = p;

        if(p>= e)
            break;

        p = strchr(p, '&');
        if(p != NULL)
            fe = p; //за этим параметром будут еще параметры
        else //возможно это был последний параметр
            fe = e - 1;

        std::string value(fs, fe - fs);
        addParameter(name, value);

        p = fe;

    } while(p<e);

}

void Request::addParameter(const std::string &name, const std::string &value)
{
    std::string d_name = UriDecode(name);
    std::string d_value = UriDecode(value);

    post[d_name] = d_value;
}

int on_header_field(http_parser * parser, const char *at, size_t length)
{
    Request * params = (Request *) parser->data;

    //последним вызывался обработчик значения
    if(params->last_was_value)
        params->field = std::string(at, length);
    else
        params->field += std::string(at, length);

    params->last_was_value = false;
    return EXIT_SUCCESS;
}

int on_header_value(http_parser * parser, const char *at, size_t length)
{
    Request * params = (Request *) parser->data;
    std::map<std::string, std::string>::iterator it;
    if(!params->last_was_value)
    {
        it = params->header.empty() ?
             params->header.begin() :
             params->header.end();
        params->header.insert( it, std::pair
                               <std::string,std::string>( params->field,
                                       std::string(at, length)));
    }
    else
        params->header[params->field] += std::string(at, length);

    params->last_was_value = true;
    return EXIT_SUCCESS;
}

int on_message_begin(http_parser *)
{
    return EXIT_SUCCESS;
}

int on_message_complete(http_parser *parser)
{
    //выходим из цикла чтения.

    Request * params = (Request *) parser->data;
    params->done = true;

    return EXIT_SUCCESS;
}

int on_headers_complete(http_parser *parser)
{
    Request * params = (Request *) parser->data;

    //можем распарсить url
    params->parseUrl();

    params->method = (enum http_method) params->parser->method;

    //а тут начинаем парсить тело сообщения.
    if(params->header.find("Content-Type") == params->header.end())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int on_url(http_parser * parser, const char *at, size_t length)
{
    Request * params = (Request *) parser->data;
    params->url += std::string(at, length);
    return EXIT_SUCCESS;
}

int on_body(http_parser * parser, const char *at, size_t length)
{
    Request * params = (Request *) parser->data;
    params->message += std::string(at, length);
    return EXIT_SUCCESS;
}

