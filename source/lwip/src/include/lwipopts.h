#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

#define MEM_LIBC_MALLOC                 0
#define MEM_ALIGNMENT                   8
#define MEM_SIZE                        (64 * 1024)
#define PBUF_POOL_SIZE                  32
#define PBUF_POOL_BUFSIZE               1536

#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_IPV4                       1
#define LWIP_ICMP                       1
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_DHCP                       1

#define TCP_MSS                         1460
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_SND_BUF                     (4 * TCP_MSS)

#define SYS_LIGHTWEIGHT_PROT            1

#define LWIP_DNS                        1
#define DNS_TABLE_SIZE                  4
#define DNS_MAX_NAME_LENGTH             256

#define LWIP_DHCP_GET_NTP_SRV           0
#define LWIP_DHCP_MAX_DNS_SERVERS       2

#define LWIP_RAW 1

#endif