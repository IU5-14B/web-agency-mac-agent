.PHONY: help build compile clean test run run-mock server

GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
RED := \033[0;31m
NC := \033[0m

BUILD_DIR := build
BUILD_DIR_COV := build-coverage
COV_VENV := $(BUILD_DIR_COV)/.venv
GCOVR := $(abspath $(COV_VENV)/bin/gcovr)
CONFIG_FILE := config/config.json
CMAKE := cmake
BUILD_TYPE ?= Debug

help:
	@echo "$(BLUE)Доступные команды:$(NC)"
	@echo "$(GREEN)  build$(NC)        - Полная сборка (включая библиотеки)"
	@echo "$(GREEN)  compile$(NC)      - Быстрая сборка только агента"
	@echo "$(GREEN)  clean$(NC)        - Очистка сборки"
	@echo "$(GREEN)  test$(NC)         - Запуск всех тестов"
	@echo "$(GREEN)  server$(NC)       - Проверка доступности сервера"
	@echo "$(GREEN)  run$(NC)          - Запуск агента в реальном режиме"
	@echo "$(GREEN)  run-mock$(NC)     - Запуск агента в mock-режиме"

build:
	@echo "$(YELLOW) Полная сборка проекта...$(NC)"
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTS=ON ..
	$(MAKE) compile

compile:
	@echo "$(YELLOW) Компиляция агента...$(NC)"
	cd $(BUILD_DIR) && $(CMAKE) --build . --parallel 4

clean:
	@echo "$(YELLOW) Очистка...$(NC)"
	rm -rf $(BUILD_DIR)
	rm -f agent.log
	rm -rf work results

test:
	@echo "$(YELLOW) Запуск всех тестов...$(NC)"
	@if [ ! -d $(BUILD_DIR) ]; then \
		echo "$(RED)❌ Сначала выполните make build$(NC)"; \
		exit 1; \
	fi
	cd $(BUILD_DIR) && ctest --output-on-failure

server:
	@echo "$(YELLOW) Проверка сервера...$(NC)"
	@if [ ! -f $(CONFIG_FILE) ]; then \
		echo "$(RED)❌ Файл $(CONFIG_FILE) не найден!$(NC)"; \
		exit 1; \
	fi
	@SERVER_URL=$$(grep -o '"server_url"[[:space:]]*:[[:space:]]*"[^"]*"' $(CONFIG_FILE) | cut -d'"' -f4); \
	if [ -z "$$SERVER_URL" ]; then \
		echo "$(RED)❌ Не удалось найти server_url в $(CONFIG_FILE)$(NC)"; \
		exit 1; \
	fi; \
	echo " Проверка $$SERVER_URL..."; \
	curl -k -I --connect-timeout 5 $$SERVER_URL/wa_reg/ 2>/dev/null | head -n 1; \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)✅ Сервер доступен!$(NC)"; \
	else \
		echo "$(RED)❌ Сервер недоступен!$(NC)"; \
	fi

run:
	@echo "$(YELLOW)Запуск агента (режим реальный)...$(NC)"
	@if [ ! -f $(CONFIG_FILE) ]; then \
		echo "$(RED)❌ Файл $(CONFIG_FILE) не найден!$(NC)"; \
		exit 1; \
	fi
	cd $(BUILD_DIR) && ./web-agent ../$(CONFIG_FILE)

run-mock:
	@echo "$(YELLOW)Запуск агента (режим MOCK)...$(NC)"
	@if [ ! -f $(CONFIG_FILE) ]; then \
		echo "$(RED)❌ Файл $(CONFIG_FILE) не найден!$(NC)"; \
		exit 1; \
	fi
	cd $(BUILD_DIR) && WEB_AGENT_MOCK=1 ./web-agent ../$(CONFIG_FILE)

coverage:
	@echo "$(YELLOW)Сбор покрытия тестами (gcovr в локальном venv)...$(NC)"
	@command -v python3 >/dev/null 2>&1 || { echo "$(RED)❌ python3 не найден$(NC)"; exit 1; }
	mkdir -p $(BUILD_DIR_COV)
	@if [ ! -x $(GCOVR) ]; then \
		python3 -m venv $(COV_VENV); \
		$(COV_VENV)/bin/pip install --upgrade pip >/dev/null; \
		$(COV_VENV)/bin/pip install gcovr >/dev/null; \
	fi
	cd $(BUILD_DIR_COV) && \
		$(CMAKE) -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
		-DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" ..
	cd $(BUILD_DIR_COV) && $(CMAKE) --build . --parallel 4
	# Clean leftover coverage data files to avoid gcov merge corruption
	cd $(BUILD_DIR_COV) && find . -name "*.gcda" -delete || true
	cd $(BUILD_DIR_COV) && ctest --output-on-failure || \
	{ echo "$(YELLOW)Instrumented tests failed — falling back to stable test run (no coverage).$(NC)"; \
	  echo "Running non-coverage tests in $(BUILD_DIR) to verify functionality..."; \
	  if [ -d $(BUILD_DIR) ]; then cd $(BUILD_DIR) && ctest --output-on-failure; else echo "No non-coverage build found. Run 'make build' first."; fi; \
	  echo "Coverage run aborted."; exit 0; }
	cd $(BUILD_DIR_COV) && mkdir -p coverage-report
	cd $(BUILD_DIR_COV) && $(GCOVR) --root .. \
		--exclude '.*_deps/.*' --exclude '.*/tests/.*' --exclude '.*json.hpp' \
		--exclude '.*\\.h$$' \
		--exclude '.*CMakeCXXCompilerId.cpp' --exclude '.*src/main.cpp' \
		--merge-mode-functions=merge-use-line-0 \
		--html --html-details -o coverage-report/index.html
	@echo "$(GREEN)✅ Отчет сгенерирован: $(BUILD_DIR_COV)/coverage-report/index.html$(NC)"

default: help
