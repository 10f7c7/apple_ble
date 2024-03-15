cmake_minimum_required(VERSION 3.10)

project(servers VERSION 1.0)


include( FindPkgConfig )

find_package(sdbus-c++ REQUIRED)

add_executable(server server.cpp)
target_link_libraries(server sdbus-c++)


add_executable(ble ble.cpp)

find_package(simpleble REQUIRED CONFIG)
target_link_libraries(ble simpleble::simpleble)

add_executable(notify notify.cpp)


find_package(simplebluez REQUIRED CONFIG)
target_link_libraries(notify simplebluez::simplebluez)
