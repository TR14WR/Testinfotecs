cmake_minimum_required(VERSION 3.28)
project(client_server)

find_package(Boost REQUIRED COMPONENTS serialization system program_options log thread asio)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror
        -O3 -march=native)
endif()

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

# Тесты (опционально)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
