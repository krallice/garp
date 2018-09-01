// Standard Includes:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

// Network Specific:
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>

// General defines:
#define MAC_LENGTH 6
#define IPV4_LENGTH 4

// Ethernet Ethertype for ARP:
#define ETHERTYPE_ARP 0x0806

// ARP specific fields for Ethernet/IPv4:
#define ARP_HTYPE_ETHER 0x01
#define ARP_PYTPE_IPV4 0x0800
#define ARP_OP_REQUEST 0x01
#define ARP_OP_REPLY 0x02

// GARPD Specific Settings:
#define GARPD_DEFAULT_REPLY_INTERVAL 1

// Struct to hold our interface details:
typedef struct garpd_iface_t {
	int iface_index;
	unsigned char iface_mac[MAC_LENGTH];
	//unsigned char macstring[MAC_LENGTH + 4];
} garpd_iface_t;

// Struct for our utility's arguments:
typedef struct garpd_settings_t {
	const char *interface_name;
	const char *ip_start;
	struct sockaddr_in ip;
	int reply_interval;
	int sd;
	garpd_iface_t *garpd_iface;
} garpd_settings_t;

// ARP Header:
typedef struct arp_header_t {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t opcode;
    uint8_t sender_mac[MAC_LENGTH];
    uint8_t sender_ip[IPV4_LENGTH];
    uint8_t target_mac[MAC_LENGTH];
    uint8_t target_ip[IPV4_LENGTH];
} arp_header_t;
