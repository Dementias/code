#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define CLIENT_NAME "127.0.0.1"
#define CLIENT_PORT 4444
#define DEST_NAME "127.0.0.1"
#define DEST_PORT 5555
#define BUFLEN 8192


struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

unsigned short CheckSum (uint16_t * addr, int len)
{
    int nleft = len;
    unsigned int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    /* Our algorithm is simple, using a 32 bit accumulator (sum), we add
    * sequential 16 bit words to it, and at the end, fold back all the
    * carry bits from the top 16 bits into the lower 16 bits.
    */

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(unsigned char *) (&answer) = * (unsigned char *) w;
        sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add high 16 to low 16 */
    sum += (sum >> 16); /* add carry */
    answer = (unsigned short) ~sum; /* truncate to 16 bits */
    return (answer);
}

int CreateRawSocket()
{
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    if (sockfd == -1)
    {
        perror("Failed to create raw socket");
        exit(1);
    }

    return sockfd;
}
void SetSockaddr(struct sockaddr_in *addr, const char * _name, unsigned short _port)
{
    bzero((char *) addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(_name);
    addr->sin_port = htons(_port);
}

bool GetHostInfo(char * host_name, char * ip)
{
    gethostname(host_name, sizeof(host_name));

    struct hostent* host = gethostbyname(host_name);
    const char* ret = inet_ntop(host->h_addrtype, host->h_addr_list[0], ip, sizeof(ip));

    if (NULL==ret)
        return false;

    return true;
}

void FillIPheader(struct iphdr *iph, const char * data, char * datagram,
        unsigned int packet_id, const struct sockaddr_in *saddr_in, const char * source_ip)
{
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
    iph->id = htonl(packet_id);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;
    iph->saddr = inet_addr (source_ip);
    iph->daddr = saddr_in->sin_addr.s_addr;
    iph->check = CheckSum((unsigned short *) datagram, iph->tot_len);
}

void FillUDPheader(struct udphdr *udph, char * data,
        const struct sockaddr_in *saddr_in, const char * source_ip, unsigned short my_port, unsigned short d_port)
{
    udph->source = htons(my_port);
    udph->dest = htons(d_port);
    udph->len = htons(8 + strlen(data));
    udph->check = 0;

    //CheckSum
    struct pseudo_header psh;
    psh.source_address = inet_addr( source_ip );
    psh.dest_address = saddr_in->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));

    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);

    char * pseudogram = (char *)malloc(psize);
    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));

    udph->check = CheckSum( (unsigned short*) pseudogram , psize);
}

void SendMessage(int sockfd, struct iphdr *iph, struct udphdr *udph, char * data, char * datagram,
                 unsigned int packet_id, const struct sockaddr_in *saddr_in, const char * source_ip,
                         unsigned short my_port)
{
    // Fill in the IP header
    FillIPheader(iph, data, datagram, packet_id, saddr_in, source_ip);

    // Fill in the UDP header
    FillUDPheader(udph, data, saddr_in, source_ip, my_port, DEST_PORT);

    int n = sendto (sockfd, datagram, iph->tot_len,  0, (struct sockaddr *) saddr_in, sizeof (*saddr_in));

    if (n < 0)
    {
        perror("sendto() failed");
        exit(3);
    }

    else
        std::cout << "I sent server this: " << data << std::endl << "Packet length: " << n << std::endl;
}

void ReceiveMessage(int sockfd, char *buffer, char *recv_data, char * data, struct sockaddr_in *raddr_in,
        struct udphdr *udp_hdr, struct iphdr *ip_hdr)
{
    unsigned int len = sizeof(*raddr_in);
    int nbytes = recvfrom(sockfd, buffer, (sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data) + 1),
            MSG_WAITALL, (struct sockaddr *)raddr_in, &len);
    if (nbytes == -1)
    {
        perror("Unable to recv message");
        exit(6);
    }

    ip_hdr = (struct iphdr *)buffer;
    udp_hdr = (struct udphdr *)(buffer + sizeof (struct iphdr));
    recv_data = (char * )(buffer + sizeof (struct iphdr) + sizeof (struct udphdr));

    std::cout << "I got this message from server: " << recv_data << std::endl;
}

void SetSockOp(int sockfd)
{
    int one = 1;
    const int *val = &one;

    if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
    {
        perror("setsockopt() error");
        exit(2);
    }
}

void RunClient()
{
    int sockfd = CreateRawSocket();

    char datagram[BUFLEN];
    char *data;

    memset(datagram, 0, BUFLEN);

    // IP header
    struct iphdr *iph = (struct iphdr *) datagram;

    //UDP header
    struct udphdr *udph = (struct udphdr *) (datagram + sizeof(struct ip));

    struct sockaddr_in saddr_in, cliaddr_in, raddr_in;

    // Data
    data = datagram + sizeof (struct iphdr) + sizeof(struct udphdr);
    strcpy(data, "Hello from client");

    SetSockaddr(&saddr_in, DEST_NAME, DEST_PORT);
    SetSockaddr(&cliaddr_in, CLIENT_NAME, CLIENT_PORT);
    socklen_t len = sizeof(cliaddr_in);

    if (bind(sockfd, (struct sockaddr *)&cliaddr_in, len) < 0)
    {
        perror("Bind failed");
        exit(4);
    }

    SetSockOp(sockfd);

    unsigned int s = sizeof(cliaddr_in);
    getsockname(sockfd, (struct sockaddr *) &cliaddr_in, &s);
    std::cout << cliaddr_in.sin_addr.s_addr << std::endl;
    std::cout << ntohs(cliaddr_in.sin_port) << std::endl;
    unsigned short my_port = ntohs(cliaddr_in.sin_port);
    char source_ip[32] = CLIENT_NAME;
    unsigned int packet_id = 0;

    SendMessage(sockfd, iph, udph, data, datagram, packet_id, &saddr_in, source_ip, my_port);

    char buffer[BUFLEN];
    char *recv_data;

    struct iphdr r_iph;
    struct udphdr r_udph;

    ReceiveMessage(sockfd, buffer, recv_data, data, &raddr_in, &r_udph, &r_iph);

    close(sockfd);
}





int main()
{
    RunClient();
}
