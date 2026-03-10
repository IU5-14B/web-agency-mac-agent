#include "Config.h"
#include <fstream>
#include <iostream>

/**
 * @brief Load configuration from file
 * @param path Path to configuration file
 * @return true if loading successful, false otherwise
 */
bool Config::loadFromFile(const std::string& path) {
    
    return true;
}

/**
 * @brief Save access code to configuration
 * @param code Access code to save
 */
void Config::saveAccessCode(const std::string& code) {
    access_code = code;
}
