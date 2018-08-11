#include "garpd.h"

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

#define GARPD_DEFAULT_REPLY_INTERVAL 1

// Struct to hold our interface details:
typedef struct garpd_iface_t {
	int iface_index;
	unsigned char iface_mac[MAC_LENGTH];
	unsigned char macstring[MAC_LENGTH + 4];
} garpd_iface_t;

// Struct for our utility's arguments:
typedef struct garpd_settings_t {
	const char *interface_name;
	const char *ip_start;
	int reply_interval;
	int sd;
	garpd_iface_t *garpd_iface;
} garpd_settings_t;

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

int get_iface_details(garpd_settings_t *garpd_settings) {

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
		printf("Error: Unable to open RAW Socket Descriptor. Check User Permissions.\n");
		exit(2);
	}

	strcpy(ifr.ifr_name, garpd_settings->interface_name);

	// todo writeup:
	// Return our interface index based on our interface_name string:
	if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
		close(sd);
		printf("Error: Unable to find interface named %s.\n", garpd_settings->interface_name);
		exit(2);
	}

	garpd_settings->garpd_iface->iface_index = ifr.ifr_ifindex;
	printf("Interface %s Index is %d\n", garpd_settings->interface_name, garpd_settings->garpd_iface->iface_index);

	// Get MAC Address of the Interface
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
		close(sd);
		printf("Error: Unable to get MAC Address for Interface %s.\n", garpd_settings->interface_name);
		exit(2);
	}

	memcpy(garpd_settings->garpd_iface->iface_mac, ifr.ifr_hwaddr.sa_data, MAC_LENGTH);
	//printf("Interface %d has MAC %s\n", garpd_settings->garpd_iface->iface_index, dump_mac(garpd_settings));
	
	// Save our socket descriptor integer and move on:
	garpd_settings->sd = sd;
	return 0;
}

int send_arp_request(garpd_settings_t *garpd_settings) {

	uint8_t buffer[128];
	memset(buffer, 0, sizeof(buffer));

	struct sockaddr_ll sockaddr;
	sockaddr.sll_family = AF_PACKET;
	sockaddr.sll_ifindex = garpd_settings->garpd_iface->iface_index;
	sockaddr.sll_protocol = htons(ETH_P_ARP);
	sockaddr.sll_hatype = htons(ARPHDR_ETHER);

	return 0;
}

// Main loop:
int garpd_do(garpd_settings_t *garpd_settings) {

	// Retrieve interface details:
	if (get_iface_details(garpd_settings) != 0) {
		return 1;
	}

	// Main Loop:
	do {
		send_arp_request(garpd_settings);
	} while (sleep(garpd_settings->reply_interval) == 0);

	// If we end up here, something has gone wrong and bomb out:
	return 1;
}

garpd_settings_t *init_garpd_settings() {

	garpd_settings_t *garpd_settings = (garpd_settings_t*)malloc(sizeof(garpd_settings_t));
	memset(garpd_settings, 0, sizeof(garpd_settings_t));

	garpd_settings->garpd_iface = (garpd_iface_t*)malloc(sizeof(garpd_iface_t));
	memset(garpd_settings->garpd_iface, 0, sizeof(garpd_iface_t));

	return garpd_settings;

}
int main(int argc, const char **argv) {

	// Init our structs:
	garpd_settings_t *garpd_settings = init_garpd_settings();

	// Check for arg count:
	if (argc != 3) {
		printf("Error: Incorrect Arguments Passed.\n");
		exit(1);
	}

	// Populate settings & go:
	garpd_settings->interface_name = argv[1];
	garpd_settings->ip_start = argv[2];
	garpd_settings->reply_interval = GARPD_DEFAULT_REPLY_INTERVAL;

	garpd_do(garpd_settings);

	// Control should only enter here if the main loop has bombed out
	return 1;
}
