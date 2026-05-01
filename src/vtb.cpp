#include <rte_launch.h>
#include <rte_mbuf.h>

#include "messenger.h"
#include "common.h"
#include "signals.h"
#include "cmdline.h"

int main() {
   // Initialize 
   vtb::Logger::get_instance().init("run.log", vtb::LogLevel::TRACE);

   vtb::disable_echoctl();
   vtb::setup_signal_handler();

   VTB_LOG(INFO) << "Starting Exhaustive Logger Test...";
   VTB_LOG(INFO) << "Press Ctrl+C to trigger the shutdown() sequence.";
   VTB_LOG(DEBUG) << "Debug Message";
   VTB_LOG(TRACE) << "Trace Message";

   unsigned next_core = rte_get_next_lcore(rte_get_main_lcore(), 1, 0);
   rte_eal_remote_launch(vtb::keep_alive_thread, NULL, next_core);
   rte_eal_mp_wait_lcore();

    // Shutdown 
   vtb::Logger::get_instance().shutdown();
   vtb::restore_echoctl();
   return 0;
}
