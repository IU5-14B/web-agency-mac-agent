#include <gtest/gtest.h>
#include "utils/TaskResponse.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Тест 1: Парсинг ответа "нет задания" (code_response = 0)
TEST(TaskResponseParserTest, HandlesWaitResponse) {
    json response = {
        {"code_responce", "0"},
        {"status", "WAIT"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, 0);
    EXPECT_EQ(result.status, "WAIT");
    EXPECT_TRUE(result.isWaiting());
    EXPECT_FALSE(result.hasTask());
    EXPECT_FALSE(result.isError());
}

// Тест 2: Парсинг ответа с заданием (code_response = 1)
TEST(TaskResponseParserTest, HandlesTaskResponse) {
    json response = {
        {"code_response", 1},
        {"task_code", "CONF"},
        {"options", "echo 'test'"},
        {"session_id", "test-session-123"},
        {"status", "RUN"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, 1);
    EXPECT_TRUE(result.hasTask());
    EXPECT_FALSE(result.isWaiting());
    EXPECT_FALSE(result.isError());
    
    ASSERT_TRUE(result.task.has_value());
    EXPECT_EQ(result.task->task_code, "CONF");
    EXPECT_EQ(result.task->options, "echo 'test'");
    EXPECT_EQ(result.task->session_id, "test-session-123");
    EXPECT_EQ(result.task->status, "RUN");
}

// Тест 3: Парсинг ошибки - неверный код доступа (code_response = -2)
TEST(TaskResponseParserTest, HandlesInvalidAccessCode) {
    json response = {
        {"code_response", -2},
        {"msg", "Invalid access code"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, -2);
    EXPECT_TRUE(result.isError());
    EXPECT_FALSE(result.hasTask());
    EXPECT_FALSE(result.isWaiting());
    EXPECT_EQ(result.message, "Invalid access code");
}

// Тест 4: Парсинг ответа с опечаткой в имени поля
TEST(TaskResponseParserTest, HandlesTypoInFieldName) {
    json response = {
        {"code_responce", 1},  // опечатка!
        {"task_code", "CONF"},
        {"session_id", "test-session"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, 1);
    EXPECT_TRUE(result.hasTask());
}

// Тест 5: Парсинг некорректного JSON (нет code_response)
TEST(TaskResponseParserTest, HandlesMissingCodeResponse) {
    json response = {
        {"status", "WAIT"}
        // нет code_response!
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, -1);  // наш код ошибки
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.message, "Missing code_response");
}

// Тест 6: Парсинг с числовым code_response (не строкой)
TEST(TaskResponseParserTest, HandlesNumericCodeResponse) {
    json response = {
        {"code_response", 0},  // число, не строка
        {"status", "WAIT"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, 0);
    EXPECT_TRUE(result.isWaiting());
}

// Тест 7: Парсинг задания без обязательных полей
TEST(TaskResponseParserTest, HandlesInvalidTaskFormat) {
    json response = {
        {"code_response", 1},
        {"task_code", "CONF"}
        // нет session_id!
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, -5);  // ошибка валидации
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.message, "Invalid task format");
}

// Тест 8: Парсинг с невалидным JSON (исключение)
TEST(TaskResponseParserTest, HandlesMalformedJson) {
    // Создадим json, который вызовет исключение при stoi
    json response = {
        {"code_response", "not_a_number"}
    };
    
    auto result = TaskResponseParser::parse(response);
    
    EXPECT_EQ(result.code, -4);  // ошибка парсинга
    EXPECT_TRUE(result.isError());
    EXPECT_TRUE(result.message.find("Parse error") != std::string::npos);
}
