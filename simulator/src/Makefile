# Makefile for Mock Hardware Library
#
# 编译libBPMboardMock.so - PC模拟硬件库
#
# 使用方法:
#   make          - 编译库
#   make clean    - 清理
#   make install  - 安装到lib目录
#   make test     - 运行测试

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 -g -std=c99
LDFLAGS = -shared
LIBS = -lm -lpthread

# 目标库名
TARGET = libBPMboardMock.so

# 源文件
SRCS = mockHardware.c dataGenerator.c fileReplay.c faultInjection.c
HEADERS = mockHardware.h dataGenerator.h fileReplay.h faultInjection.h

# 对象文件
OBJS = $(SRCS:.c=.o)

# 安装路径
INSTALL_DIR = ../../lib/linux-x86_64

# 默认目标
all: $(TARGET)

# 编译共享库
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build successful: $(TARGET)"
	@echo ""
	@echo "To use this library:"
	@echo "  export LD_LIBRARY_PATH=$(shell pwd):$$LD_LIBRARY_PATH"
	@echo "  export BPMIOC_HW_LIB=$(TARGET)"

# 编译对象文件
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	@echo "Cleaning..."
	rm -f $(TARGET) $(OBJS)
	rm -f test_mock
	@echo "Clean complete"

# 安装到lib目录
install: $(TARGET)
	@echo "Installing to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/
	@echo "Installed: $(INSTALL_DIR)/$(TARGET)"

# 测试程序
test: test_mock
	@echo "Running test..."
	./test_mock

test_mock: test_mock.c $(TARGET)
	$(CC) $(CFLAGS) -o $@ $< -L. -lBPMboardMock $(LIBS) -Wl,-rpath,.

# 显示帮助
help:
	@echo "Mock Hardware Library Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the mock library"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Install to lib directory"
	@echo "  make test     - Build and run test program"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Environment variables:"
	@echo "  BPMIOC_SIM_MODE      - Simulation mode (0=sine, 1=random, 2=replay)"
	@echo "  BPMIOC_DEBUG_LEVEL   - Debug level (0-4)"
	@echo ""

.PHONY: all clean install test help
