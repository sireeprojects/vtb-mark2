#include "signals.h"

#include "messenger.h"
#include "common.h"

#include <csignal>
#include <execinfo.h>
#include <rte_pause.h>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace vtb {

void signal_handler([[maybe_unused]] int signal) {
    vtb::Logger::get_instance().direct_append(
          "[VTBMAIN] Signal caught. Starting shutdown...\n");
    
    // Stop the producers first so no new logs are generated
    keep_running = false;

    // shutdown Logger
    vtb::Logger::get_instance().shutdown();
    std::exit(0); // 0 - pretend normal exit of program
}

int keep_alive_thread(void*) {
   while (!keep_running) {
      rte_pause();
   }
   return 0;
}

void setup_signal_handler() {
   std::set_terminate(vtb::graceful_exit);
   std::signal(SIGINT,  signal_handler);  // detect ctrl+c
   std::signal(SIGTERM, signal_handler); // terminate from os
   std::signal(SIGSEGV, signal_handler_crash); // Segfault
   std::signal(SIGABRT, signal_handler); // Abort (rte_panic)
   std::signal(SIGFPE,  signal_handler); // Math error
}                                         

[[maybe_unused]] void signal_handler_crash(int sig) {
   // Disable handlers to prevent loops
   std::signal(SIGSEGV, SIG_DFL);
   std::signal(SIGABRT, SIG_DFL);

   std::stringstream ss;
   ss << "\n" << std::string(50, '=') << "\n";
   ss << "CRASH SIGNAL: " << sig << (sig == 6 ? " (SIGABRT/Terminate)" : " (SIGSEGV)") << "\n";

   void* array[20];
   size_t size = backtrace(array, 20);
   char** symbols = backtrace_symbols(array, size);

   for (size_t i = 0; i < size; i++) {
      std::string line(symbols[i]);
      std::string name = line;

      // Robust parsing: extract text between '(' and '+' OR after last ' '
      size_t begin = line.find_first_of('(');
      size_t end = line.find_first_of('+', begin);

      if (begin != std::string::npos && end != std::string::npos) {
         std::string mangled = line.substr(begin + 1, end - begin - 1);
         int status;
         char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
         if (status == 0) {
            name = demangled;
            free(demangled);
         } else {
            name = mangled;
         }
      }

      ss << "  [" << i << "] " << name << "\n";
   }
   free(symbols);
   ss << std::string(50, '=') << "\n";

   // Log and Flush
   vtb::Logger::get_instance().direct_append(ss.str());
   vtb::Logger::get_instance().emergency_flush();

   // To stop 'make' from printing "Aborted (core dumped)"
   // we exit with 0. Note: This prevents core file generation!
   // If you want a core dump, use std::raise(sig) instead of _exit(0).
   fprintf(stderr, "\n[VTB] Emergency shutdown complete. Exiting.\n");
   _exit(0);
}

}
