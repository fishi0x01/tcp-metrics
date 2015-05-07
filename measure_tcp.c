/*
 * Author: fishi <fischer@devtail.com>
 * Date: May 2015
 *
 * Bandwidth (more precisely goodput) and rtt measurements are done here. Goodput is measured via harmonic mean. 
 * rtt is measured via weighed moving average. Goodput can be measured in two ways:
 * 1.: by using the first socket.read() timestamp as the start time.
 * 2.: by using a later socket.read() timestamp as the start time (to avoid bursts and slow-start falsifying the estimate).
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "measure_tcp.h"

double timeval_subtract(struct timeval *x, struct timeval *y)  
{  
	double diff = x->tv_sec - y->tv_sec;  
	diff += (x->tv_usec - y->tv_usec)/1000000.0;  

	return diff;  
}

/* measure bandwidth (with harmonic mean)
	cur_ts - start_ts define the total time interval. 
	bytes is the number of bytes read with the last socket.read() call */
double measure_bw(struct timeval *start_ts, struct timeval *cur_ts, float bytes, int option, tcp_measurement *msrmnt)
{
	msrmnt->total_bytes[option] += bytes;

	// calculate current measurement
	double ts_diff = timeval_subtract(cur_ts,start_ts);
	double cur_bw = (msrmnt->total_bytes[option]/(1024*1024))/ts_diff;

	if(!msrmnt->n[option])
	{
		// first measurement
		msrmnt->bandwidth[option] = cur_bw;
	}
	else
	{
		// harmonic mean
		msrmnt->bandwidth[option] = (msrmnt->n[option]+1)/((msrmnt->n[option]/msrmnt->bandwidth[option])+(1/cur_bw));
	}

	msrmnt->n[option]++;

	if(msrmnt->verbose) printf("Goodput Option %d: %f MB/s\n",option+1,msrmnt->bandwidth[option]);

	return msrmnt->bandwidth[option];
}

/* measure rtt (with weighed moving average).
	cur_ts - start_ts is the time between request sent and first response socket.read() */
double measure_rtt(struct timeval *start_ts, struct timeval *cur_ts, tcp_measurement *msrmnt)
{
	double cur_rtt = timeval_subtract(cur_ts,start_ts);

	if(msrmnt->rtt < 0)
	{
		// first measurement
		msrmnt->rtt = cur_rtt;
	}
	else
	{
		// weighed moving average
		msrmnt->rtt = 0.8*msrmnt->rtt + 0.2*cur_rtt;
	}

	if(msrmnt->verbose) printf("Last rtt estimate: %d ms\n",(int)(msrmnt->rtt*1000));

	return msrmnt->rtt;
}

void make_request(int sock, char *buf, char *resource, char *domain, int buf_size)
{
	// 5. prepare and send request
	bzero(buf,buf_size);
	sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", resource, domain);

	if(send(sock, buf, strlen(buf), 0) < 0)
	{
		perror("Error while sending request");
		return;
	}
}

int create_tcp_connection(tcp_measurement *msrmnt)
{
	// create socket
	int sock;
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error : Could not create socket\n");
		return -1;
	}

	// find server
	struct hostent *server;
	server = gethostbyname(msrmnt->domain);
	if(server == NULL)
	{
		perror("Could not find server\n");
		return -1;
	}

	// create address
	struct sockaddr_in serveraddr;
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(msrmnt->port);
	
	// connect to server
	if(connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("Could not connect to server\n");
		return -1;
	}

	return sock;
}

void measure_tcp_metrics(tcp_measurement *msrmnt)
{
	// set initial metric values
	msrmnt->rtt = -1;
	msrmnt->bandwidth[0] = -1;
	msrmnt->bandwidth[1] = -1;
	msrmnt->total_bytes[0] = 0;
	msrmnt->total_bytes[1] = 0;
	msrmnt->n[0] = 0;
	msrmnt->n[1] = 0;

	// prepare timestamps
	struct timeval start_ts,first_pack_ts,n_pack_ts,cur_ts;
	start_ts.tv_sec = 0;
	start_ts.tv_usec = 0;
	n_pack_ts.tv_sec = 0;
	n_pack_ts.tv_usec = 0;
	first_pack_ts.tv_sec = 0;
	first_pack_ts.tv_usec = 0;
	cur_ts.tv_sec = 0;
	cur_ts.tv_usec = 0;

	int sock;

	if(!msrmnt->multi)
	{
		// create a TCP connection
		sock = create_tcp_connection(msrmnt);
	}
	
	// metrics
	int bytes;
	double bw_a = 0; // option 1
	double bw_b = 0; // option 2
	double rtl = 0;

	// buffer
	int req_buf_size = 1024*4; // 4KB
	char *req_buf = malloc(req_buf_size); // the request buffer
	char *buf = malloc(msrmnt->buf_size);

	int round_cnt = 1; // round counter for verbose

	while(msrmnt->rounds-- > 0)
	{
		if(msrmnt->verbose) printf("Round %d\n",round_cnt++);

		if(msrmnt->multi)
		{
			// create a TCP connection
			sock = create_tcp_connection(msrmnt);
		}

		// prepare objects for blocking IO
		fd_set set;
		struct timeval timeout;
		timeout.tv_sec = msrmnt->timeout;
		timeout.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(sock,&set);

		// make request
		gettimeofday(&start_ts,NULL);
		make_request(sock,req_buf,msrmnt->resource,msrmnt->domain,req_buf_size);
		
		// flags 
		int got_nth_packet = 0; // n-th socket.read() of the response
		int first_packet = 1;

		// receive response and measure metrics
		while(select(FD_SETSIZE,&set,NULL,NULL,&timeout))
		{
			// refresh timeout
			timeout.tv_sec = msrmnt->timeout;

			gettimeofday(&cur_ts,NULL);
			bytes = read(sock,buf,msrmnt->buf_size);
			
			// OPTION 1: we take the time of the n-th read() as the start time - to handle initial bursts and TCP slow-start
			if(msrmnt->measure_bw_a)
			{
				if(msrmnt->skips > 0)
				{
					msrmnt->skips--;
				}
				else
				{
					if(!got_nth_packet)
					{
						gettimeofday(&n_pack_ts,NULL);
						got_nth_packet = 1;
					}
					else
					{
						bw_a = measure_bw(&n_pack_ts,&cur_ts,bytes,0,msrmnt);
					}
				}
			}

			// very first socket.read()
			if(first_packet)
			{
				// RTT
				if(msrmnt->measure_rtt)
				{
					rtl = measure_rtt(&start_ts,&cur_ts,msrmnt);
				}

				gettimeofday(&first_pack_ts,NULL);
				first_packet = 0;
			}
			else if(msrmnt->measure_bw_b)
			{
				// OPTION 2: we take the request sent timestamp as the start time
				bw_b = measure_bw(&first_pack_ts,&cur_ts,bytes,1,msrmnt);
			}
		}

		if(msrmnt->multi)
		{
			// close socket
			close(sock);
		}
	}

	if(!msrmnt->multi)
	{
		// close socket
		close(sock);
	}

	// free buffers
	free(buf);
	free(req_buf);

	// set results
	msrmnt->bw_a = bw_a;
	msrmnt->bw_b = bw_b;
	msrmnt->rtt = rtl;
}
