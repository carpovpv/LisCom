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

#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <map>

#include "http_parser.h"
#include <openssl/ssl.h>

extern "C"
{
    class Request
    {
    public:

        Request(SSL *socket);
        ~Request();

        //ассоциативный массив $_POST
        std::map<std::string, std::string> post;
        const std::string & getVal(const std::string, bool *ok) const;

        bool isDone() {
            return done;
        }
        int getStatus() {
            return parser->status_code;
        }
        const std::map<std::string, std::string> & getHeaders() {
            return header;
        }
        const std::string &getMessage() {
            return message;
        }

    private:

        //дескриптор соединения с клиентом
        SSL * m_socket;
        std::string route;

        //запрет на копирование
        Request(const Request &);
        Request & operator = (const Request &);

        //указывает выйти из цикла чтения, заполняется когда
        //получено все сообщение целиком
        bool done;

        //ассоциативный массив параметров заголовка
        std::map<std::string, std::string> header;

        //ассоциативный массив подзаголовков
        //очищается по мере продвижения по подзаголовкам
        std::map<std::string, std::string> b_header;

        //тип запроса
        enum http_method method;

        //url
        std::string url;

        //body
        std::string message;

        //последний callback был для значения,
        //тогда это поле true
        bool last_was_value;

        //header_field В карте нельзя изменять значение ключа,
        //после вставки, поэтому используем отдельную строку для
        //полного получения всего ключа.
        std::string field;

        std::string filename;
        std::string contenttype;
        std::string value;

        //парсер заголовков
        http_parser_settings settings;
        http_parser *parser;

        friend class PrivateNacpp;
        //вся низкоуровневая обработка должна быть в друзьях

        friend int on_header_field(http_parser * parser, const char *at, size_t length);
        friend int on_header_value(http_parser * parser, const char *at, size_t length);
        friend int on_message_complete(http_parser *parser);
        friend int on_url(http_parser * parser, const char *at, size_t length);
        friend int on_headers_complete(http_parser *parser);
        friend int on_body(http_parser * parser, const char *at, size_t length);
        friend int on_message_begin(http_parser *parser);

        friend int sslRead(SSL *, Request &);

        void parseUrl();

        //добавляем параметр
        void addParameter(const std::string & name,
                          const std::string & value);

    };

//и снова наши прототипы.

    int on_header_field(http_parser * parser, const char *at, size_t length);
    int on_header_value(http_parser * parser, const char *at, size_t length);
    int on_message_complete(http_parser *parser);
    int on_message_begin(http_parser *parser);
    int on_url(http_parser * parser, const char *at, size_t length);
    int on_body(http_parser * parser, const char *at, size_t length);
    int on_headers_complete(http_parser *parser);
}
#endif // REQUEST_H
