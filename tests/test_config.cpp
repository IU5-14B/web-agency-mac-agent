#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "config/Config.h"

namespace fs = std::filesystem;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаём тестовый конфиг перед каждым тестом
        test_file = "test_config.json";
    }
    
    void TearDown() override {
        // Удаляем тестовый конфиг после каждого теста
        if (fs::exists(test_file)) {
            fs::remove(test_file);
        }
    }
    
    std::string test_file;
};

// Тест 1: Загрузка существующего файла
TEST_F(ConfigTest, LoadExistingFile) {
    // Создаём тестовый JSON
    std::ofstream f(test_file);
    f << R"({
        "uid": "test123",
        "server_url": "https://test.com/api",
        "poll_interval_sec": 10,
        "work_dir": "./work",
        "results_dir": "./results",
        "log_file": "./agent.log",
        "log_level": "debug",
        "access_code": "abc123"
    })";
    f.close();
    
    Config cfg;
    bool result = cfg.loadFromFile(test_file);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(cfg.uid, "test123");
    EXPECT_EQ(cfg.server_url, "https://test.com/api");
    EXPECT_EQ(cfg.poll_interval_sec, 10);
    EXPECT_EQ(cfg.work_dir, "./work");
    EXPECT_EQ(cfg.results_dir, "./results");
    EXPECT_EQ(cfg.log_file, "./agent.log");
    EXPECT_EQ(cfg.log_level, "debug");
    EXPECT_EQ(cfg.access_code, "abc123");
}

// Тест 2: Загрузка несуществующего файла
TEST_F(ConfigTest, LoadNonExistingFile) {
    Config cfg;
    bool result = cfg.loadFromFile("nonexistent.json");
    
    EXPECT_FALSE(result);
}

// Тест 3: Загрузка с пустыми полями
TEST_F(ConfigTest, LoadWithMissingFields) {
    std::ofstream f(test_file);
    f << R"({
        "uid": "test123"
    })";
    f.close();
    
    Config cfg;
    bool result = cfg.loadFromFile(test_file);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(cfg.uid, "test123");
    EXPECT_EQ(cfg.server_url, "");
    EXPECT_EQ(cfg.poll_interval_sec, 5);
}

// Тест 4: Сохранение access_code
TEST_F(ConfigTest, SaveAccessCode) {
    // Сначала загружаем конфиг
    std::ofstream f(test_file);
    f << R"({
        "uid": "test123",
        "server_url": "https://test.com/api",
        "poll_interval_sec": 10,
        "work_dir": "./work",
        "results_dir": "./results",
        "log_file": "./agent.log",
        "log_level": "debug",
        "access_code": ""
    })";
    f.close();
    
    Config cfg;
    cfg.loadFromFile(test_file);
    
    cfg.saveAccessCode("new_code_456");
    
    EXPECT_EQ(cfg.access_code, "new_code_456");
    
    Config cfg2;
    cfg2.loadFromFile(test_file);
    EXPECT_EQ(cfg2.access_code, "new_code_456");
}
