#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>	/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/wait.h>

#include	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>

#include	<getopt.h>

#define PROGRAM "gnome-ping-helper"
#define USAGE1 "Usage: %s [-c count] [-v] host\n\n"
#define USAGE2 "Try 'gnome-ping-helper --help' for more information\n"

#define	BUFSIZE		1500

void do_ping (const char *host, int count);
void process_icmp (char *, ssize_t, struct timeval *);
void send_icmp (void);
void readloop (int count);
void sig_alrm (int);
void tv_sub (struct timeval *, struct timeval *);
unsigned short in_cksum (unsigned short *addr, int len);

struct proto {
	struct sockaddr *sasend;	/* sockaddr{} for send, from getaddrinfo */
	struct sockaddr *sarecv;	/* sockaddr{} for receiving */
	socklen_t salen;	/* length of sockaddr{}s */
	int icmpproto;		/* IPPROTO_xxx value for ICMP */
};
