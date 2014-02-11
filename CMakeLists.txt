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

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -s")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -s")
ADD_DEFINITIONS(-static-libgcc)

    target_link_libraries(LisCom ${LIBS} ssl crypto wsock32 gdi32 ssl pthread ${SQLITE3_LIBRARIES})

set_target_properties(LisCom PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(LisCom PROPERTIES LINK_SEARCH_END_STATIC 1)
set_target_properties(LisCom PROPERTIES LINK_FLAGS "-static-libgcc")
endif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    target_link_libraries(LisCom ${LIBS} ${OPENSSL_LIBRARIES} pthread ${SQLITE3_LIBRARIES})
endif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

install (TARGETS LisCom DESTINATION ${CMAKE_BINARY_DIR}/bin)

