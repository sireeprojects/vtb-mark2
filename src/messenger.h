#pragma once

#include <sstream>
#include <string>
#include "logger.h"

namespace vtb {

class LogStream {
public:
   LogStream(LogLevel level, const std::string& prefix) 
       : level_(level), prefix_(prefix) {}
   
   ~LogStream() {
      // Moves the internal string buffer to the logger's background queue[cite: 3]
      Logger::get_instance().log(level_, std::move(oss_.str()));
   }

   template <typename T>
   LogStream& operator<<(const T& value) {
      oss_ << value;
      return *this;
   }

private:
   LogLevel level_;
   std::string prefix_;
   std::ostringstream oss_;
};

void set_verbosity(std::string level_str);

} // namespace vtb

// Standard logging macro: checks level before stream creation[cite: 3]
#define VTB_LOG(level) \
    if (vtb::Logger::get_instance().get_level() >= vtb::LogLevel::level) \
        vtb::LogStream(vtb::LogLevel::level, "[" #level "] ")

// Conditional Debug Macro: Completely optimized out if VTB_DEBUG is not defined
#ifdef VTB_DEBUG
    #define VTB_DEBUG_LOG(level) VTB_LOG(level)
#else
    // The compiler sees that the 'else' branch is unreachable and removes 
    // the entire line, including any function calls or packet data lookups.
    #define VTB_DEBUG_LOG(level) if (true) {} else vtb::LogStream(vtb::LogLevel::level, "")
#endif
