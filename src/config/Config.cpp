#include "Config.h"
#include <fstream>
#include <iostream>

bool Config::loadFromFile(const std::string& path) {
    // Заглушка
    return true;
}

void Config::saveAccessCode(const std::string& code) {
    access_code = code;
}
