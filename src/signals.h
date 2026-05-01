#pragma once

#include <atomic>

namespace vtb {

int keep_alive_thread(void*);
void signal_handler(int signal);
void setup_signal_handler();
void signal_handler_crash(int sig);

static std::atomic<bool> keep_running{true};

}  // namespace vtb
