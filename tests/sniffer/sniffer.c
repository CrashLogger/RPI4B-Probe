/*
   sniffer.c

   Example packet sniffer using the libpcap packet capture library available
   from http://www.tcpdump.org.
  
   ------------------------------------------

   Copyright (c) 2012 Vic Hargrave

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pcap/pcap.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

//1us galtzen da kaptura egiten gutxi gora behera, beste 1us galtzen da gauzak kopiatu eta mugitzen, beraz, T2 prozesatzean hori parkatuko dugu
#define T2SIMTIME_US 16

//Oinarrizko ML egoera, minutu bateko interbaloetara borobilduta
#define BASIC_T31SIMTIME_S 60
#define BASIC_T32SIMTIME_US 1162000

//ML egoera aurreratua, minutu bateko interbaloetara borobilduta
#define ADVANCED_T31SIMTIME_S 60
#define ADVANCED_T32SIMTIME_US 2350000


enum { NS_PER_SECOND = 1000000000 };

pcap_t* handle;
int linkhdrlen;
int packets;
char timestamp[255];

//For measuring CPU Usage
struct timespec cpuStart, cpuFinish, cpuDelta;

void T2Sim(uint32_t usecKop){
  struct timespec start, check;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  while(1){
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &check);
    if(((uint64_t)check.tv_nsec - (uint64_t)start.tv_nsec) > usecKop * 1000){
      return;
    }
    else{
      //Ezer ez dugu egiten
    }
  }
}

void MLSim(int signo)
{
  //T2Sim-ek mikrosegundoak nahi ditu.
  printf("ML simulatzen!\n");
  T2Sim(BASIC_T32SIMTIME_US);
  printf("ML simulatuta!\n");
  alarm(BASIC_T31SIMTIME_S);
  
  return;
}

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0)
    {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0)
    {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}

char* parseTimeStamp(struct timeval tv){

    
    uint8_t hours = (tv.tv_sec/3600);
    uint8_t minutes = (tv.tv_sec -(3600*hours))/60;
    uint8_t seconds = (tv.tv_sec -(3600*hours)-(minutes*60));
  
    //+10: Ordu-zonaldea konpentsatzeko
    hours = ((hours + 10) > 24) ? (hours+10)-24 : hours;
  
  sprintf(timestamp, "%02d:%02d:%02d:%06ld", hours, minutes, seconds, tv.tv_usec);
  return timestamp;

}

struct timeval getTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv;
}

pcap_t* create_pcap_handle(char* device, char* filter)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = NULL;
    pcap_if_t* devices = NULL;
    struct bpf_program bpf;
    bpf_u_int32 netmask;
    bpf_u_int32 srcip;

    // If no network interface (device) is specfied, get the first one.
    if (!*device) {
    	if (pcap_findalldevs(&devices, errbuf)) {
            fprintf(stderr, "pcap_findalldevs(): %s\n", errbuf);
            return NULL;
        }
        strcpy(device, devices[0].name);
    }

    // Get network device source IP address and netmask.
    if (pcap_lookupnet(device, &srcip, &netmask, errbuf) == PCAP_ERROR) {
        fprintf(stderr, "pcap_lookupnet: %s\n", errbuf);
        return NULL;
    }

    // Open the device for live capture.
    handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "pcap_open_live(): %s\n", errbuf);
        return NULL;
    }

    // Convert the packet filter epxression into a packet filter binary.
    if (pcap_compile(handle, &bpf, filter, 1, netmask) == PCAP_ERROR) {
        fprintf(stderr, "pcap_compile(): %s\n", pcap_geterr(handle));
        return NULL;
    }

    // Bind the packet filter to the libpcap handle.    
    if (pcap_setfilter(handle, &bpf) == PCAP_ERROR) {
        fprintf(stderr, "pcap_setfilter(): %s\n", pcap_geterr(handle));
        return NULL;
    }

    return handle;
}

void get_link_header_len(pcap_t* handle)
{
    int linktype;
 
    // Determine the datalink layer type.
    if ((linktype = pcap_datalink(handle)) == PCAP_ERROR) {
        printf("pcap_datalink(): %s\n", pcap_geterr(handle));
        return;
    }
 
    // Set the datalink layer header size.
    switch (linktype)
    {
    case DLT_NULL:
        linkhdrlen = 4;
        break;
 
    case DLT_EN10MB:
        linkhdrlen = 14;
        break;
 
    case DLT_SLIP:
    case DLT_PPP:
        linkhdrlen = 24;
        break;
 
    default:
        printf("Unsupported datalink (%d)\n", linktype);
        linkhdrlen = 0;
    }
}

void packet_handler(u_char *user, const struct pcap_pkthdr *packethdr, const u_char *packetptr)
{
    struct ip* iphdr;
    struct icmp* icmphdr;
    struct tcphdr* tcphdr;
    struct udphdr* udphdr;
    char iphdrInfo[256];
    char srcip[256];
    char dstip[256];
    
    // Skip the datalink layer header and get the IP header fields.
    packetptr += linkhdrlen;
    iphdr = (struct ip*)packetptr;
    strcpy(srcip, inet_ntoa(iphdr->ip_src));
    strcpy(dstip, inet_ntoa(iphdr->ip_dst));
    sprintf(iphdrInfo, "ID:%d TOS:0x%x, TTL:%d IpLen:%d DgLen:%d",
            ntohs(iphdr->ip_id), iphdr->ip_tos, iphdr->ip_ttl,
            4*iphdr->ip_hl, ntohs(iphdr->ip_len));
 
    // Advance to the transport layer header then parse and display
    // the fields based on the type of hearder: tcp, udp or icmp.
    packetptr += 4*iphdr->ip_hl;
    
    switch (iphdr->ip_p)
    {
    case IPPROTO_TCP:
        tcphdr = (struct tcphdr*)packetptr;
        /*
        printf("%s\n", parseTimeStamp(getTimeStamp()));
        printf("TCP  %s:%d -> %s:%d\n", srcip, ntohs(tcphdr->th_sport),
               dstip, ntohs(tcphdr->th_dport));
        printf("%s\n", iphdrInfo);
        printf("%c%c%c%c%c%c Seq: 0x%x Ack: 0x%x Win: 0x%x TcpLen: %d\n",
               (tcphdr->th_flags & TH_URG ? 'U' : '*'),
               (tcphdr->th_flags & TH_ACK ? 'A' : '*'),
               (tcphdr->th_flags & TH_PUSH ? 'P' : '*'),
               (tcphdr->th_flags & TH_RST ? 'R' : '*'),
               (tcphdr->th_flags & TH_SYN ? 'S' : '*'),
               (tcphdr->th_flags & TH_SYN ? 'F' : '*'),
               ntohl(tcphdr->th_seq), ntohl(tcphdr->th_ack),
               ntohs(tcphdr->th_win), 4*tcphdr->th_off);
        printf("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n\n");
        */
        packets += 1;
        break;
 
    case IPPROTO_UDP:
        udphdr = (struct udphdr*)packetptr;
        /*
        printf("%s\n", parseTimeStamp(getTimeStamp()));
        printf("UDP  %s:%d -> %s:%d\n", srcip, ntohs(udphdr->uh_sport),
               dstip, ntohs(udphdr->uh_dport));
        printf("%s\n", iphdrInfo);
	    printf("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n\n");
        */
        packets += 1;
        break;
 
    case IPPROTO_ICMP:
        icmphdr = (struct icmp*)packetptr;
        /*
        printf("%s\n", parseTimeStamp(getTimeStamp()));
        printf("ICMP %s -> %s\n", srcip, dstip);
        printf("%s\n", iphdrInfo);
        printf("Type:%d Code:%d ID:%d Seq:%d\n", icmphdr->icmp_type, icmphdr->icmp_code,
               ntohs(icmphdr->icmp_hun.ih_idseq.icd_id), ntohs(icmphdr->icmp_hun.ih_idseq.icd_seq));
	    printf("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n\n");
        */
        packets += 1;
        break;
    }
    
    //Protokoloa identifikatu ostean, harrapatutako guztia pasa ahal zaio benetako T2 funtzioari
    //Kasu honetan ordea, simulatzailea dugu bakarrik.
    
    T2Sim(T2SIMTIME_US);
}

void stop_capture(int signo)
{
    struct pcap_stat stats;
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuFinish);
    sub_timespec(cpuStart, cpuFinish, &cpuDelta);
    
    printf("T1 + T2 CPU Time: %d.%.9ld\n", (int)cpuDelta.tv_sec, cpuDelta.tv_nsec);
    
    if (pcap_stats(handle, &stats) >= 0) {
        printf("\n%d packets captured\n", packets);
        printf("%d packets received by filter\n", stats.ps_recv); 
        printf("%d packets dropped\n\n", stats.ps_drop);
    }
    pcap_close(handle);
    exit(0);
}

int main(int argc, char *argv[])
{

    char device[256];
    char filter[256]; 
    int count = 0;
    int opt;

    *device = 0;
    *filter = 0;

    // Get the command line options, if any
    while ((opt = getopt(argc, argv, "hi:n:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("usage: %s [-h] [-i interface] [-n count] [BPF expression]\n", argv[0]);
            exit(0);
            break;
        case 'i':
            strcpy(device, optarg);
            break;
        case 'n':
            count = atoi(optarg);
            break;
        }
    }

    // Get the packet capture filter expression, if any.
    for (int i = optind; i < argc; i++) {
        strcat(filter, argv[i]);
        strcat(filter, " ");
    }

    signal(SIGINT, stop_capture);
    signal(SIGTERM, stop_capture);
    signal(SIGQUIT, stop_capture);
    signal(SIGALRM, MLSim);
    
    // Create packet capture handle.
    handle = create_pcap_handle(device, filter);
    if (handle == NULL) {
        return -1;
    }

    // Get the type of link layer.
    get_link_header_len(handle);
    if (linkhdrlen == 0) {
        return -1;
    }

    //T1
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuStart);

    alarm(BASIC_T31SIMTIME_S);

    // Start the packet capture with a set count or continually if the count is 0.
    if (pcap_loop(handle, count, packet_handler, (u_char*)NULL) == PCAP_ERROR) {
        fprintf(stderr, "pcap_loop failed: %s\n", pcap_geterr(handle));
        return -1;
    }

    stop_capture(0);
}
