#include <iostream>
#include <pcap.h>
#include <cstring>
#include <netinet/ether.h>
extern "C" {
    #include "checksum.h"
}

using namespace std;

#define HTTP_PORT 80

/**
 * @brief
 * structure defined for the Ethernet header file
*/
struct EthernetHead{
    u_int8_t destAddr[6];
    u_int8_t srcAddr[6];
    u_int16_t Type;
};

/**
 *  @brief
 * Structure defined for the ARP header file
*/
struct ARPHead{
    u_int8_t arp_hard[2];
    u_int8_t arp_protocol[2];
    u_int8_t arp_hardware_len;
    u_int8_t arp_protocol_len;
    u_int16_t Opcode;
    u_int8_t senderMAC[6];
    u_int8_t senderIP[4];
    u_int8_t targetMAC[6];
    u_int8_t targetIP[4];
};

/**
 * @brief
 * Structure defined for the UDP header
*/
struct UDPHead{
    uint16_t source;
    uint16_t destination;
    uint16_t length;
    uint16_t check_sum;
};

/**
 * @brief
 * Structure defined for the IP header file
*/
struct IPHead{
    u_int8_t HeaderLen : 4;
    u_int8_t Version : 4;
    u_int8_t TOS;
    u_int16_t IP_PDU_LEN;
    u_int8_t ID[2];
    u_int8_t fragOff[2];
    u_int8_t TTL;
    u_int8_t Protocol;
    u_int16_t Checksum;
    u_int8_t SenderIp[4];
    u_int8_t Dest_Ip[4];
};

/**
 * @brief
 * Structure defined for the IP ICMP header
 * union fields are the representation of
 * use case field, since they relate to same
 * memory our use case is applicable to it.
*/
struct IP_icmp{
    uint8_t type;
    uint8_t code;
    uint8_t checksum[2];
    uint8_t unionfields[4];
};


/**
 * @brief
 * Struct defined to store the value of the TCP header
*/
struct TCPHead{
    u_int16_t sourcePort;
    u_int16_t destPort;
    u_int32_t sequence;
    u_int32_t ackNumber;
    u_int8_t Do : 4;
    u_int8_t RSV : 4;

    u_int8_t fin : 1;
    u_int8_t syn : 1;
    u_int8_t rst : 1;
    u_int8_t psh : 1;
    u_int8_t ack : 1;
    u_int8_t urg : 1;
    u_int8_t res2 : 2;

    u_int16_t window;
    u_int16_t checksum;
    u_int16_t urgentPointer;
};


/**
 * @brief
 * This pseudoheader is designed to calculate the
 * checksum value for the TCP header
*/
struct IP_pseudoheader {
    u_int8_t sourceIP[4];
    u_int8_t destinationIP[4];
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t TCP_len;
};

/**
 * @brief
 * provides the information on the icmHead.
 * The type values is displayed based off the integer
 * value of type from the network packet
 * @param packet_data
*/
void IP_icmp_HeadFunc(const u_char * packet_data, const IPHead IPhead){
    IP_icmp IP_icmHead;
    
    memcpy(&IP_icmHead,packet_data + sizeof(EthernetHead) + IPhead.HeaderLen*4,sizeof(IP_icmp));
    printf("\n");
    printf("\tICMP Header\n");
    if(IP_icmHead.type == 0){
        printf("\t\tType: Reply\n");
    }
    else if(IP_icmHead.type == 8){
        printf("\t\tType: Request\n");
    }
    else{
        printf("\t\tType: %u\n",IP_icmHead.type);
    }
}

/**
 * @brief
 * Function stores the UDP header information based on the Network data
 * and displays the source, destination port number.
 * 
 * @param packet_data
*/
void UDP_head_Func(const u_char * packet_data, const IPHead IPhead){
    UDPHead UDPhead;
    memcpy(&UDPhead,packet_data + sizeof(EthernetHead) + IPhead.HeaderLen*4,sizeof(UDPHead));
    printf("\n");
    printf("\tUDP Header\n");
    printf("\t\tSource Port: : %u\n",ntohs(UDPhead.source));
    printf("\t\tDest Port: : %u\n",ntohs(UDPhead.destination));
}


/**
 * @brief
 * function stores and display the data for the TCP header.
 * It also creates an instance of the IP suedo header to store the
 * required value in order to perform the checksum.
 * The check sum value is displayed then if it is correct or incorrect.
 * 
 * @param packet_data, IPhead
*/
void TCPheadFunc(const u_char * packet_data,const IPHead IPhead){
    TCPHead TCPhead;
    memcpy(&TCPhead,packet_data + sizeof(EthernetHead) + IPhead.HeaderLen*4,sizeof(TCPHead));
    printf("\n");
    printf("\tTCP Header\n");
    u_int16_t sourcePort = ntohs(TCPhead.sourcePort);   //holds the value of sourcePort in host order
    sourcePort == HTTP_PORT ? printf("\t\tSource Port:  HTTP\n") : printf("\t\tSource Port: : %u\n",ntohs(TCPhead.sourcePort));
    u_int16_t destPort = ntohs(TCPhead.destPort);   //holds the value of destPort in host order
    destPort == HTTP_PORT ? printf("\t\tDest Port:  HTTP\n") : printf("\t\tDest Port: : %u\n",ntohs(TCPhead.destPort));
    printf("\t\tSequence Number: %u\n",ntohl(TCPhead.sequence));
    TCPhead.ack == 0 ? printf("\t\tACK Number: <not valid>\n") : printf("\t\tACK Number: %u\n",ntohl(TCPhead.ackNumber));
    //print flags
    TCPhead.ack == 0 ? printf("\t\tACK Flag: No\n") : printf("\t\tACK Flag: Yes\n");
    TCPhead.syn == 0 ? printf("\t\tSYN Flag: No\n") : printf("\t\tSYN Flag: Yes\n");
    TCPhead.rst == 0 ? printf("\t\tRST Flag: No\n") : printf("\t\tRST Flag: Yes\n");
    TCPhead.fin == 0 ? printf("\t\tFIN Flag: No\n") : printf("\t\tFIN Flag: Yes\n");

    printf("\t\tWindow Size: %u\n",ntohs(TCPhead.window));

    //calculate the TCP PDU length by taking subtracting the IP header from IP PDU
    u_int16_t TCP_PDU_len = ntohs(IPhead.IP_PDU_LEN) - (IPhead.HeaderLen*4);
    // dynamic array to hold the total size of TCP + IP pseudoheader, is used for checksum
    u_int8_t *TCP_check_sum = new uint8_t[TCP_PDU_len + sizeof(IP_pseudoheader)];

    //fill the pseudoheader with the expected val
    IP_pseudoheader tempIP;
    tempIP.reserved = 0;
    memcpy(tempIP.sourceIP,IPhead.SenderIp,sizeof(u_int32_t));
    memcpy(tempIP.destinationIP,IPhead.Dest_Ip,sizeof(u_int32_t));
    tempIP.protocol = IPhead.Protocol;
    tempIP.TCP_len = htons(TCP_PDU_len);    //note TCP_PDU_len needs to be in network order

    //first get the data from the IP_psudeoHeader
    memcpy(TCP_check_sum,&tempIP,sizeof(IP_pseudoheader));
    //then get the data for the TCP header and TCP data-seg
    memcpy(TCP_check_sum + sizeof(IP_pseudoheader), packet_data + sizeof(EthernetHead) + IPhead.HeaderLen*4, TCP_PDU_len);
    //perform the checksum
    u_int16_t checkSumAns = in_cksum((u_int16_t *)TCP_check_sum,TCP_PDU_len + sizeof(IP_pseudoheader));
    //post result
    checkSumAns == 0 ? printf("\t\tChecksum: Correct (0x%x)\n",ntohs(TCPhead.checksum)) : printf("\t\tChecksum: Incorrect (0x%x)\n",ntohs(TCPhead.checksum));
    delete[] TCP_check_sum; //remove the dynamic allocated checksum block
}

/**
 * @brief
 * provides the IP header information.
 * Also checks if the protocol type is icmp then
 * executes the icmp head function to display it's
 * content.
 * @param packet_data
*/
void IPHeadFunc(const u_char * packet_data){
    IPHead IPhead;
    memcpy(&IPhead,packet_data + sizeof(EthernetHead),sizeof(IPHead));
    u_int16_t pdu =  ntohs(IPhead.IP_PDU_LEN);
    char HostIPAddr[INET_ADDRSTRLEN];   //to hold the return string value 
    
    //the header length is 4 bits but it represents number of 32 bits word in the
    //header. To get the header size we need to multiply the number of headers by
    //4. so eg 5 is HeaderLen, so size= 5*(32bits) or 5*(4 Bytes) = 20 Bytes
    u_int8_t len =  IPhead.HeaderLen*4;

    //performing the checksum on the Ip header
    u_int16_t checkSumAns = in_cksum((u_int16_t *)(packet_data + sizeof(EthernetHead)), IPhead.HeaderLen*4);

    printf("\tIP Header\n");
    printf("\t\tHeader Len: %u (bytes)\n",len);
    printf("\t\tTOS: 0x%x\n",IPhead.TOS);
    printf("\t\tTTL: %u\n",IPhead.TTL);
    printf("\t\tIP PDU Len: %u (bytes)\n",pdu);
    if(IPhead.Protocol == 1){
        printf("\t\tProtocol: ICMP\n");
    }
    else if(IPhead.Protocol == 17){
        printf("\t\tProtocol: UDP\n");
    }
    else if(IPhead.Protocol == 6){
        printf("\t\tProtocol: TCP\n");
    }
    else{
        printf("\t\tProtocol: Unknown\n");
    }
    checkSumAns == 0 ? printf("\t\tChecksum: Correct (0x%x)\n",IPhead.Checksum) : printf("\t\tChecksum: Incorrect (0x%x)\n",IPhead.Checksum);
    printf("\t\tSender IP: %s\n",inet_ntop(AF_INET,(struct in_addr*)IPhead.SenderIp,HostIPAddr,INET_ADDRSTRLEN));
    printf("\t\tDest IP: %s\n",inet_ntop(AF_INET,(struct in_addr*)IPhead.Dest_Ip,HostIPAddr,INET_ADDRSTRLEN));

    if(IPhead.Protocol == 1){
        IP_icmp_HeadFunc(packet_data,IPhead);
    }
    else if(IPhead.Protocol == 17){
        UDP_head_Func(packet_data, IPhead);
    }
    else if(IPhead.Protocol == 6){
        TCPheadFunc(packet_data, IPhead);
    }
}

/**
    @brief 
    creates the instance of Ethernet, copy the header info
    from packet_data and store it in the Ethernet struct.
    convert the binary data of each of the header type to 
    ASCII value and print it in the readable format
    @param packet_data
*/
u_int16_t EthernetHeadFunc(const u_char * packet_data){
    EthernetHead etherHead;
    memcpy(&etherHead, packet_data, sizeof(EthernetHead));

    u_int16_t etherType =  ntohs(etherHead.Type);
    printf("\tEthernet Header\n");
    printf("\t\tDest MAC: %s\n",ether_ntoa((struct ether_addr *)etherHead.destAddr));
    printf("\t\tSource MAC: %s\n",ether_ntoa((struct ether_addr *)etherHead.srcAddr));
    if(etherType == 2048){
        printf("\t\tType: IP\n");
    }
    else if(etherType == 2054){
        printf("\t\tType: ARP\n");
    }
    else{
         printf("\t\tType: Unknown\n");
    }
    printf("\n");

    return etherType;
}

/**
 * @brief
 * takes the packet_data pointer in, moves the pointer to ARP header
 * store the data in the ARP header. Opcode is converted to host bits
 * order. Value is printed on the console in the specific order
 * Operation val is represented as an array and based off the index val
 * 
 * @param packet_data
*/
void ARPHeadFunc(const u_char * packet_data){
    string OperationVal[26] = {"Reserved", "Request", "Reply", "Request Reverse", "Reply Reverse", "DRARP Request", 
    "DRARP Reply", "DRARP Error", "InARP Request", "InARP Reply", "ARP NAK", "MARS Request", "MARS Multi", 
    "MARS MServ", "MARS Join", "MARS Leave", "MARS NAK", "MARS Unserv", "MARS SJoin", "MARS SLeave", 
    "MARS Grouplist Request", "MARS Grouplist Reply", "MARS Redirect Map", "MAPOS UNARP", "OP_EXP1", "OP_EXP2"};

    ARPHead arpHead;
    memcpy(&arpHead, packet_data + sizeof(EthernetHead), sizeof(ARPHead));
    u_int16_t OpcodeVal = ntohs(arpHead.Opcode);  //convert 16 bits network bits to 16 bits host bits order
    char HostIPAddr[INET_ADDRSTRLEN];   //to hold the return string value 
    printf("\tARP header\n");
    OpcodeVal > 25 ? printf("\t\tOpcode: Unknown\n") : printf("\t\tOpcode: %s\n",OperationVal[OpcodeVal].c_str());

    printf("\t\tSender MAC: %s\n",ether_ntoa((struct ether_addr *)arpHead.senderMAC));
    printf("\t\tSender IP: %s\n",inet_ntop(AF_INET,(struct in_addr*) arpHead.senderIP,HostIPAddr,INET_ADDRSTRLEN));
    printf("\t\tTarget MAC: %s\n",ether_ntoa((struct ether_addr *)arpHead.targetMAC));
    printf("\t\tTarget IP: %s\n",inet_ntop(AF_INET,(struct in_addr*) arpHead.targetIP,HostIPAddr,INET_ADDRSTRLEN));
    printf("\n");
}



int main(int argc, char **argv){
    if(argc < 2 || argc > 2){
        printf("Not the right number of arguments provided.");
        return -1;
    }
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t * readFile = pcap_open_offline(argv[1], errbuf);
    struct pcap_pkthdr *header;   // Packet header
    const u_char *packet_data;
    int result = 1;
    u_int16_t packetNum = 1;

    while(result == 1){
        result = pcap_next_ex(readFile,&header,&packet_data);
        if(result != 1){
            break;
        }
        printf("\n");
        printf("Packet number: %u  Frame Len: %u\n\n",packetNum,header->len);

        u_int16_t etherType = EthernetHeadFunc(packet_data);
        if(etherType == 2048){
            IPHeadFunc(packet_data);
        }
        else if(etherType == 2054){
            ARPHeadFunc(packet_data);
        }
        
        packetNum++;
    }


    //close the pcap_file that is opened
    pcap_close(readFile);

    return 0;
}