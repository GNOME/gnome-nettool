#include	"gnome-ping-helper.h"

char sendbuf[BUFSIZE];

int datalen = 56;	/* data that goes with ICMP echo request */
int nsent;			/* add 1 for each sendto() */
pid_t pid;			/* our PID */
int sockfd;
struct proto *pr;
int verbose = 0;

int
main (int argc, char **argv)
{
	int c;
	int count = 0;
	char *host;

	for (;;) {
		int option_index = 0;
		static struct option long_options[] = {
			{"count", 1, 0, 'c'},
			{"verbose", 0, 0, 'v'},
			{"help", 0, 0, 'h'}
		};

		opterr = 0;     /* don't want getopt() writing to stderr */
		c = getopt_long (argc, argv, "c:vh",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verbose = 1;
			break;

		case 'c':
			count = atoi (optarg);
			if (count <= 0) {
				printf ("%s: Bad number of packets to transmit.\n", PROGRAM);
				exit (-1);
			}
			break;

		case 'h':
			printf (USAGE1, PROGRAM);
			printf ("  -h,  --help\t\tPrint this help.\n");
			printf ("  -c,  --count=NUMBER\tSet limit of packets to send\n");
			printf ("  -v,  --verbose\tShow the packets sent\n");
			break;

		default:
			printf (USAGE1, PROGRAM);
			printf (USAGE2);
			exit (-1);
		}
	}

	if (optind < argc) {
		if (argc - optind != 1) {
			printf (USAGE1, PROGRAM);
			printf (USAGE2);
			exit (-1);
		} else {
			host = argv[optind];
			do_ping (host, count);
		}
	} else {
			printf (USAGE1, PROGRAM);
			printf (USAGE2);
			exit (-1);
	}

	return (0);
}

void
do_ping (const char *host, int count)
{
	struct hostent *hp;
	struct sockaddr_in addr;

	pid = getpid ();
	signal (SIGALRM, sig_alrm);

	if ((hp = gethostbyname (host)) == NULL) {
		perror ("Host unknown");
		exit (3);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (INADDR_ANY);
	addr.sin_port = htons (1);
	memcpy (&addr.sin_addr, hp->h_addr, hp->h_length);

	printf ("PING %s (%s): %d data bytes\n", hp->h_name,
		inet_ntoa (addr.sin_addr), datalen);

	pr = malloc (sizeof (struct proto));
		
	pr->sasend = (struct sockaddr *) &(addr);
	pr->sarecv = calloc (1, sizeof (addr));
	pr->salen = sizeof (addr);
	pr->icmpproto = IPPROTO_ICMP;

	readloop (count);

	free (pr);
	
	exit (0);
}

void
sig_alrm (int signo)
{
	send_icmp ();

	alarm (1);
	return;			/* probably interrupts recvfrom() */
}

/* Main loop that send/receive ICMP ECHO packets */
void
readloop (int count)
{
	int size;
	char recvbuf[BUFSIZE];
	socklen_t len;
	ssize_t n;
	struct timeval tval;
	int i = 0;

	sockfd = socket (pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
	setuid (getuid ());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof (size));

	sig_alrm (SIGALRM);	/* send first packet */

	for (;;) {
		len = pr->salen;
		n = recvfrom (sockfd, recvbuf, sizeof (recvbuf), 0,
			      pr->sarecv, &len);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
				i++;
			} else {
				perror ("recvfrom error");
			}
		}

		gettimeofday (&tval, NULL);
		process_icmp (recvbuf, n, &tval);

		if (count > 0) {
			i++;
			if (i == count) {
				alarm (0);
				break;
			}
		}
	}
}

void
process_icmp (char *ptr, ssize_t len, struct timeval *tvrecv)
{
	int hlen1, icmplen;
	double rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	struct sockaddr_in *sin = NULL;

	ip = (struct ip *) ptr;	/* start of IP header */
	hlen1 = ip->ip_hl << 2;	/* length of IP header */

	icmp = (struct icmp *) (ptr + hlen1);	/* start of ICMP header */
	if ((icmplen = len - hlen1) < 8) {
		fprintf (stderr, "icmplen (%d) < 8", icmplen);
		exit (-3);
	}

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)
			return;	/* not a response to our ECHO_REQUEST */
		if (icmplen < 16) {
			fprintf (stderr, "icmplen (%d) < 16", icmplen);
			exit (-3);
		}

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub (tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		sin = (struct sockaddr_in *) pr->sarecv;

		printf ("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
			icmplen, inet_ntoa (sin->sin_addr),
			icmp->icmp_seq, ip->ip_ttl, rtt);
	}
}

void
send_icmp (void)
{
	int len;
	struct icmp *icmp;
	struct sockaddr_in *sin = NULL;

	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = nsent++;
	gettimeofday ((struct timeval *) icmp->icmp_data, NULL);

	len = 8 + datalen;	/* checksum ICMP header and data */
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);

	sendto (sockfd, sendbuf, len, 0, pr->sasend, pr->salen);

	sin = (struct sockaddr_in *) pr->sasend;

	if (verbose) {
		printf ("%d bytes sent to %s: seq=%u, id=%d\n",
			len, inet_ntoa (sin->sin_addr),
			icmp->icmp_seq, icmp->icmp_id);
	}
}

void
tv_sub (struct timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0) {	/* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

unsigned short
in_cksum (unsigned short *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	/* 4mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}

	/* 4add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);	/* add carry */
	answer = ~sum;		/* truncate to 16 bits */
	return (answer);
}
