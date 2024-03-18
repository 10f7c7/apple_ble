cmake_minimum_required(VERSION 3.10)

project(servers VERSION 1.0)


include( FindPkgConfig )

# find_package(PkgConfig REQUIRED)

# pkg_check_modules(deps REQUIRED IMPORTED_TARGET sdbus-c++ simpleble)


find_package(sdbus-c++ REQUIRED)
find_package(simpleble REQUIRED CONFIG)
include_directories("/usr/include/dbus-1.0")
# find_package(simplebluez REQUIRED CONFIG)

file(GLOB_RECURSE SRCFILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(ams_mpris ${SRCFILES})
target_link_libraries(ams_mpris PUBLIC rt simpleble::simpleble sdbus-c++)

# add_executable(server src/server.cpp)
# target_link_libraries(server sdbus-c++)


# add_executable(ble src/ble.cpp)

# target_link_libraries(ble simpleble::simpleble)

# add_executable(notify notify.cpp)


# target_link_libraries(notify simplebluez::simplebluez)
