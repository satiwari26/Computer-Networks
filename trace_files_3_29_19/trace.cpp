#include <iostream>
#include <pcap.h>
#include <cstring>
#include <vector>
#include <netinet/ether.h>
#include <net/if_arp.h>

using namespace std;

/*Note to self: Need to verify that checksum for IP is working
*Also need to get the icmp type to work, the first packet is not outputting the
*right expect value in the result*/

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
 * checksum algorithm provided by the professor
 * will be used to detect the error and return the
 * answer.
 * @param addr, len
*/
unsigned short in_cksum(unsigned short *addr,int len)
{
        int sum = 0;
        u_short answer = 0;
        u_short *w = addr;
        int nleft = len;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

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
 * provides the information on the icmHead.
 * The type values is displayed based off the integer
 * value of type from the network packet
 * @param packet_data
*/
void IP_icmp_HeadFunc(const u_char * packet_data){
    IP_icmp IP_icmHead;
    memcpy(&IP_icmHead,packet_data + sizeof(EthernetHead) + sizeof(IPHead),sizeof(IP_icmp));
    cout<<endl;
    cout<<"\tICMP Header"<<endl;
    if(IP_icmHead.type == 0){
        printf("\t\tType: Reply\n\n");
    }
    else if(IP_icmHead.type == 8){
        printf("\t\tType: Request\n\n");
    }
    else{
        printf("\t\tType: %d\n\n",IP_icmHead.type);
    }
}

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
 * Function stores the UDP header information based on the Network data
 * and displays the source, destination port number.
 * 
 * @param packet_data
*/
void UDP_head_Func(const u_char * packet_data){
    UDPHead UDPhead;
    memcpy(&UDPhead,packet_data + sizeof(EthernetHead) + sizeof(IPHead),sizeof(UDPHead));
    printf("\n");
    printf("\tUDP Header\n");
    printf("\t\tSource Port: : %d\n",ntohs(UDPhead.source));
    printf("\t\tDest Port: : %d\n",ntohs(UDPhead.destination));
    cout<<endl;
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
    u_int16_t checkSumAns = in_cksum((u_int16_t *)&IPhead, IPhead.HeaderLen);

    cout<<"\tIP Header"<<endl;
    printf("\t\tHeader Len: %d (Bytes)\n",len);
    printf("\t\tTOS: 0x%x\n",IPhead.TOS);
    printf("\t\tTTL: %d\n",IPhead.TTL);
    printf("\t\tIP PDU LEN: %d (Bytes)\n",pdu);
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
        printf("\t\tProtocol: unknown\n");
    }
    IPhead.Checksum == checkSumAns ? printf("\t\tChecksum: Correct (0x%x)\n",IPhead.Checksum) : printf("\t\tChecksum: Incorrect (0x%x)\n",IPhead.Checksum);
    cout<<"\t\tSender IP: "<<inet_ntop(AF_INET,(struct in_addr*)IPhead.SenderIp,HostIPAddr,INET_ADDRSTRLEN)<<endl;
    cout<<"\t\tDest IP: "<<inet_ntop(AF_INET,(struct in_addr*)IPhead.Dest_Ip,HostIPAddr,INET_ADDRSTRLEN)<<endl;

    if(IPhead.Protocol == 1){
        IP_icmp_HeadFunc(packet_data);
    }
    else if(IPhead.Protocol == 17){
        UDP_head_Func(packet_data);
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

    cout<<"\t EtherNet Header"<<endl;
    cout<<"\t\t Dest MAC: "<<ether_ntoa((struct ether_addr *)etherHead.destAddr)<<endl;
    cout<<"\t\t Source MAC: "<<ether_ntoa((struct ether_addr *)etherHead.srcAddr)<<endl;
    if(etherType == 2048){
        cout<<"\t\t Type: "<<"IP"<<endl;
    }
    else if(etherType == 2054){
        cout<<"\t\t Type: "<<"ARP"<<endl;
    }
    else{
         cout<<"\t\t Type: "<<"Unknown"<<endl;
    }
    cout<<endl;

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
    cout<<"\tARP Header\n";
    OpcodeVal > 25 ? cout<<"\t\tOpcode: "<<"Unknown"<<endl : cout<<"\t\tOpcode: "<<OperationVal[OpcodeVal]<<endl;

    cout<<"\t\tSender Mac: "<<ether_ntoa((struct ether_addr *)arpHead.senderMAC)<<endl;
    cout<<"\t\tSender IP: "<<inet_ntop(AF_INET,(struct in_addr*) arpHead.senderIP,HostIPAddr,INET_ADDRSTRLEN)<<endl;
    cout<<"\t\tTarget Mac: "<<ether_ntoa((struct ether_addr *)arpHead.targetMAC)<<endl;
    cout<<"\t\tTarget IP: "<<inet_ntop(AF_INET,(struct in_addr*) arpHead.targetIP,HostIPAddr,INET_ADDRSTRLEN)<<endl;
    cout<<endl;
}



int main(){
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t * readFile = pcap_open_offline("UDPfile.pcap", errbuf);
    struct pcap_pkthdr *header;   // Packet header
    const u_char *packet_data;
    int result = 1;
    vector<EthernetHead> etherHeadStorage;
    int packetNum = 1;

    while(result == 1){
        cout<<endl;
        result = pcap_next_ex(readFile,&header,&packet_data);
        if(result != 1){
            break;
        }
        cout<<"Packet Number: "<<packetNum<<"  Frame Len: "<<header->len<<endl<<endl;

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