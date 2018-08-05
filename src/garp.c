#include "garp.h"

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

// Struct to hold our interface details:
typedef struct garp_iface_t {
	int iface_index;
	unsigned char iface_mac[MAC_LENGTH];
	unsigned char macstring[MAC_LENGTH + 4];
} garp_iface_t;

// Struct for our utility's settings:
typedef struct garp_settings_t {
	const char *interface_name;
	const char *ip_start;
	garp_iface_t *garp_iface;
} garp_settings_t;

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

// Bail:
int garp_exit(void) {
	printf("Error, Exiting\n");
	exit(1);
}

void dump_mac(garp_settings_t *garp_settings) {

	//int y = 0;
	//for (int i = 0; i < MAC_LENGTH; i++) {
		//garp_settings->garp_iface->macstring[i + y]  = garp_settings->garp_iface->iface_mac[i];
		//garp_
	//}
}

int get_iface_details(garp_settings_t *garp_settings) {

	/*
	man 7 netdevice:
	 struct ifreq {
		       char ifr_name[IFNAMSIZ];
		       union {
			   struct sockaddr ifr_addr;
			   struct sockaddr ifr_dstaddr;
			   struct sockaddr ifr_broadaddr;
			   struct sockaddr ifr_netmask;
			   struct sockaddr ifr_hwaddr;
			   short           ifr_flags;
			   int             ifr_ifindex;
			   int             ifr_metric;
			   int             ifr_mtu;
			   struct ifmap    ifr_map;
			   char            ifr_slave[IFNAMSIZ];
			   char            ifr_newname[IFNAMSIZ];
			   char           *ifr_data;
		       };
		   };
	*/
	struct ifreq ifr;

	// Socket descriptor:
	/*
	AF_PACKET = 
	Packet  sockets  are  used to receive or send raw packets at the device driver (OSI Layer 2) level.  
	They allow the user to implement protocol modules in user space on top of the physical layer.  
	The socket_type is either SOCK_RAW for raw packets including the link-level header or SOCK_DGRAM for cooked packets with the link-level header removed.  
	The link-level header  information  is  available  in  a  common  format  in  a  sockaddr_ll  structure.   
	protocol  is the IEEE 802.3 protocol number in network byte order.  

	See the <linux/if_ether.h> include file for a list of allowed protocols.  When protocol is set to htons(ETH_P_ALL), then all protocols are received. 	

	*/
	int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (sd <= 0) {
		close(sd);
		garp_exit();
	}

	strcpy(ifr.ifr_name, garp_settings->interface_name);

	// todo writeup:
	if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
		close(sd);
		garp_exit();
	}

	garp_settings->garp_iface->iface_index = ifr.ifr_ifindex;
	printf("Interface %s Index is %d\n", garp_settings->interface_name, garp_settings->garp_iface->iface_index);

	// Get MAC Address of the Interface
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
		close(sd);
		garp_exit();
	}

	memcpy(garp_settings->garp_iface->iface_mac, ifr.ifr_hwaddr.sa_data, MAC_LENGTH);
	//printf("Interface %d has MAC %s\n", garp_settings->garp_iface->iface_index, dump_mac(garp_settings));

	return 0;
}

// Main loop:
int garp_do(garp_settings_t *garp_settings) {

	// Get interface details:
	if (! get_iface_details(garp_settings)) {
		return 0;
	}

	return 1;

}
int main(int argc, const char **argv) {

	// Init our structs:
	garp_settings_t *garp_settings = (garp_settings_t*)malloc(sizeof(garp_settings_t));
	memset(garp_settings, 0, sizeof(garp_settings_t));

	garp_settings->garp_iface = (garp_iface_t*)malloc(sizeof(garp_iface_t));
	memset(garp_settings->garp_iface, 0, sizeof(garp_iface_t));

	// Check for arg count:
	if (argc != 3) {
		garp_exit();
	}

	// Populate settings & go:
	garp_settings->interface_name = argv[1];
	garp_settings->ip_start = argv[2];
	if (garp_do(garp_settings)) {
		return 0;
	} else {
		return 2;
	}
}
