#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    const uint32_t it=arp_table.find(next_hop_ip);
    
    if(it==arp_table.end()){
        //广播
        if(it==wait_to_address.end()){
            ARPMessage arp_mess;
            arp_mess.opcode=ARPMessage::OPCODE_REQUEST;
            arp_mess.sender_ip_address=_ip_address.ipv4_numeric();
            arp_mess.sender_ethernet_address=_ethernet_address;
            arp_mess.target_ip_address=next_hop_ip;
            arp_mess.target_ethernet_address={};

            //包装成以太网帧；
            EthernetFrame ether_addr_frame;
            ether_addr_frame.header()={ETHERNET_BROADCAST,_ethernet_address,EthernetHeader::TYPE_ARP};
            ether_addr_frame.payload()=arp_mess.serialize();
            _frames_out.push(ether_addr_frame);

            wait_to_address.insert(next_hop_ip,wait_arp_ttl);
        }
        wait_to_ip_address.push_back({next_hop,dgram});
    }else{
        EthernetFrame ether_addr_frame;
        ether_addr_frame.header()={it->second.ethernet_addrr,_ethernet_address,EthernetHeader::TYPE_IPv4};
        ether_addr_frame.payload()=arp_mess.serialize();
        _frames_out.push(ether_addr_frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }
