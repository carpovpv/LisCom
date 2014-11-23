cmake_minimum_required (VERSION 2.6)
SET(CMAKE_BUILD_TYPE "Release")
set (PROJECT 
        LisCom)

project (${PROJECT})

set (HEADERS 
        request.h
        http_parser.h
        uricodec.h
        privateNacpp.h
        nacppTransport.h
    )
set (SOURCE 
        http_parser.c
        request.cpp
        uricodec.cpp
        privateNacpp.cpp
        nacppTransport.cpp
    )

find_package(OpenSSL REQUIRED)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
find_package(SQLite3 REQUIRED)

add_library ( ${PROJECT} SHARED ${HEADERS} ${SOURCE} )
if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    target_link_libraries(LisCom ${LIBS} eay32 ssleay32 wsock32 gdi32 pthreadGC2)
endif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

target_link_libraries(LisCom ${LIBS} ${OPENSSL_LIBRARIES} pthread ${SQLITE3_LIBRARIES})
install (TARGETS LisCom DESTINATION ${CMAKE_BINARY_DIR}/bin)
