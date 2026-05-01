#pragma once

#include "cmdline.h"

#include <any>
#include <map>
#include <mutex>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <stdexcept>

namespace vtb {

class ConfigManager {
public:
   static ConfigManager& get_instance();
   ConfigManager(const ConfigManager&) = delete;
   ConfigManager& operator=(const ConfigManager&) = delete;
   bool init(int argc, char** argv);

private:
   ConfigManager();
   ~ConfigManager();
   CmdlineParser parser_;

public:
   template <typename T>
   T get_arg(std::string_view name) const {
      return parser_.get<T>(name);
   }
};

}  // namespace vtb
