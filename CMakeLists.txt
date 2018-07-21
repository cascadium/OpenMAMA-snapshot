cmake_minimum_required(VERSION 3.10)
project(OpenMAMA_snapshot)

set(CMAKE_CXX_STANDARD 14)

set(BOOST_ROOT "${_VCPKG_INSTALLED_DIR}/x64-windows")

message(${BOOST_ROOT})

find_package(Boost REQUIRED program_options)

add_executable(cascadium-snapshot main.cpp ConfigManager.cpp ConfigManager.h RestRequestHandler.cpp RestRequestHandler.h ApplicationRunner.cpp ApplicationRunner.h)

include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(cascadium-snapshot ${Boost_LIBRARIES})