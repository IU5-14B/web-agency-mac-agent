#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "utils/Utils.h"

namespace fs = std::filesystem;

class UtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "test_utils_dir";
        fs::create_directory(test_dir);
    }
    
    void TearDown() override {
        fs::remove_all(test_dir);
    }
    
    std::string test_dir;
};

// TODO: Добавить тесты для утилит, когда они появятся
TEST_F(UtilsTest, Placeholder) {
    EXPECT_TRUE(true);
}

// Пример теста для будущих функций работы с файлами
TEST_F(UtilsTest, CreateDirectoryIfNotExists) {
    std::string new_dir = test_dir + "/subdir";
    
    // TODO: когда будет функция - раскомментировать
    // bool result = Utils::createDirectory(new_dir);
    // EXPECT_TRUE(result);
    // EXPECT_TRUE(fs::exists(new_dir));
    
    SUCCEED();
}
