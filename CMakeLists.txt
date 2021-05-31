
cmake_minimum_required(VERSION 2.6)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost 1.41.0 REQUIRED COMPONENTS system thread regex)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} baseserver)


set(CMAKE_CXX_FLAGS "-O0 -g -Wall -std=c++98")

add_subdirectory(baseserver)


add_subdirectory(orquestrador)
add_subdirectory(servidor)
add_subdirectory(cliente)
