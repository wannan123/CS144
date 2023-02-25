#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include <optional>
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
    const auto &it=arp_table.find(next_hop_ip);
    
    if(it==arp_table.end()){
        //广播
        if(wait_to_address.find(next_hop_ip)==wait_to_address.end()){
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
            wait_to_address[next_hop_ip]=wait_arp_ttl;
            
        }
        wait_to_ip_address.push_back({next_hop,dgram});
    }else{
        EthernetFrame ether_addr_frame;
        ether_addr_frame.header()={it->second.ethernet_addrr,_ethernet_address,EthernetHeader::TYPE_IPv4};
        ether_addr_frame.payload()=dgram.serialize();//注意serialize的对象
        _frames_out.push(ether_addr_frame);
    }
    
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    //DUMMY_CODE(frame);
    if(frame.header().dst!=_ethernet_address &&frame.header().dst!=ETHERNET_BROADCAST){
        return std::nullopt;
    }
    //if ipv4
    if(frame.header().type==EthernetHeader::TYPE_IPv4){
        InternetDatagram datagram;
        if(datagram.parse(frame.payload())!=ParseResult::NoError){
            return std::nullopt;
        }
        return datagram;
    }
    //if arp
    if(frame.header().type==EthernetHeader::TYPE_ARP){
        ARPMessage messages_;
        if(messages_.parse(frame.payload())!=ParseResult::NoError){
            return std::nullopt;
        }
        const uint32_t &ip_src=messages_.sender_ip_address;
        const EthernetAddress &ethernet_src=messages_.sender_ethernet_address;
        const uint32_t &ip_dst=messages_.target_ip_address;
        const EthernetAddress &ethernet_dst=messages_.target_ethernet_address;
        //只有request是用ip来判断，因为request有可能是广播
        //若是REPLY,mac地址必然知道
        bool is_send_to_me=messages_.opcode==ARPMessage::OPCODE_REQUEST && ip_dst == _ip_address.ipv4_numeric();
        bool is_respone=messages_.opcode==ARPMessage::OPCODE_REPLY && ethernet_dst== _ethernet_address;
        if(is_send_to_me){
            //就需要告知对方自己的消息
            ARPMessage message_reply;
            message_reply.opcode=ARPMessage::OPCODE_REPLY;
            message_reply.sender_ip_address=_ip_address.ipv4_numeric();
            message_reply.sender_ethernet_address=_ethernet_address;
            message_reply.target_ip_address=ip_src;
            message_reply.target_ethernet_address=ethernet_src;

            EthernetFrame frame_;
            frame_.header()={ethernet_src,_ethernet_address,EthernetHeader::TYPE_IPv4};
            frame_.payload()=message_reply.serialize();
            _frames_out.push(frame_);

        }
        //更新arp_table表
        if(is_send_to_me ||is_respone){
            arp_table[ip_src]={ethernet_src,table_default_time};
            //删除wait里的
            for(auto item=wait_to_ip_address.begin();item!=wait_to_ip_address.end();){
                if(item->first.ipv4_numeric()==ip_src){
                    send_datagram(item->second,item->first);
                    item=wait_to_ip_address.erase(item);
                }
                else{
                    ++item;
                }
            }wait_to_address.erase(ip_src);
        }
    }
    return std::nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick);
    for(auto item=arp_table.begin();item!=arp_table.end();){
        if(ms_since_last_tick>=item->second.ttl){
            item=arp_table.erase(item);
        }
        else{
            item->second.ttl-=ms_since_last_tick;
            ++item;
        }
    }
    for(auto item=wait_to_address.begin();item!=wait_to_address.end();){
        if (ms_since_last_tick>=item->second)
        {
            ARPMessage messages_;
            messages_.opcode=ARPMessage::OPCODE_REQUEST;
            messages_.sender_ip_address=_ip_address.ipv4_numeric();
            messages_.sender_ethernet_address=_ethernet_address;
            messages_.target_ip_address=item->first;
            messages_.target_ethernet_address={};

            EthernetFrame frame_;
            frame_.header()={ETHERNET_BROADCAST,_ethernet_address,EthernetHeader::TYPE_ARP};
            frame_.payload()=messages_.serialize();
            _frames_out.push(frame_);
            item->second=wait_arp_ttl;
        }
        else{
            item->second-=ms_since_last_tick;
            ++item;
        }
    }
}
