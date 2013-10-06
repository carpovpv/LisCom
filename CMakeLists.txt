cmake_minimum_required (VERSION 2.6)
SET(CMAKE_BUILD_TYPE "Release")
set (PROJECT NacppTransport)

project (${PROJECT})

set (HEADERS
        nacppInterface.h
    )
set (SOURCE 
        main.cpp
    )

add_subdirectory(${PROJECT_SOURCE_DIR}/liscom)

add_executable ( ${PROJECT} ${HEADERS} ${SOURCE} )

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    target_link_libraries(NacppTransport ${LIBS} ${QT_LIBRARIES})
endif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    target_link_libraries(NacppTransport ${LIBS} ${QT_LIBRARIES} dl)
endif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)


install (TARGETS NacppTransport DESTINATION ${CMAKE_BINARY_DIR}/bin)

