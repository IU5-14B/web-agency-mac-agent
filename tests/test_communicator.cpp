#include <gtest/gtest.h>
#include "communicator/Communicator.h"

class CommunicatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Используем mock-режим для тестов
        comm = std::make_unique<Communicator>("https://test.com/api", true);
    }
    
    std::unique_ptr<Communicator> comm;
};

TEST_F(CommunicatorTest, MockRegisterReturnsCode) {
    auto result = comm->registerAgent("test-uid");
    
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
    
    // Проверяем формат (должен содержать дефисы как в реальном API)
    EXPECT_EQ(result->length(), 30);
}

TEST_F(CommunicatorTest, MockFetchTaskReturnsJson) {
    auto result = comm->fetchTask("test-uid", "test-code");
    
    ASSERT_TRUE(result.has_value());
    
    // Проверяем структуру ответа
    EXPECT_TRUE(result->contains("code_responce") || result->contains("code_response"));
}

TEST_F(CommunicatorTest, MockFetchTaskCyclesThroughResponses) {
    // Первый запрос - обычно WAIT (в нашей mock-логике)
    auto r1 = comm->fetchTask("test-uid", "test-code");
    ASSERT_TRUE(r1.has_value());
    
    // Второй запрос
    auto r2 = comm->fetchTask("test-uid", "test-code");
    ASSERT_TRUE(r2.has_value());
    
    // Третий запрос - должен быть TASK (каждые 3 раза)
    auto r3 = comm->fetchTask("test-uid", "test-code");
    ASSERT_TRUE(r3.has_value());
    
    // Проверяем, что третий ответ содержит задание
    int code = -1;
    if (r3->contains("code_response")) {
        code = (*r3)["code_response"].get<int>();
    } else if (r3->contains("code_responce")) {
        code = (*r3)["code_responce"].get<int>();
    }
    
    EXPECT_EQ(code, 1); // Должно быть задание
}

TEST_F(CommunicatorTest, MockSendResultReturnsTrue) {
    bool result = comm->sendResult("test-uid", "test-code", 
                                   "session-123", 0, "success", 
                                   0, {});
    
    EXPECT_TRUE(result);
}
