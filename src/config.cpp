#include "config.h"
#include "messenger.h"

#include <iomanip>
#include <sstream>

namespace vtb {

ConfigManager::ConfigManager() {
   parser_.add_argument("--help",
                        "-h", 
                        "Show this help menu", 
                        false, 
                        "false");
   parser_.add_argument("--mode", 
                        "-m", 
                        "Loopback | Back2Back | Emulator", 
                        false, 
                        "Loopback");
   parser_.add_argument("--client", 
                        "-c", 
                        "Operate as Vhost Client", 
                        false, 
                        "false");
   parser_.add_argument("--threading-mode",
                        "-thmode",
                        "EachQ-TwoThread | EachQ-OneThread | AllQ-TwoThread | AllQ-OneThread",
                        false,
                        "EachQ-OneThread");
   parser_.add_argument("--abstract-sockname",
                        "-absn", 
                        "Specify a random name. For internal use only", 
                        false, 
                        "cm_to_ph_sock");
   parser_.add_argument("--port-data-sockname",
                        "-pdsn", 
                        "Unix socket path to connect to port data plane", 
                        false, 
                        "/tmp/port_data.sock");
   parser_.add_argument("--port-control-sockname",
                        "-pcsn", 
                        "Unix socket path to connect to port control plane", 
                        false, 
                        "/tmp/port_ctrl.sock");
   parser_.add_argument("--vhost-sockname",
                        "-vsn", 
                        "Unix socket path to connect to virtual machine", 
                        false, 
                        "/tmp/vhost.sock");
   parser_.add_argument("--verbosity", 
                        "-v", 
                        "Fatal | Error | Warning | Info | Debug | Trace", 
                        false, 
                        "Info");
}

ConfigManager& ConfigManager::get_instance() {
   static ConfigManager instance;
   return instance;
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::init(int argc, char** argv) {
   try {
      parser_.parse(argc, argv);
      if (parser_.get<bool>("--help")) {
         parser_.print_usage();
         return false;
      }
      return true;
   } catch (const std::exception& e) {
      VTB_LOG(ERROR) << "Init Error: " << e.what();
      parser_.print_usage();
      return false;
   }
}

}  // namespace vtb
