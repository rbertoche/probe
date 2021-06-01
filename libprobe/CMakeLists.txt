cmake_minimum_required(VERSION 2.6)
project(libprobe)
aux_source_directory(. SRC_LIST)

add_library(${PROJECT_NAME} ${SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC .)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${USED_LIBS})

