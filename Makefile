#/*
#**  _                                              _      ___    ___
#** | |                                            | |    |__ \  / _ \
#** | |_Created _       _ __   _ __    ___    __ _ | |__     ) || (_) |
#** | '_ \ | | | |     | '_ \ | '_ \  / _ \  / _` || '_ \   / /  \__, |
#** | |_) || |_| |     | | | || | | || (_) || (_| || | | | / /_    / /
#** |_.__/  \__, |     |_| |_||_| |_| \___/  \__,_||_| |_||____|  /_/
#**          __/ |     on 25/06/2026.
#**         |___/
#*/


##
## metricd
## File description:
## Makefile
##

# === CONFIGURATION ===
NAME       := metricd
LIB_NAME   := libmetricd_core.a
TEST_NAME  := metricd_tests

CXX        := g++
CXXFLAGS   := -std=c++20 -Wall -Wextra -Iinclude -Isrc -ILogger -fPIC
LDFLAGS    :=
LIBS       :=

DEBUG ?= 0
ifeq ($(DEBUG),1)
	CXXFLAGS += -g -DMETRICD_DEBUG
else
	CXXFLAGS += -O2
endif

# === VERBOSE SWITCH ===
ifndef V
	SILENT = @
else
	SILENT =
endif

# === COLORS ===
GREEN  := $(shell echo -e "\033[0;32m")
RED    := $(shell echo -e "\033[0;31m")
VIOLET := $(shell echo -e "\033[0;35m")
BLUE   := $(shell echo -e "\033[0;34m")
NC     := $(shell echo -e "\033[0m")

# === DIRECTORIES ===
SRC_DIR      := src
INC_DIR      := include
APP_DIR      := app
TEST_DIR     := tests
OBJ_DIR      := obj

# === LIB SOURCES (src/) ===
LIB_SRCS := \
	$(SRC_DIR)/core/Daemon.cpp \
	$(SRC_DIR)/core/Config.cpp \
	Logger/Logger.cpp \
	$(SRC_DIR)/collectors/CpuCollector.cpp \
	$(SRC_DIR)/collectors/MemoryCollector.cpp \
	$(SRC_DIR)/collectors/DiskCollector.cpp \
	$(SRC_DIR)/collectors/NetworkCollector.cpp \
	$(SRC_DIR)/ipc/Server.cpp \
	$(SRC_DIR)/ipc/ClientSession.cpp \
	$(SRC_DIR)/serialization/JsonSerializer.cpp

LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# === APP SOURCE ===
APP_SRCS := $(APP_DIR)/main.cpp
APP_OBJS := $(APP_SRCS:%.cpp=$(OBJ_DIR)/%.o)

# === TEST SOURCES ===
TEST_SRCS := \
	$(TEST_DIR)/unit/test_cpu_collector.cpp \
	$(TEST_DIR)/unit/test_json_serializer.cpp

TEST_OBJS := $(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)

# === RULES ===
all: $(NAME)
	@echo "$(GREEN)[OK] Full build complete.$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/$(APP_DIR)/%.o: $(APP_DIR)/%.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_NAME): $(LIB_OBJS)
	$(SILENT)ar rcs $@ $^
	@echo "$(GREEN)[OK] Static library built.$(NC)"

$(NAME): $(LIB_NAME) $(APP_OBJS)
	$(SILENT)$(CXX) $(CXXFLAGS) $(APP_OBJS) -o $@ -L. -lmetricd_core $(LDFLAGS)
	@echo "$(GREEN)[OK] Binary built: $(NAME)$(NC)"

$(TEST_NAME): $(LIB_NAME) $(TEST_OBJS)
	$(SILENT)$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $@ -L. -lmetricd_core $(LDFLAGS)
	@echo "$(GREEN)[OK] Tests built: $(TEST_NAME)$(NC)"

test: $(TEST_NAME)
	$(SILENT)./$(TEST_NAME)
	@echo "$(GREEN)[OK] Tests passed.$(NC)"

clean:
	$(SILENT)$(RM) -r $(OBJ_DIR)
	@echo "$(VIOLET)[CLEAN] Object files removed.$(NC)"

fclean: clean
	$(SILENT)$(RM) $(LIB_NAME) $(NAME) $(TEST_NAME)
	@echo "$(VIOLET)[FCLEAN] Binaries and library removed.$(NC)"

re: fclean all

.PHONY: all clean fclean re test
