#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "executor/TaskExecutor.h"
#include "config/Config.h"

namespace fs = std::filesystem;

class TaskExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        base_dir = fs::temp_directory_path() / ("wa_task_exec_" + std::to_string(ts));
        work_dir = base_dir / "work";
        results_dir = base_dir / "results";
        fs::create_directories(work_dir);
        fs::create_directories(results_dir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(base_dir, ec);
    }

    fs::path base_dir;
    fs::path work_dir;
    fs::path results_dir;
};

TEST_F(TaskExecutorTest, ExecutesCommandAndCopiesResults) {
    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "TASK";
    task.options = "echo hello > file.txt";
    task.session_id = "s1";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, 0);
    EXPECT_FALSE(out_files.empty());
    EXPECT_TRUE(fs::exists(results_dir / "file.txt"));

    bool work_has_files = false;
    for (const auto& entry : fs::directory_iterator(work_dir)) {
        if (entry.is_regular_file()) {
            work_has_files = true;
            break;
        }
    }
    EXPECT_FALSE(work_has_files);
}

TEST_F(TaskExecutorTest, TimesOutLongCommand) {
    TaskExecutor executor(work_dir.string(), results_dir.string());
    executor.setTimeout(std::chrono::seconds(1));

    Task task;
    task.task_code = "TASK";
    task.options = "sleep 2";
    task.session_id = "s2";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -3);
    EXPECT_TRUE(out_files.empty());
    EXPECT_NE(message.find("timed out"), std::string::npos);
}

TEST_F(TaskExecutorTest, CommandFailureReturnsError) {
    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "TASK";
    task.options = "false";  // exits with non-zero
    task.session_id = "s_fail";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -2);
    EXPECT_TRUE(out_files.empty());
}

TEST_F(TaskExecutorTest, SendsExistingFileFromResults) {
    std::ofstream(results_dir / "artifact.txt") << "data";

    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "FILE";
    task.options = "artifact.txt";
    task.session_id = "s3";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, 0);
    ASSERT_EQ(out_files.size(), 1u);
    EXPECT_EQ(fs::path(out_files[0]).filename(), fs::path("artifact.txt"));
    EXPECT_NE(message.find("File found"), std::string::npos);
}

TEST_F(TaskExecutorTest, NoActionWhenOptionsEmpty) {
    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "TASK";
    task.options = "";
    task.session_id = "s4";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(out_files.empty());
    EXPECT_NE(message.find("no action"), std::string::npos);
}

TEST_F(TaskExecutorTest, FileNotFoundReturnsError) {
    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "FILE";
    task.options = "absent.txt";
    task.session_id = "s_missing";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -2);
    EXPECT_TRUE(out_files.empty());
}

TEST_F(TaskExecutorTest, UnsupportedTaskCode) {
    TaskExecutor executor(work_dir.string(), results_dir.string());

    Task task;
    task.task_code = "UNKNOWN";
    task.options = "";
    task.session_id = "s_unknown";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -1);
    EXPECT_TRUE(out_files.empty());
}

TEST_F(TaskExecutorTest, TimeoutTaskChangesTimeout) {
    fs::path cfg_path = work_dir / "cfg.json";
    std::ofstream(cfg_path) << R"({"uid":"u","server_url":"s","poll_interval_sec":3,"work_dir":"./w","results_dir":"./r","log_file":"./l","log_level":"info","access_code":""})";

    Config cfg;
    ASSERT_TRUE(cfg.loadFromFile(cfg_path.string()));

    TaskExecutor executor(work_dir.string(), results_dir.string(), &cfg);

    Task task;
    task.task_code = "TIMEOUT";
    task.options = "5";
    task.session_id = "s_timeout";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(out_files.empty());
    EXPECT_EQ(cfg.poll_interval_sec, 5);
}

TEST_F(TaskExecutorTest, ConfigTaskChangesPollInterval) {
    // Prepare config file to persist changes
    fs::path cfg_path = work_dir / "cfg.json";
    std::ofstream(cfg_path) << R"({"uid":"u","server_url":"s","poll_interval_sec":3,"work_dir":"./w","results_dir":"./r","log_file":"./l","log_level":"info","access_code":""})";

    Config cfg;
    ASSERT_TRUE(cfg.loadFromFile(cfg_path.string()));

    TaskExecutor executor(work_dir.string(), results_dir.string(), &cfg);

    Task task;
    task.task_code = "CONFIG";
    task.options = "poll_interval=7";
    task.session_id = "s_cfg";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(out_files.empty());
    EXPECT_EQ(cfg.poll_interval_sec, 7);
}

TEST_F(TaskExecutorTest, ConfigTaskUnknownParamFails) {
    Config cfg;
    // create minimal config to set last_config_path_
    fs::path cfg_path = work_dir / "cfg.json";
    std::ofstream(cfg_path) << R"({"uid":"u","server_url":"s"})";
    ASSERT_TRUE(cfg.loadFromFile(cfg_path.string()));

    TaskExecutor executor(work_dir.string(), results_dir.string(), &cfg);

    Task task;
    task.task_code = "CONFIG";
    task.options = "unknown=value";
    task.session_id = "s_cfg_bad";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -2);
    EXPECT_TRUE(out_files.empty());
}

TEST_F(TaskExecutorTest, TimeoutTaskWithoutConfigFails) {
    TaskExecutor executor(work_dir.string(), results_dir.string(), nullptr);

    Task task;
    task.task_code = "TIMEOUT";
    task.options = "10";
    task.session_id = "s_timeout_fail";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -3);
    EXPECT_TRUE(out_files.empty());
}

TEST_F(TaskExecutorTest, ConfigTaskInvalidValue) {
    Config cfg;
    fs::path cfg_path = work_dir / "cfg.json";
    std::ofstream(cfg_path) << R"({"uid":"u","server_url":"s","poll_interval_sec":3,"work_dir":"./w","results_dir":"./r","log_file":"./l","log_level":"info","access_code":""})";
    ASSERT_TRUE(cfg.loadFromFile(cfg_path.string()));

    TaskExecutor executor(work_dir.string(), results_dir.string(), &cfg);

    Task task;
    task.task_code = "TIMEOUT";
    task.options = "not_number";
    task.session_id = "s_timeout_bad";
    task.status = "RUN";

    std::string message;
    std::vector<std::string> out_files;

    int rc = executor.execute(task, message, out_files);

    EXPECT_EQ(rc, -2);
    EXPECT_TRUE(out_files.empty());
}
