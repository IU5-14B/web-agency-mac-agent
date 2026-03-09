cmake_minimum_required(VERSION 3.15)
project(web-agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции сборки
option(BUILD_TESTS "Build tests" OFF)

# Путь к исполняемым файлам
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Подключаем файл с зависимостями


# Собираем список всех исходников (рекурсивно)
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(web-agent ${SOURCES} ${HEADERS})

# Подключаем директории с заголовками
target_include_directories(web-agent PRIVATE src)

include(cmake/dependencies.cmake)


# Если тесты включены
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
