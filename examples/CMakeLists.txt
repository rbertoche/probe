cmake_minimum_required(VERSION 2.6)
project(baseserver)

add_executable(sender sender.cpp)
TARGET_LINK_LIBRARIES(sender ${USED_LIBS})
add_executable(receiver receiver.cpp)

TARGET_LINK_LIBRARIES(receiver ${USED_LIBS})
