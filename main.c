/*
 * Author: fishi <fischer@devtail.com>
 * Date: May 2015
 *
 * The tcp-metrics tool - measure the round-trip time and bandwidth of a TCP connection. 
 * This file contains the main method and argument parser
 */
#include <argp.h>
#include <stdlib.h>
#include <regex.h>

#include "measure_tcp.h"

#define DEFAULT_TIMEOUT 2
#define DEFAULT_BUF_SIZE 1024*1024*2 // 2MB
#define DEFAULT_PORT 80
#define DEFAULT_SKIPS 10
#define DEFAULT_ROUNDS 1

// argp specifications
const char *argp_program_version = "tcp-metrics v0.1";
const char *argp_program_bug_address = "<fischer@devtail.com>";
static char doc[] = "tcp-metrics -- A tool to measure the round-trip time (rtt) and bandwidth (bw) of a TCP connection to a specified resource. Two different methods to measure the bandwidth are available.\nExample:\ttcp-metrics yourdomain.com /my/resource\n";
static char args_doc[] = "DOMAIN PATH";
static struct argp_option options[] = { 
	{ "port", 'p', "<Port>", 0, "The <Port> the resource to measure is available at - default is 80."},
    { "size", 's', "<Size>", 0, "<Size> of the receiver buffer - if not specified, by default a 2MB receiver buffer is used. Note that on each socket.read() the buffer is written from the start (it is just a dummy buffer...). Please note, that if this buffer is to small, the socket.read() operation does not have enough memory to store data to - thus a huge bottleneck in the application layer would occur."},
    { "skips", 'k', "<Skips>", 0, "Number of socket.read() <Skips> to avoid bursting and TCP slow-start to falsify the result - by default this value is 10. Note that a value of 0 here would be the same as using measurement option 2 (-c)."},
	{ "rounds", 'n', "<Rounds>", 0, "Number of repeated measurement <Rounds>. In order to get a fair estimate of the rtt, several rounds should be used. By default this value is 2"},
	{ "timeout", 't', "<Timeout>", 0, "The socket <Timeout> in seconds - by default 2s."},
	{ "multi", 'm', 0, 0, "If set, for each round a different TCP socket is used. That way you might get a better overall estimate about an expected path between the client and the resource. Please note, that you need at least two measurement rounds for this option, since otherwise you will only use one TCP socket anyways..."},
	{ "rtt", 'r', 0, 0, "Measure the rtt."},
	{ "bandwidth-method-A", 'b', 0, 0, "Measure the bandwidth with option 1, meaning the first socket.read() operations are skipped in order to avoid initial bursts and TCP slow-start to falsify the result."},
	{ "bandwidth-method-B", 'c', 0, 0, "Measure the bandwidth with option 2, meaning we measure beginning from the first socket.read() operation."},
	{ "all", 'a', 0, 0, "Measure everything - option1 + option2 + rtt."},
	{ "v", 'v', 0, 0, "Verbose - print subsequent estimates to stdout."},
    { 0 } 
};

struct arguments
{
	char *domain;
	char *resource;
	int port;
	int size;
	int skips;
	int rounds;
	int multi;
	int timeout;
	int rtt;
	int bwa;
	int bwb;
	int all;
	int verbose;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;

	switch(key)
	{
		case 'p': arguments->port = atoi(arg); break;
		case 's': arguments->size = atoi(arg); break;
		case 'k': arguments->skips = atoi(arg); break;
		case 'n': arguments->rounds = atoi(arg); break;
		case 't': arguments->timeout = atoi(arg); break;
		case 'm': arguments->multi = 1; break;
		case 'r': arguments->rtt = 1; break;
		case 'b': arguments->bwa = 1; break;
		case 'c': arguments->bwb = 1; break;
		case 'a': arguments->all = 1; break;
		case 'v': arguments->verbose = 1; break;

		case ARGP_KEY_ARG:
			if(state->arg_num > 2)
			{
				argp_usage(state);
			}

			if(state->arg_num == 0) arguments->domain = arg;
			else arguments->resource = arg;

			break;
    
		case ARGP_KEY_FINI:
			if(state->arg_num != 2)
			{
				argp_usage(state);
			}

			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[])
{
	struct arguments arguments;

	// set defaults
	arguments.domain = "";
	arguments.resource = "";
	arguments.verbose = 0;
	arguments.timeout = DEFAULT_TIMEOUT;
	arguments.skips = DEFAULT_SKIPS;
	arguments.size = DEFAULT_BUF_SIZE; 
	arguments.rounds = DEFAULT_ROUNDS;
	arguments.port = DEFAULT_PORT;
	arguments.bwa = 0;
	arguments.bwb = 0;
	arguments.rtt = 0;
	arguments.all = 0;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	// measure all set
	if(arguments.all)
	{
		arguments.bwa = 1;
		arguments.bwb = 1;
		arguments.rtt = 1;
	}

	// default measurements
	if(arguments.bwa == 0 && arguments.bwb == 0 && arguments.rtt == 0)
	{
		arguments.bwa = 1;
		arguments.rtt = 1;
	}

	// create measurement object
	tcp_measurement msrmnt;

	msrmnt.skips = arguments.skips;
	msrmnt.buf_size = arguments.size;
	msrmnt.rounds = arguments.rounds;
	msrmnt.port = arguments.port;
	msrmnt.timeout = arguments.timeout;
	msrmnt.domain = arguments.domain;
	msrmnt.resource = arguments.resource;
	msrmnt.verbose = arguments.verbose;
	msrmnt.measure_bw_a = arguments.bwa;
	msrmnt.measure_bw_b = arguments.bwb;
	msrmnt.measure_rtt = arguments.rtt;	

	// measure
	measure_tcp_metrics(&msrmnt);

	// print result
	if(arguments.bwa) printf("Option 1 Goodput: %f MB/s\n",msrmnt.bw_a);
	if(arguments.bwb) printf("Option 2 Goodput: %f MB/s\n",msrmnt.bw_b);
	if(arguments.rtt) printf("rtt: %d ms\n",(int)(msrmnt.rtt*1000));

	return 0;
}
