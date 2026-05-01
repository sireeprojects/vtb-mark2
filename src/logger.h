#pragma once

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace vtb {

enum class LogLevel {
   FATAL = 0,
   ERROR = 1,
   WARNING = 2,
   INFO = 3,
   DEBUG = 4,
   TRACE = 5
};

class Logger {
public:
   static Logger& get_instance();

   // Initialize the logger with a filename and verbosity level
   void init(const std::string& filename, LogLevel level);
   
   // Explicitly shut down the logger, joining the background thread
   void shutdown();

   LogLevel get_level() const { return level_; }

   void set_level(LogLevel msg_level) {
      level_ = msg_level;
   }
   
   // Logs a message asynchronously using rvalue move semantics[cite: 2]
   void log(LogLevel msg_level, std::string&& message);

   // Emergency flush for crashes; uses try_lock to avoid deadlocks
   void emergency_flush();
   
   // Bypasses the swap logic to append directly to the buffer
   void direct_append(const std::string& msg);

   ~Logger();

private:
   Logger();
   Logger(const Logger&) = delete;
   Logger& operator=(const Logger&) = delete;

   void flush_loop_();
   std::string_view get_level_name_(LogLevel level);

   std::string log_filename_;
   std::ofstream file_;
   
   std::string active_buffer_; 
   std::mutex mutex_;
   std::thread flush_thread_;
   std::condition_variable cv_;

   LogLevel level_{LogLevel::INFO};
   
   std::atomic<bool> running_{false};
   std::atomic<bool> initialized_{false};
};

}  // namespace vtb
