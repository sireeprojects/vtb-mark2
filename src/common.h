#pragma once

#include <thread>

#include "messenger.h"

namespace vtb {

enum class ThreadMode {
   EachQTwoThread,
   EachQOneThread,
   AllQTwoThread,
   AllQOneThread
};

ThreadMode string_to_thread_mode(std::string_view mode_str);

int create_server_socket(const std::string& path);
int create_client_socket(const std::string& path);

void set_thread_name(std::thread& th, const std::string& name);

void restore_echoctl();
void disable_echoctl();

void graceful_exit();

bool is_even(int n);
bool is_odd(int n);

template <typename T>
bool send_packet(int fd, const T& data) {
   ssize_t bytes_sent = write(fd, &data, sizeof(T));

   if (bytes_sent == -1) {
      perror("write");
      return false;
   } else if (static_cast<size_t>(bytes_sent) < sizeof(T)) {
      // Handle partial writes if necessary for large buffers
      VTB_LOG(INFO) << "Warning: Partial write occurred";
      return false;
   }
   return true;
}

}  // namespace vtb
