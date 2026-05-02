PROJ_ROOT := ..

# --- Compiler and Flags ---
CXX      := g++
# Using $(shell ...) is the most robust way to handle pkg-config in Make
CXXFLAGS := -I$(PROJ_ROOT)/src -Wall -Wextra -std=c++17 $(shell pkg-config --cflags libdpdk)
LDFLAGS  := $(shell pkg-config --libs libdpdk) -pthread

# --- Directories ---
SRC_DIR  := $(PROJ_ROOT)/src
OBJ_DIR  := obj
BIN_DIR  := .

# -g: Adds debug symbols
# -rdynamic: Exports symbols for backtrace readability
CXXFLAGS += -g -rdynamic
LDFLAGS  += -rdynamic
LDFLAGS  += -lstdc++fs

# --- Target ---
TARGET   := $(BIN_DIR)/vtb

# --- File Selection ---
# Explicitly list the files you want to include in the build[cite: 4]
SRCS_LIST := logger.cpp \
				 common.cpp \
				 signals.cpp \
				 cmdline.cpp \
				 config.cpp \
             vtb.cpp

# Prepend the source directory to the listed files[cite: 4]
SRCS     := $(addprefix $(SRC_DIR)/, $(SRCS_LIST))

# Generate Object list (maps src/*.cpp to obj/*.o)[cite: 4]
OBJS     := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# --- Formatting & Verbosity ---
ifndef V
    Q := @
else
    Q := 
endif

# Colors
BLUE  := \033[0;34m
GREEN := \033[0;32m
RESET := \033[0m
RED   := \033[0;31m

# --- Rules ---

all: $(TARGET)

# Special target to enable VTB_DEBUG for packet logging
debug: CXXFLAGS += -DVTB_DEBUG
debug: all

# Link the executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@printf "$(RED)Linking:   $(notdir $@)$(RESET)\n"
	$(Q)$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@printf "$(GREEN)Build Finished Successfully!$(RESET)\n"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@printf "$(BLUE)Compiling:$(RESET) $(notdir $<)\n"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGET) *.log

purge:
	rm -rf /tmp/vhost-user.sock
	rm -rf /dev/hugepages/v*

run:
	@./vtb -l 10-11 --file-prefix=vhost_backend --no-pci --no-telemetry \
			--log-level=lib.vhost.config:3 \
			--log-level=lib.vhost:error \
			--log-level=pmd.net.vhost:error \
			--log-level=lib.eal:3 \
			--log-level=lib.ring:8 \
			--log-level=lib.mempool:8 \
			--huge-unlink \
			-- \
			--verbosity Info

# Run with vdevs			
rund:
	@./vtb -l 10-11 --file-prefix=vhost_backend --no-pci --no-telemetry \
			--log-level=lib.vhost.config:3 \
			--log-level=lib.vhost:error \
			--log-level=pmd.net.vhost:error \
			--log-level=lib.eal:3 \
			--log-level=lib.ring:8 \
			--log-level=lib.mempool:8 \
			--huge-unlink \
			--vdev 'net_vhost0,iface=/tmp/vhost-user0,queues=1' \
			--vdev 'net_vhost1,iface=/tmp/vhost-user1,queues=2' \
			--vdev 'net_vhost2,iface=/tmp/vhost-user2,queues=3' \
			--vdev 'net_vhost3,iface=/tmp/vhost-user3,queues=4' \
			--vdev 'net_vhost4,iface=/tmp/vhost-user4,queues=5' \
			--vdev 'net_vhost5,iface=/tmp/vhost-user5,queues=6' \
			--vdev 'net_vhost6,iface=/tmp/vhost-user6,queues=7' \
			--vdev 'net_vhost7,iface=/tmp/vhost-user7,queues=8' \
			-- \
			--verbosity Info

# Run command for DPDK environment[cite: 4]
oldrun:
	@./vtb -l 1-2 --file-prefix=vhost_backend --no-pci --no-telemetry \
			--log-level=lib.vhost.config:3 \
			--log-level=lib.vhost:error \
			--log-level=pmd.net.vhost:error \
			--log-level=lib.eal:3 \
			--log-level=lib.ring:8 \
			--log-level=lib.mempool:8 \
			--huge-unlink \
			-- \
			--mode Loopback \
			--verbosity Trace \
			--threading-mode EachQ-OneThread \
			--abstract-sockname absockname \
			--port-data-sockname datasockname \
			--port-control-sockname controlsockname \
			--vhost-sockname /tmp/vhost-user.sock

.PHONY: all clean purge run noh debug
