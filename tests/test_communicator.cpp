#include <gtest/gtest.h>
#include "communicator/Communicator.h"
#include <regex>

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

    // ensure format matches pattern sections 6-4-4-4-8
    std::regex re("^[0-9a-f]{6}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{8}$");
    EXPECT_TRUE(std::regex_match(*result, re));
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

TEST_F(CommunicatorTest, MockSendResultSkipsMissingFile) {
    // Provide non-existent file to ensure it logs and continues
    std::vector<std::string> files = {"/tmp/definitely_missing_file.bin"};
    bool result = comm->sendResult("test-uid", "test-code",
                                   "session-999", 0, "msg",
                                   static_cast<int>(files.size()), files);
    EXPECT_TRUE(result);
}

TEST_F(CommunicatorTest, MockFetchTaskWaitResponse) {
    // First call returns WAIT in mock cycle
    auto r = comm->fetchTask("uid", "code");
    ASSERT_TRUE(r.has_value());
    int code = -1;
    if (r->contains("code_response")) {
        code = (*r)["code_response"].get<int>();
    } else if (r->contains("code_responce")) {
        code = (*r)["code_responce"].get<int>();
    }
    EXPECT_EQ(code, 0);
}

TEST_F(CommunicatorTest, BuildUrlHandlesTrailingSlash) {
    Communicator c1("https://example.com/api/", true);
    EXPECT_EQ(c1.buildUrl("wa_reg/"), "https://example.com/api/wa_reg/");

    Communicator c2("https://example.com/api", true);
    EXPECT_EQ(c2.buildUrl("wa_reg/"), "https://example.com/api/wa_reg/");
}

TEST_F(CommunicatorTest, SetMockModeToggles) {
    Communicator c("", false);
    c.setMockMode(true);
    // No direct getter; ensure registerAgent uses mock when enabled
    auto r = c.registerAgent("u1");
    EXPECT_TRUE(r.has_value());
}
