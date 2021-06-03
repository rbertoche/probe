cmake_minimum_required(VERSION 2.6)
project(orquestrador)
aux_source_directory(. SRC_LIST)

add_executable(orquestrador orquestrador.cpp)
TARGET_LINK_LIBRARIES(orquestrador ${USED_LIBS})
add_executable(servidor servidor.cpp)
TARGET_LINK_LIBRARIES(servidor ${USED_LIBS})
add_executable(cliente cliente.cpp)
TARGET_LINK_LIBRARIES(cliente ${USED_LIBS})

