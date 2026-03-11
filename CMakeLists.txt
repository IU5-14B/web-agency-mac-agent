cmake_minimum_required(VERSION 3.15)
project(web-agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции сборки
option(BUILD_TESTS "Build tests" OFF)

# Путь к исполняемым файлам
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Явный список исходников
set(SOURCES
    src/main.cpp
    src/agent/Agent.cpp
    src/config/Config.cpp
    src/communicator/Communicator.cpp
    src/logger/Logger.cpp
    src/executor/TaskExecutor.cpp
    src/utils/Utils.cpp
    src/utils/SignalHandler.cpp
    src/utils/TaskResponse.cpp
)

# Исполняемый файл агента
add_executable(web-agent ${SOURCES})

# Подключаем директории с заголовками
target_include_directories(web-agent PRIVATE src)

# Подключаем зависимости
include(cmake/dependencies.cmake)

# Линковка агента (только нужные библиотеки)
target_link_libraries(web-agent PRIVATE spdlog::spdlog_header_only cpr::cpr)

# Тесты
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
