#include "garpd.h"


int get_iface_details(garpd_settings_t *garpd_settings) {

	struct ifreq ifr;

	int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (sd <= 0) {
		close(sd);
		printf("Error: Unable to open RAW Socket Descriptor. Check User Permissions.\n");
		exit(2);
	}

	strcpy(ifr.ifr_name, garpd_settings->interface_name);

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
	printf("MAC Address is: %02x:%02x:%02x:%02x:%02x:%02x",
			garpd_settings->garpd_iface->iface_mac[0],
			garpd_settings->garpd_iface->iface_mac[1],
			garpd_settings->garpd_iface->iface_mac[2],
			garpd_settings->garpd_iface->iface_mac[3],
			garpd_settings->garpd_iface->iface_mac[4],
			garpd_settings->garpd_iface->iface_mac[5]);
	fflush(stdout);
	
	// Bind our socket to our interface
	struct sockaddr_ll socket_address;
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	socket_address.sll_ifindex = garpd_settings->garpd_iface->iface_index;

	if ( bind(sd, (struct sockaddr*)&socket_address, sizeof(socket_address)) == -1 ) {
		close(sd);
		printf("Error: Unable to bind socket to interface %s\n", garpd_settings->interface_name);
		exit(2);
	}

	// Save our socket descriptor integer and move on:
	garpd_settings->sd = sd;
	return 0;
}

int send_arp_request(garpd_settings_t *garpd_settings) {

	uint8_t buffer[128];
	memset(buffer, 0x69, sizeof(buffer));

	// Our ethernet header struct

	struct sockaddr_ll mysockaddr;
	mysockaddr.sll_family = AF_PACKET;
	mysockaddr.sll_ifindex = garpd_settings->garpd_iface->iface_index;
	mysockaddr.sll_protocol = htons(ETH_P_ARP);
	mysockaddr.sll_hatype = htons(ARPHRD_ETHER);
	mysockaddr.sll_pkttype = (PACKET_BROADCAST);
	mysockaddr.sll_halen = MAC_LENGTH;
	memset(mysockaddr.sll_addr, 0x00, sizeof(mysockaddr.sll_addr));

	// FORMAT ETHERNET HEADER:
	// Cast our buffer of bytes as a header:
	struct ethhdr *ethernet_header = (struct ethhdr *) buffer;
	
	// Set Destination MAC:
	memset(ethernet_header->h_dest, 0xFF, MAC_LENGTH);

	// Set Source MAC + Socket Address:
	memcpy(ethernet_header->h_source, garpd_settings->garpd_iface->iface_mac, MAC_LENGTH);
	memcpy(mysockaddr.sll_addr, garpd_settings->garpd_iface->iface_mac, MAC_LENGTH);

	// Set our proto to ARP:
	ethernet_header->h_proto = htons(ETH_P_ARP);
	
	// FORMAT ARP HEADER:
	// Start formatting our arp header sizeof(struct ethhdr) into our range of bytes:
	struct arp_header_t *arp_header = (struct arp_header_t *) (buffer + sizeof(struct ethhdr));
	arp_header->hardware_type = htons(ARP_HTYPE_ETHER);
	arp_header->protocol_type = htons(ARP_PYTPE_IPV4);
	arp_header->hardware_len = MAC_LENGTH;
	arp_header->protocol_len = IPV4_LENGTH;
	arp_header->opcode = htons(ARP_OP_REPLY);

	memcpy(arp_header->sender_mac, garpd_settings->garpd_iface->iface_mac, MAC_LENGTH);
	memcpy(arp_header->sender_ip, &(garpd_settings->ip.sin_addr), IPV4_LENGTH);

	memcpy(arp_header->target_ip, &(garpd_settings->ip.sin_addr), IPV4_LENGTH);
	memset(arp_header->target_mac, 0xFF, MAC_LENGTH);

	if ( sendto(garpd_settings->sd, buffer, (sizeof(struct ethhdr) + sizeof(struct arp_header_t)), 0, (struct sockaddr *) &mysockaddr, sizeof(mysockaddr)) == -1 ){
		close(garpd_settings->sd);
		printf("Error: Unable to send on socket\n");
		exit(2);
	}

	return 0;
}

int convert_ip_address(garpd_settings_t *garpd_settings) {

	if (inet_pton(AF_INET, garpd_settings->ip_start, &(garpd_settings->ip.sin_addr)) != 1) {
		close(garpd_settings->sd);
		printf("Error: Unable to convert IP address to binary format\n");
		exit(2);
	}
	return 0;
}

// Main loop:
int garpd_do(garpd_settings_t *garpd_settings) {

	// Retrieve interface details:
	if (get_iface_details(garpd_settings) != 0) {
		return 1;
	}

	if (convert_ip_address(garpd_settings) != 0) {
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
