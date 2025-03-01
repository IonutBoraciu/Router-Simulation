// Boraciu Ionut-Sorin 325CA
#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lib.h>
#include "protocols.h"

// global variables accounting for the routing table and mac table
struct route_table_entry *rtable;
int rtable_len;

struct arp_table_entry *arp_table;
int mac_table_len;

#define ETHERTYPE_IP 0x0800
#define ARP_ETHER_TYPE 0x0806
#define MAX_LEN 100000
#define NOT_FOUND NULL
#define WRONG_PACKET 7
#define GOOD_PACKET 6
#define START_ICMP (sizeof(struct ether_header)+sizeof(struct iphdr))

// Binary search function to search in nlogn time for the longest match in the
// routing table
int binary_search (uint32_t value) {

	int left = 0, right = rtable_len-1;
	int poz = -1;
	int compare_value = value & rtable->mask;
    while (left <=right) {
		long int m =  (left + right)/2;
		// check if a possible match was found
		if (rtable[m].prefix == compare_value) {
			if (poz != -1) {
				// if we already found a match, check if the new one is better
				// ( is a longer match than the previous one)
				if(ntohl (rtable[m].mask) > ntohl(rtable[poz].mask)) {
					poz = m;
				}
			} else {
				poz = m;
			}
		}
		if (ntohl (rtable[m].prefix) < ntohl(value)) {
			right = m - 1;
		} else {
			left = m + 1;
		}
	}
	return poz;
}

// compute the best route in nlogn time with a binary search, and return its address if given or
// null if not found
struct route_table_entry *get_best_route (uint32_t ip_dest) {

	int best_route_poz = binary_search(ip_dest);
	if (best_route_poz != -1) {
		return &rtable[best_route_poz];
	}
	return NULL;

}

// find the mac address coresponding to a given ip in n time
// or return NULL if not found
struct arp_table_entry *get_mac_entry (uint32_t given_ip) {

    for (int i = 0; i < mac_table_len; i++) {
		if (given_ip == arp_table[i].ip)
			return &arp_table[i];
	}

	return NULL;
}

// compute the control sum of the ip_header
int checkSum (struct iphdr *ip_hdr) {
	// to ensure the correct computation of the control sum, we will first save
	// the old checksum and then reset it to 0 before computing the new one.
	uint16_t old_checksum = ip_hdr->check;
	ip_hdr->check = 0;
	uint16_t check_sum = checksum ((uint16_t *)ip_hdr, sizeof(struct iphdr));
	ip_hdr->check = old_checksum;
	// check if the packet was corrupted on the way to the router.
	if (check_sum != ntohs (ip_hdr->check)) {
		return WRONG_PACKET;
	}
	return GOOD_PACKET;	
}

// comparing function used for qsort
int comparator (const void* p1, const void* p2) {
	// convert the input to 2 routing table entries
	struct route_table_entry *first_entry = (struct route_table_entry*) p1;
	struct route_table_entry *second_entry = (struct route_table_entry*) p2;

	// first we sort in desdending order by the prefix
	if (ntohl (first_entry->prefix) < ntohl (second_entry->prefix)) {
		return 1;
	// secondly if the we have 2 equal prefixes we will compare the masks in the same descending order
	} else if (ntohl (first_entry->prefix) == ntohl (second_entry->prefix)) {
		if(ntohl (first_entry->mask) < ntohl (second_entry->mask)) {
			return 1;
		}
	}
	return -1;
}

// Helper function used the set the mac in the ethernet level.
void set_ethernet_level(struct ether_header *eth_hdr,unsigned char *source_mac,unsigned char *dest_mac) {
	memcpy(eth_hdr->ether_dhost, dest_mac, 6);
	memcpy(eth_hdr->ether_shost, source_mac, 6*sizeof(unsigned char));
}

// sends an icmp message of either type 0 ( echo reply), 11 (out of time) or 3 (host unreachable)
void icmp_message (char *packet, size_t packet_len, int interface, int type,int code) {

	// get the ethernet and ip header from the packet
	struct ether_header *eth_hdr = (struct ether_header *) packet;
	struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));

	// if the packet is of either type 11 or 3 we will have to make space for the icmp header
	// by moving the first 64 bytes of the data into over the icmp header
	if (type !=0) {
		char *buffer = malloc(64);
		DIE(buffer == NULL, "buffer allocation failed");

		memcpy(buffer, packet + START_ICMP, 64);
		memcpy(packet + START_ICMP + sizeof(struct icmphdr), buffer, 64);
		// we will also adjust the sizes of our packet with the new added icmp header
		ip_hdr->tot_len +=  htons(sizeof(struct icmphdr));
		packet_len += sizeof(struct icmphdr);
		free(buffer);
	}

	// Set the ethernet variables, by changing the destination with the source
	// and the source with the mac of the router.
	unsigned char *mac = malloc(6*sizeof(unsigned char));
	DIE(mac == NULL, "mac_icmp allocation failed");

	get_interface_mac(interface, mac);
	set_ethernet_level(eth_hdr, mac,eth_hdr->ether_shost);
	free(mac);

	// change the source and destination as the mac address
	ip_hdr->daddr = ip_hdr->saddr;
	ip_hdr->saddr = inet_addr(get_interface_ip(interface));

	// also change the protocol to icmp, so that the router will know
	// how to handle the given packet
	ip_hdr->protocol = IPPROTO_ICMP;

	// set the type and code based on the given input of the function
	struct icmphdr *icmp_hdr = (struct icmphdr *)(packet +START_ICMP);
	icmp_hdr->type = type;
	icmp_hdr->code = code;

	// send the packet BACK to the original sourse.
	send_to_link(interface, packet, packet_len);

}
int main(int argc, char *argv[])
{

	int interface;
	char packet[MAX_PACKET_LEN];
	size_t packet_len;

	init(argc - 2, argv + 2);

	// allocate the routing and arp table
	rtable = malloc(MAX_LEN * sizeof(struct route_table_entry));
	DIE(rtable == NULL, "rtable allocation failed");

	arp_table = malloc(MAX_LEN * sizeof(struct  arp_table_entry));
	DIE(arp_table == NULL, "arp_table allocation failed");

	// populate the two tables
	rtable_len = read_rtable(argv[1], rtable);
	mac_table_len = parse_arp_table("arp_table.txt", arp_table);

	// sort the table with a custom comparator in descending order
	qsort(rtable,rtable_len, sizeof(struct route_table_entry), comparator);

	while (1) {
		// wait to receive a package
		interface = recv_from_any_link(packet, &packet_len);
		DIE(interface < 0, "how did this happen?");

		// get the 3 needed headers from the packet
		struct ether_header *eth_hdr = (struct ether_header *) packet;
		struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = (struct icmphdr *)(packet +START_ICMP);

		// check if the package experied as to avoid creating loops
		// for a package that may never reach its destination
		if (ip_hdr->ttl == 1 || ip_hdr->ttl == 0) {
			printf("Out of time\n");
			// send an icmp message alerting the source that the package expired
			icmp_message(packet, packet_len, interface, 11, 0);
			continue;
		}
		
		// check that the pakage sent is ipv4
		if (eth_hdr->ether_type != ntohs(ETHERTYPE_IP)) {
			printf("Not ipv4\n");
			continue;
		}

		// make sure that WE didnt receive a corrupted package
		if (checkSum(ip_hdr) == WRONG_PACKET) {
			printf("Received a corrupted packet\n");
			continue;
		}

		// check if a received packet is of type echo request to the ROUTER
		// ( meaning that a host wants to ping the router)
		if (ip_hdr->protocol == IPPROTO_ICMP) {
			if (icmp_hdr->type == 8 && icmp_hdr->code == 0 && ip_hdr->daddr == inet_addr(get_interface_ip(interface))) {
				printf("Mi-a dat, mi-a dat pachet\n");
				icmp_message(packet, packet_len, interface, 0, 0);
				continue;
			}
		}

		// decrease the time to live, and recompute the control sum with the new ttl
		uint16_t old_checksum = ip_hdr->check;
		int old_ttl = ip_hdr->ttl;
		ip_hdr->ttl -= 1;
		ip_hdr->check = ~(~old_checksum +  ~((uint16_t)old_ttl) + (uint16_t)ip_hdr->ttl) - 1;

		// check if a route has been found
		struct route_table_entry *best_router = get_best_route(ip_hdr->daddr);
		if (best_router == NOT_FOUND) {
			// otherwise send an icmp message alerting the sourse that the destination is unreachable
			printf("Not found best route\n");
			icmp_message(packet,packet_len, interface, 3, 0);
			continue;
		}

		// check if there is a mac address given for the destination
		struct arp_table_entry *dest_mac_entry = get_mac_entry(ip_hdr->daddr);
		if (dest_mac_entry == NOT_FOUND) {
			printf("Not found mac entry\n");
			// otherwise send an icmp message
			icmp_message(packet, packet_len, interface, 3, 1);
            continue;
			
		}

		unsigned char *dest_mac = malloc(6*sizeof(char));
		DIE(dest_mac == NULL, "dest_mac allocation failed");
	
		unsigned char *mac = malloc(6*sizeof(char));
		DIE(mac == NULL, "mac allocation failed");

		// find the mac address of the interface
		get_interface_mac(best_router->interface, mac);

		// set the ethernet destination and source address
		set_ethernet_level(eth_hdr,mac, dest_mac_entry->mac);

		// free the data
		free(dest_mac);
		free(mac);

		// finally send the package to the destination
		send_to_link(best_router->interface, packet, packet_len);
	}

	// free the tables
	free(rtable);
	free(arp_table);

	// end the programme
	return 0;
}