# Тест для проверки библиотек
add_executable(test_libraries test_libraries.cpp)
target_link_libraries(test_libraries PRIVATE 
    spdlog::spdlog_header_only
    cpr::cpr
)
target_include_directories(test_libraries PRIVATE ${CMAKE_BINARY_DIR}/include)

# Регистрируем тест
add_test(NAME LibrariesTest COMMAND test_libraries)
