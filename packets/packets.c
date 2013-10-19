#include <pcap/pcap.h>

int main(int argc, char *argv[]) {

	char pcap_buff[PCAP_ERRBUF_SIZE];       /* Error buffer used by pcap functions */
	pcap_t *pcap_handle = NULL;             /* Handle for PCAP library */
	struct pcap_pkthdr *packet_hdr = NULL;  /* Packet header from PCAP */
	const u_char *packet_data = NULL;       /* Packet data from PCAP */
	int ret = 0;                            /* Return value from library calls */
	char *trace_file = NULL;                /* Trace file to process */
	char *dev_name = NULL;                  /* Device name for live capture */
	char use_file = 0;                      /* Flag to use file or live capture */

	/* Check command line arguments */
	if( argc > 2 ) {
		fprintf(stderr, "Usage: %s trace_file\n", argv[0]);
		return -1;
	}
	else if( argc > 1 ){
		use_file = 1;
		trace_file = argv[1];
	}
	else {
		use_file = 0;
	}

	/* Open the trace file, if appropriate */
	if( use_file ){
		pcap_handle = pcap_open_offline(trace_file, pcap_buff);
		if( pcap_handle == NULL ){
			fprintf(stderr, "Error opening trace file \"%s\": %s\n", trace_file, pcap_buff);
			return -1;
		}
		printf("Processing file '%s'\n", trace_file);
	}
	/* Lookup and open the default device if trace file not used */
	else{
		dev_name = pcap_lookupdev(pcap_buff);
		if( dev_name == NULL ){
			fprintf(stderr, "Error finding default capture device: %s\n", pcap_buff);
			return -1;
		}
		pcap_handle = pcap_open_live(dev_name, BUFSIZ, 1, 0, pcap_buff);
		if( pcap_handle == NULL ){
			fprintf(stderr, "Error opening capture device %s: %s\n", dev_name, pcap_buff);
			return -1;
		}
		printf("Capturing on interface '%s'\n", dev_name);
	}

	/* Loop through all the packets in the trace file.
	 * ret will equal -2 when the trace file ends.
	 * This is an infinite loop for live captures. */
	ret = pcap_next_ex(pcap_handle, &packet_hdr, &packet_data);
	while( ret != -2 ) {

		/* An error occurred */
		if( ret == -1 ) {
			pcap_perror(pcap_handle, "Error processing packet:");
			pcap_close(pcap_handle);
			return -1;
		}

		/* Unexpected return values; other values shouldn't happen when reading trace files */
		else if( ret != 1 ) {
			fprintf(stderr, "Unexpected return value (%i) from pcap_next_ex()\n", ret);
			pcap_close(pcap_handle);
			return -1;
		}

		/* Process the packet and print results */
		else {
			/*
			 *
			 * Put your code here
			 *
			 */

			/* Examples:
			 * Print the first byte of the packet
			 * printf("%02X", packet_data[0]);

			 * Print the fifth byte of the packet
			 * printf("%02X", packet_data[4]);
			 */

		}

		/* Get the next packet */
		ret = pcap_next_ex(pcap_handle, &packet_hdr, &packet_data);
	}

	/* Close the trace file or device */
	pcap_close(pcap_handle);
	return 0;
}
