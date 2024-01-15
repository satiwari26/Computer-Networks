#include <iostream>
#include <pcap.h>
#include <cstring>
#include <vector>
#include <netinet/ether.h>
#include <net/if_arp.h>

using namespace std;

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
    @brief 
    creates the instance of Ethernet, copy the header info
    from packet_data and store it in the Ethernet struct.
    convert the binary data of each of the header type to 
    ASCII value and print it in the readable format
    @param packet_data
*/
void EthernetHeadFunc(const u_char * packet_data){
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
    pcap_t * readFile = pcap_open_offline("ArpTest.pcap", errbuf);
    struct pcap_pkthdr *header;   // Packet header
    const u_char *packet_data;
    int result = 1;
    vector<EthernetHead> etherHeadStorage;
    int packetNum = 1;

    while(result == 1){
        cout<<endl;
        result = pcap_next_ex(readFile,&header,&packet_data);
        cout<<"Packet Number: "<<packetNum<<"  Frame Len: "<<header->len<<endl<<endl;

        EthernetHeadFunc(packet_data);
        ARPHeadFunc(packet_data);
        
        packetNum++;
    }


    //close the pcap_file that is opened
    pcap_close(readFile);

    return 0;
}