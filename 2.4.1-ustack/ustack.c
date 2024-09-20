#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#include <unistd.h>

#define NUM_MBUFFERS		2048
#define BURST_LEN			128
#define ENABLE_SEND 		1

#if ENABLE_SEND

uint8_t src_MAC[RTE_ETHER_ADDR_LEN];
uint8_t des_MAC[RTE_ETHER_ADDR_LEN];

uint32_t src_IP, des_IP;

uint16_t src_port, des_port;

#endif

int dpdk_port_id = 0;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN }
};

#if ENABLE_SEND

static int ustack_encode_udp_pkt(uint8_t *msg, char *data, uint16_t length) {
	// 1. ethernet
	struct rte_ether_hdr *ehdr = (struct rte_ether_hdr *)msg;
	rte_memcpy(ehdr->d_addr.addr_bytes, des_MAC, RTE_ETHER_ADDR_LEN);
	rte_memcpy(ehdr->s_addr.addr_bytes, src_MAC, RTE_ETHER_ADDR_LEN);
	ehdr->ether_type = htons(RTE_ETHER_TYPE_IPV4);

	// 2. IP
	struct rte_ipv4_hdr *iphdr = (struct rte_ipv4_hdr *)(ehdr + 1);
	
	iphdr->version_ihl = 0x45; 
	iphdr->type_of_service = 0;
	iphdr->fragment_offset = 0;
	iphdr->next_proto_id = IPPROTO_UDP;
	iphdr->packet_id = 0;
	iphdr->time_to_live = 64;
	iphdr->total_length = htons(length - sizeof(struct rte_ether_hdr));
	iphdr->dst_addr = des_IP;
	iphdr->src_addr = src_IP;
	
	iphdr->hdr_checksum = 0;
	iphdr->hdr_checksum = rte_ipv4_cksum(iphdr);	

	// 3. UDP
	struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);
	udphdr->dst_port = des_port;
	udphdr->src_port = src_port;

	uint16_t udplen = length - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr);
	udphdr->dgram_len = htons(udplen);

	rte_memcpy((uint8_t*)(udphdr + 1), data, udplen - sizeof(struct rte_udp_hdr));

	udphdr->dgram_cksum = 0;
	udphdr->dgram_cksum = rte_ipv4_udptcp_cksum(iphdr, udphdr);

	return length;
}


static struct rte_mbuf *ustack_send(struct rte_mempool *mbuf_pool, char *msg, uint16_t msg_len) {
	// total_length = msg_len + 14 + 20 + 8
	int total_length = msg_len + sizeof(struct rte_ether_hdr) + 
			sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr);

	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	if (!mbuf) {
		rte_exit(EXIT_FAILURE, "error with creating pktmbuf\n");
	}

	// like sk_buf
	mbuf->pkt_len = total_length;
	mbuf->data_len = total_length;

	uint8_t *pktdata = rte_pktmbuf_mtod(mbuf, uint8_t*);
	
	ustack_encode_udp_pkt(pktdata, msg, total_length);
	
	return mbuf;
}


#endif

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
	const int num_tx_queues = ENABLE_SEND ? 1 : 0;
	struct rte_eth_conf port_conf = port_conf_default;

	if (0 > rte_eth_dev_configure(dpdk_port_id, num_rx_queues, num_tx_queues, 
									&port_conf_default)) {
		rte_exit(EXIT_FAILURE, "failed to config devices\n");
	}

	if (0 > rte_eth_rx_queue_setup(dpdk_port_id, 0, 1024, 
			rte_eth_dev_socket_id(dpdk_port_id), NULL, mbuf_pool)) {
		rte_exit(EXIT_FAILURE, "failed to setup rx queue\n");
	}

#if ENABLE_SEND

	struct rte_eth_txconf txq_conf = dev_info.default_txconf;
	txq_conf.offloads = port_conf.rxmode.offloads;

	if (0 > rte_eth_tx_queue_setup(dpdk_port_id, 0, 1024, 
			rte_eth_dev_socket_id(dpdk_port_id), &txq_conf)) {
		rte_exit(EXIT_FAILURE, "failed to setup tx queue\n");
	}

#endif

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
				// 过滤非以太网帧
				// printf("not ethernet data\n");
				continue;
			}

			struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(mbufs[i], struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
			if (iphdr->next_proto_id == IPPROTO_UDP) { // 走到这里才能确定这个包是可以解析的

				struct rte_udp_hdr *udphdr = (struct rte_udp_hdr *)(iphdr + 1);

#if ENABLE_SEND

				// rte_memcpy 这玩意儿自己就做了字节序转换，所以不论是给mac地址赋值还是给ip，port赋值
				// 都不用再次自己调用字节序转换函数
				rte_memcpy(src_MAC, ehdr->d_addr.addr_bytes, RTE_ETHER_ADDR_LEN);
				rte_memcpy(des_MAC, ehdr->s_addr.addr_bytes, RTE_ETHER_ADDR_LEN);
				rte_memcpy(&src_IP, &iphdr->dst_addr, sizeof(uint32_t));
				rte_memcpy(&des_IP, &iphdr->src_addr, sizeof(uint32_t));
				rte_memcpy(&src_port, &udphdr->dst_port, sizeof(uint16_t));
				rte_memcpy(&des_port, &udphdr->src_port, sizeof(uint16_t));

#endif

				uint16_t length = ntohs(udphdr->dgram_len) - sizeof(struct rte_udp_hdr);

				printf("--> recv: length: %d, content: %s\n", length, (char *)(udphdr + 1));
															
#if ENABLE_SEND // 如果我要send出去，就要用 tx_burst
				char *data = (char *)(udphdr + 1);
				struct rte_mbuf *tx_buf = ustack_send(mbuf_pool, data, length);											
				rte_eth_tx_burst(dpdk_port_id, 0, &tx_buf, 1);							
				printf("<-- send: length: %d, content: %s\n", length, (char *)(udphdr + 1));
				rte_pktmbuf_free(tx_buf);
#endif
				rte_pktmbuf_free(mbufs[i]);

			}
		}

	}

	return 0;
}

