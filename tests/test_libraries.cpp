#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>

int main() {
    std::cout << "Testing libraries..." << std::endl;
    
    nlohmann::json test = {
        {"name", "test"},
        {"value", 42}
    };
    std::cout << "nlohmann/json: " << test.dump() << std::endl;
    
    spdlog::info("spdlog: logging works");
    
    cpr::Response r = cpr::Get(cpr::Url{"https://httpbin.org/get"});
    std::cout << "CPR: status code " << r.status_code << std::endl;
    
    std::cout << "All libraries test passed!" << std::endl;
    return 0;
}
