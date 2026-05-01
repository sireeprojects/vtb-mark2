#include <rte_launch.h>
#include <rte_mbuf.h>

#include "messenger.h"
#include "common.h"
#include "signals.h"
#include "cmdline.h"
#include "config.h"
#include <iostream>

int main(int argc, char** argv) {

   // Logger
   vtb::Logger::get_instance().init(
      "run.log", vtb::LogLevel::INFO);

   // Signal handlers
   vtb::disable_echoctl();
   vtb::setup_signal_handler();

   // Configuration manager
   auto& config = vtb::ConfigManager::get_instance();
   if (!config.init(argc, argv)) {
      VTB_LOG(ERROR) << "Config.Init failed";
      vtb::Logger::get_instance().shutdown();
      vtb::restore_echoctl();
      return 0;
   }

   // Set user verbosity, overrides default verbosity
   auto verbosity = config.get_arg<std::string>("--verbosity");
   vtb::set_verbosity(verbosity);

   rte_eal_init(argc, argv); // temp placeholder

   unsigned int keepalive_lcore = rte_get_next_lcore(rte_get_main_lcore(), true, 0);
   VTB_LOG(TRACE) << "VTB App Keep-Alive thread started on: " << keepalive_lcore;
   rte_eal_remote_launch(vtb::keep_alive_thread, NULL, keepalive_lcore);
   rte_eal_wait_lcore(keepalive_lcore);

   // Controlled shutdown 
   vtb::Logger::get_instance().shutdown();
   vtb::restore_echoctl();
   return 0;
}
