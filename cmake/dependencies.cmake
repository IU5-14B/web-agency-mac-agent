include(FetchContent)

# nlohmann/json (header-only)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(json)
file(COPY ${json_SOURCE_DIR}/json.hpp DESTINATION ${CMAKE_BINARY_DIR}/include/nlohmann)
target_include_directories(web-agent PRIVATE ${CMAKE_BINARY_DIR}/include)

# spdlog (header-only)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(web-agent PRIVATE spdlog::spdlog_header_only)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)
target_link_libraries(web-agent PRIVATE cpr::cpr)
