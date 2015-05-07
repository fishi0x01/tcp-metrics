/*
 * Author: fishi <fischer@devtail.com>
 * Date: May 2015
 *
 * Interface for the core functionality of the tcp-metrics measurement tool. 
 */

#ifndef __FISHI_TCP_MEASUREMENT__
#define __FISHI_TCP_MEASUREMENT__

/*
 * Structure that holds all the settings and measurement results
 */
typedef struct tcp_measurement
{
	// estimates
	double bw_a; // bandwidth (in MB/s) estimate for measurement option 1 (using skips)
	double bw_b; // bandwidth (in MB/s) estimate for measurement option 2 (no skips)
	double rtt;  // rtt (in s) estimate

	// helpers
	double bandwidth[2];   // in MB/s {option1, option2}
	float total_bytes[2];  // number of total bytes read {option1, option2}
	int n[2];              // number socket.read() operations {option1, option2}

	// measurement definition
	int skips;         // skips to avoid bursts and slow-start
	int buf_size;      // size of the receiver buffer
	int rounds;        // number of repeated measurement rounds
	char *domain;      // domain where the resource is reachable
	char *resource;    // path to resource on server
	int port;          // port through which the resource is reachable
	int timeout;       // the socket timeout in s
	int verbose;       // measure verbose (print subsequent estimates)
	int multi;         // determines whether to use several TCP sockets
	int measure_bw_a;  // measure bandwidth with option 1
	int measure_bw_b;  // measure bandwidth with option 2
	int measure_rtt;   // measure rtt

} tcp_measurement;

/*
 * This function measures the bandwidth and rtt using the settings in the given argument. 
 * Results are stored back into the given argument.
 */
void measure_tcp_metrics(tcp_measurement *msrmnt);

#endif
