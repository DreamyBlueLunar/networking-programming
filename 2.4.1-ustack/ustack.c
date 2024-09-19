#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#include <unistd.h>

#define NUM_MBUFFERS		2048
#define BURST_LEN			128

int dpdk_port_id = 0;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN }
};

int main(int argc, char *argv[]) {
	if (0 > rte_eal_init(argc, argv)) {
		rte_exit(EXIT_FAILURE, "error with EAL init\n");
	}

	uint16_t nb_sys_ports = rte_eth_dev_count_avail();
	if (0 == nb_sys_ports) {
		rte_exit(EXIT_FAILURE, "No avaliable eth devices\n");
	}

	printf("nb_sys_ports: %d\n", nb_sys_ports); // 1

	struct rte_mempool *mbuf_pool = 
		rte_pktmbuf_pool_create("mbufpool", NUM_MBUFFERS, 0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (NULL == mbuf_pool) {
		rte_exit(EXIT_FAILURE, "failed to create mbuf_pool\n");
	}

	struct rte_eth_dev_info dev_info;
	rte_eth_dev_info_get(dpdk_port_id, &dev_info);

	const int num_rx_queues = 1;
	const int num_tx_queues = 0;

	if (0 > rte_eth_dev_configure(dpdk_port_id, num_rx_queues, num_tx_queues, 
									&port_conf_default)) {
		rte_exit(EXIT_FAILURE, "failed to config devices\n");
	}

	if (0 > rte_eth_rx_queue_setup(dpdk_port_id, 0, 128, rte_eth_dev_socket_id(dpdk_port_id), 
							NULL, mbuf_pool)) {
		rte_exit(EXIT_FAILURE, "failed to setup rx queue\n");
	}

	if (0 > rte_eth_dev_start(dpdk_port_id)) {
		rte_exit(EXIT_FAILURE, "failed to start devices\n");
	}

	printf("dev start success\n");

	while (1) {
		struct rte_mbuf *mbufs[BURST_LEN];
		int nb_rcvd = rte_eth_rx_burst(dpdk_port_id, 0,  mbufs, BURST_LEN);
		if (nb_rcvd > BURST_LEN) {
			rte_exit(EXIT_FAILURE, "error with rte_eth_rx_burst\n");
		}

/*
+------------+---------------+-------------------+--------------+
|   ethhdr   |     iphdr     |   udphdr/tcphdr   |   payload    |  
+------------+---------------+-------------------+--------------+
*/

		for (int i = 0; i < nb_rcvd; i ++) {
			struct rte_ether_hdr *ehdr =  rte_pktmbuf_mtod(mbufs[i], struct rte_ether_hdr *);
			if (ehdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
				printf("not ethernet data\n");
				continue;
			}

			struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(mbufs[i], struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
			if (iphdr->next_proto_id == IPPROTO_UDP) {

				struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);

				uint16_t length = ntohs(udphdr->dgram_len) - sizeof(struct rte_udp_hdr);

				printf("length: %d, content: %s\n", length, (char *)(udphdr + 1));
			}
		}

	}

	return 0;
}

