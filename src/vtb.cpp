#include <rte_launch.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_kvargs.h>
#include <vector>

#include "messenger.h"
#include "common.h"
#include "signals.h"
#include "cmdline.h"
#include "config.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

/* 
 * port_id: The ID of the port that changed status
 * type:    The event type (RTE_ETH_EVENT_INTR_LSC)
 * param:   A pointer to any custom data you passed during registration
 * ret_param: Reserved for future use
 */
static int
vhost_lsc_callback(uint16_t port_id, enum rte_eth_event_type type, void *param, void *ret_param)
{
    struct rte_eth_link link;
    int ret;

    // We don't care about other events here
    if (type != RTE_ETH_EVENT_INTR_LSC)
        return 0;

    // Get the new link status
    ret = rte_eth_link_get_nowait(port_id, &link);
    if (ret < 0) {
        // printf("Port %u: Failed to get link status\n", port_id);
        VTB_LOG(ERROR) << "Port Failed to get link status: " << port_id;
        return ret;
    }

    if (link.link_status == RTE_ETH_LINK_UP) {
        // printf("\n[EVENT] Port %u: VM Connected (Link UP). Speed: %u Mbps\n", port_id, link.link_speed);
        VTB_LOG(INFO) << "Port Connected: " << port_id << " With linkspeed: " << link.link_speed;
        // You could set a global flag here to start polling this port
    } else {
        // printf("\n[EVENT] Port %u: VM Disconnected (Link DOWN)\n", port_id);
        VTB_LOG(INFO) << "Port Disconnected: " << port_id;
        // You could set a flag here to stop polling this port
    }

    return 0;
}

int initialize_dpdk_port(uint16_t port_id) {
    struct rte_eth_conf port_conf;
    struct rte_eth_dev_info dev_info;
    struct rte_mempool *mbuf_pool;
    char pool_name[RTE_MEMPOOL_NAMESIZE];
    int ret;

    // 1. Zero out the configuration structure to avoid -Wmissing-field-initializers
    memset(&port_conf, 0, sizeof(struct rte_eth_conf));
    port_conf.intr_conf.lsc = 1;

   // struct rte_eth_conf port_conf = {
   //    .intr_conf = {
   //       .lsc = 1, // Enable Link Status Change interrupts
   //    },
   // };

    // 2. Query device capabilities and current configuration (queues)
    ret = rte_eth_dev_info_get(port_id, &dev_info);
    if (ret != 0) {
        VTB_LOG(ERROR) << "Error getting info for port " << port_id << ": " << rte_strerror(-ret);
        return ret;
    }

    // Use the queue counts specified during vdev creation/attachment
    uint16_t nb_rx_q = dev_info.max_rx_queues;
    uint16_t nb_tx_q = dev_info.max_tx_queues;

    VTB_LOG(INFO) << "Initializing Port " << port_id << " [" << dev_info.driver_name 
                  << "] with RX=" << nb_rx_q << ", TX=" << nb_tx_q;

    // 3. Create a unique MBUF Pool for this specific port
    snprintf(pool_name, sizeof(pool_name), "VTB_POOL_P%u", port_id);
    
    // Scale mbuf count by queue count to ensure enough descriptors for high throughput
    uint32_t total_mbufs = NUM_MBUFS * std::max((uint16_t)1, nb_rx_q);
    
    mbuf_pool = rte_pktmbuf_pool_create(pool_name, total_mbufs,
                                        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_eth_dev_socket_id(port_id));
    if (mbuf_pool == NULL) {
        VTB_LOG(ERROR) << "Mempool creation failed for port " << port_id << ": " << rte_strerror(rte_errno);
        return -rte_errno;
    }

    // 4. Configure Multi-Queue / RSS if more than one queue is used
    if (nb_rx_q > 1) {
        port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
        // Apply only RSS hash functions supported by the hardware/driver
        port_conf.rx_adv_conf.rss_conf.rss_hf = (RTE_ETH_RSS_IP | RTE_ETH_RSS_TCP | RTE_ETH_RSS_UDP) 
                                                & dev_info.flow_type_rss_offloads;
    }

    // 5. Finalize Configuration
    ret = rte_eth_dev_configure(port_id, nb_rx_q, nb_tx_q, &port_conf);
    if (ret < 0) {
        VTB_LOG(ERROR) << "Port configuration failed (port " << port_id << "): " << rte_strerror(-ret);
        return ret;
    }

    // 6. Setup RX Queues
    for (uint16_t q = 0; q < nb_rx_q; q++) {
        ret = rte_eth_rx_queue_setup(port_id, q, RX_RING_SIZE,
                                     rte_eth_dev_socket_id(port_id),
                                     NULL, mbuf_pool);
        if (ret < 0) {
            VTB_LOG(ERROR) << "RX-Q[" << q << "] setup failed for port " << port_id << ": " << rte_strerror(-ret);
            return ret;
        }
    }

    // 7. Setup TX Queues
    for (uint16_t q = 0; q < nb_tx_q; q++) {
        ret = rte_eth_tx_queue_setup(port_id, q, TX_RING_SIZE,
                                     rte_eth_dev_socket_id(port_id),
                                     NULL);
        if (ret < 0) {
            VTB_LOG(ERROR) << "TX-Q[" << q << "] setup failed for port " << port_id << ": " << rte_strerror(-ret);
            return ret;
        }
    }

    // Register the LSC callback for this specific port
    rte_eth_dev_callback_register(port_id, RTE_ETH_EVENT_INTR_LSC, vhost_lsc_callback, NULL);

    // 8. Start the Device
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        VTB_LOG(ERROR) << "Failed to start port " << port_id << ": " << rte_strerror(-ret);
        return ret;
    }

    // 9. Post-start configuration
    rte_eth_promiscuous_enable(port_id);

    VTB_LOG(INFO) << "Port " << port_id << " fully initialized and started.";
    return 0;
}


// Callback to capture the string value from kvargs
static int get_string_cb([[maybe_unused]] const char *key, const char *value, void *opaque) {
    char **str_ptr = (char **)opaque;
    if (value) *str_ptr = strdup(value);
    return 0;
}

std::string get_vhost_path_by_name(const char *target_name) {
    struct rte_devargs *da;
    char *socket_path = nullptr;
    std::string final_path = "";

    RTE_EAL_DEVARGS_FOREACH("vdev", da) {
        if (strcmp(da->name, target_name) == 0) {
            struct rte_kvargs *kvlist = rte_kvargs_parse(da->args, NULL);
            if (kvlist) {
                rte_kvargs_process(kvlist, "iface", &get_string_cb, &socket_path);
                rte_kvargs_free(kvlist);
            }
            break; 
        }
    }

    if (socket_path) {
        final_path = socket_path; 
        free(socket_path);        
    }

    return final_path;
}

int shutdown_dpdk_port(uint16_t port_id) {
    char pool_name[RTE_MEMPOOL_NAMESIZE];
    struct rte_mempool *mbuf_pool = nullptr;
    int ret;

    VTB_LOG(INFO) << "Shutting down Port " << port_id << "...";

    // 1. Stop the device
    // This stops the RX/TX engines and disables the link.
    ret = rte_eth_dev_stop(port_id);
    if (ret != 0) {
        VTB_LOG(ERROR) << "Failed to stop port " << port_id << ": " << rte_strerror(-ret);
    }

    // 2. Close the device
    // This releases the hardware/virtual resources (like vhost-user sockets).
    ret = rte_eth_dev_close(port_id);
    if (ret != 0) {
        VTB_LOG(ERROR) << "Error closing port " << port_id << ": " << rte_strerror(-ret);
    }

    // 3. Locate and Free the MBUF Pool
    // Mempools persist in hugepage memory even if the port is closed.
    // We must manually find and free it to prevent memory leaks during restarts.
    snprintf(pool_name, sizeof(pool_name), "VTB_POOL_P%u", port_id);
    mbuf_pool = rte_mempool_lookup(pool_name);

    if (mbuf_pool != nullptr) {
        VTB_LOG(INFO) << "Releasing mempool: " << pool_name;
        rte_mempool_free(mbuf_pool);
    } else {
        VTB_LOG(WARNING) << "Could not find mempool " << pool_name << " during shutdown";
    }

    VTB_LOG(INFO) << "Port " << port_id << " shutdown sequence complete.";
    return 0;
}

void process_vdevs() {
   VTB_LOG(INFO) << "Processing vdevs from commandline";
   unsigned int nb_ports = rte_eth_dev_count_avail();
   VTB_LOG(INFO) << "Number of ports identified: " << nb_ports;

   struct rte_eth_dev_info dev_info;

   uint16_t port_id;
   RTE_ETH_FOREACH_DEV(port_id) {
      int ret = rte_eth_dev_info_get(port_id, &dev_info);
      if (ret != 0) {
         VTB_LOG(ERROR) << "Error during dev_info get for port: " << port_id;
      }
      // The number of queues from your command line is stored here:
      uint16_t nb_rx_q = dev_info.max_rx_queues;
      uint16_t nb_tx_q = dev_info.max_tx_queues;

      VTB_LOG(INFO) << "Port: " << port_id << "  was configured with " << nb_rx_q << " RX queues and " << nb_tx_q << " TX queues via command line";
   }

   RTE_ETH_FOREACH_DEV(port_id) {
      char devname[RTE_ETH_NAME_MAX_LEN];
      int ret = rte_eth_dev_get_name_by_port(port_id, devname);
      if (ret == 0) {
         VTB_LOG(INFO) << "Port: " << port_id << " Name: " << devname << " path: " << get_vhost_path_by_name(devname);
         // debug_find_vhost_path(name);
      }
   }

   RTE_ETH_FOREACH_DEV(port_id) {
      initialize_dpdk_port(port_id);
   }


   // RTE_ETH_FOREACH_DEV(port_id) {
   //    shutdown_dpdk_port(port_id);
   // }
}

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

   process_vdevs();

   unsigned int keepalive_lcore = rte_get_next_lcore(rte_get_main_lcore(), true, 0);
   VTB_LOG(TRACE) << "VTB App Keep-Alive thread started on: " << keepalive_lcore;
   rte_eal_remote_launch(vtb::keep_alive_thread, NULL, keepalive_lcore);
   rte_eal_wait_lcore(keepalive_lcore);

   // Controlled shutdown 
   vtb::Logger::get_instance().shutdown();
   vtb::restore_echoctl();
   return 0;
}
