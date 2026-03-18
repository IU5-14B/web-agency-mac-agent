# Подключаем модуль GoogleTest
include(GoogleTest)

# Тесты для Config
add_executable(test_config 
    test_config.cpp
    ${CMAKE_SOURCE_DIR}/src/config/Config.cpp
)

target_link_libraries(test_config PRIVATE
    GTest::gtest_main
    spdlog::spdlog_header_only
)

target_include_directories(test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

gtest_discover_tests(test_config)


# Тесты для парсера TaskResponse
add_executable(test_task_response 
    test_task_response.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/TaskResponse.cpp
)

target_link_libraries(test_task_response PRIVATE
    GTest::gtest_main
    spdlog::spdlog_header_only
)

target_include_directories(test_task_response PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

gtest_discover_tests(test_task_response)


# Тесты для TaskExecutor
add_executable(test_task_executor
    test_task_executor.cpp
    ${CMAKE_SOURCE_DIR}/src/executor/TaskExecutor.cpp
    ${CMAKE_SOURCE_DIR}/src/config/Config.cpp
)

target_link_libraries(test_task_executor PRIVATE
    GTest::gtest_main
    spdlog::spdlog_header_only
)

target_include_directories(test_task_executor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

gtest_discover_tests(test_task_executor)


# Тесты для SignalHandler
add_executable(test_signal_handler 
    test_signal_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/SignalHandler.cpp
)

target_link_libraries(test_signal_handler PRIVATE
    GTest::gtest_main
    spdlog::spdlog_header_only
)

target_include_directories(test_signal_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

gtest_discover_tests(test_signal_handler)


# Тесты для Communicator (в заглушках, без реальных запросов)
add_executable(test_communicator 
    test_communicator.cpp
    ${CMAKE_SOURCE_DIR}/src/communicator/Communicator.cpp
)

target_link_libraries(test_communicator PRIVATE
    GTest::gtest_main
    spdlog::spdlog_header_only
    cpr::cpr
)

target_include_directories(test_communicator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

gtest_discover_tests(test_communicator)
