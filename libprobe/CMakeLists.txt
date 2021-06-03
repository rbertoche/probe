cmake_minimum_required(VERSION 2.6)
project(libprobe)

set(SOURCES
    basemulticastserver.cpp
    dump.cpp
    mensagem.cpp
    cli.c
    rtclock.c
)

set(HEADERS
    basemulticastserver.h
    dump.h
    mensagem.h
    cli.h
    rtclock.h
)


add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC .)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${USED_LIBS})

