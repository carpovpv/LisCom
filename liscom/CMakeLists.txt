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

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_library ( ${PROJECT} SHARED ${HEADERS} ${SOURCE} )
if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    target_link_libraries(LisCom ${LIBS} ${QT_LIBRARIES} wsock32 ${OPENSSL_LIBRARIES} )
endif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    target_link_libraries(LisCom ${LIBS} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES} )
endif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

install (TARGETS LisCom DESTINATION ${CMAKE_BINARY_DIR}/bin)

