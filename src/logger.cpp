#include "logger.h"

#include <chrono>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <string_view>
#include <iostream>

namespace vtb {

Logger& Logger::get_instance() {
   static Logger instance;
   return instance;
}

Logger::Logger() {
}

void Logger::init(const std::string& filename, LogLevel level) {
   if (initialized_.exchange(true)) return;

   level_ = level;
   log_filename_ = filename;

   if (!log_filename_.empty()) {
      file_.open(log_filename_, std::ios::out);
   }

   running_ = true;
   flush_thread_ = std::thread(&Logger::flush_loop_, this);
}

void Logger::shutdown() {
   vtb::Logger::get_instance().direct_append("[VTBMAIN] Shutdown: Logger\n");
   bool expected = true;
   // Ensure shutdown only executes once
   if (!running_.compare_exchange_strong(expected, false)) {
      return; 
   }

   cv_.notify_all();
   if (flush_thread_.joinable()) {
      flush_thread_.join();
   }
   
   // Final drain to ensure no logs are lost after thread joins
   if (!active_buffer_.empty()) {
      if (file_.is_open()) {
         file_ << active_buffer_ << std::flush;
      }
      std::cout << active_buffer_ << std::flush;
      active_buffer_.clear();
   }

   if (file_.is_open()) {
      file_.close();
   }
}

void Logger::emergency_flush() {
   // Try to lock; if fail, the crashing thread might own it, proceed anyway
   std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);

   if (!active_buffer_.empty()) {
      if (file_.is_open()) {
         file_ << "--- EMERGENCY FLUSH START ---\n";
         file_ << active_buffer_;
         file_ << "\n--- EMERGENCY FLUSH END ---\n";
         file_.flush(); 
      }
      fprintf(stderr, "%s", active_buffer_.c_str());
      active_buffer_.clear();
   }
}

void Logger::direct_append(const std::string& msg) {
   std::lock_guard<std::mutex> lock(mutex_);
   active_buffer_ += msg;
}

void Logger::log(LogLevel msg_level, std::string&& message) {
   if (msg_level <= level_) {
      std::lock_guard<std::mutex> lock(mutex_);
      active_buffer_.append(get_level_name_(msg_level)).append(message).append("\n");
   }
}

void Logger::flush_loop_() {
   while (running_) {
      std::string to_write;
      {
         std::unique_lock<std::mutex> lock(mutex_);
         cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { 
            return !running_ || !active_buffer_.empty(); 
         });
         
         // Swap strategy to minimize lock contention
         to_write.swap(active_buffer_);
      }

      if (!to_write.empty()) {
         if (file_.is_open()) {
            file_ << to_write << std::flush;
         }
         std::cout << to_write << std::flush;
      }
   }
}

Logger::~Logger() {
   shutdown();
}

std::string_view Logger::get_level_name_(LogLevel level) {
    switch (level) {
        case LogLevel::FATAL:   return "[..FATAL] *** ";
        case LogLevel::ERROR:   return "[..ERROR] *** ";
        case LogLevel::WARNING: return "[WARNING] *** ";
        case LogLevel::INFO:    return "[...INFO] ";
        case LogLevel::DEBUG:   return "[..DEBUG] ";
        case LogLevel::TRACE:   return "[..TRACE] ";
        default:                return "[UNKNOWN] ";
    }
}

void set_verbosity(std::string level_str) {
   std::transform(level_str.begin(), level_str.end(), level_str.begin(), ::toupper);

   static const std::unordered_map<std::string, LogLevel> verbosity_map = {
      {"FATAL",   LogLevel::FATAL},
      {"ERROR",   LogLevel::ERROR},
      {"WARNING", LogLevel::WARNING},
      {"INFO",    LogLevel::INFO},
      {"DEBUG",   LogLevel::DEBUG},
      {"TRACE",   LogLevel::TRACE}
   };

   auto it = verbosity_map.find(level_str);
   if (it != verbosity_map.end()) {
      vtb::Logger::get_instance().set_level(it->second);
   } else {
      std::cerr << "[WARNING] Invalid log level '" << level_str << "'. Defaulting to INFO.\n";
   }
}

}  // namespace vtb
