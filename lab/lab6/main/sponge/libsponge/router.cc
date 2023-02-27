#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    router_list.push_back(router_table{route_prefix,prefix_length,next_hop,interface_num});
    // Your code here.
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    DUMMY_CODE(dgram);
    // Your code here.
    const uint32_t dst=dgram.header().dst;
    bool is_found=false;
    //start search
    auto rt = router_list.end();
    for(auto it=router_list.begin();it!=router_list.end();it++){
        //看是是否为0或者取dst和前缀的高length位
        //uint32_t offset = (it->prefix_length == 0) ? 0 : 0xffffffff << (32 - it->prefix_length);
        
        if(it->prefix_length==0||(it->route_prefix ^ dst)>>(32-it->prefix_length)==0){
            //取有效位最多的，更新一下
            
            if(!is_found||rt->prefix_length<it->prefix_length){
                is_found=true;
                rt=it;
            }
        }
    }
    if(!is_found){
        return;
    }
    if(dgram.header().ttl<=1){
        return;
    }
    dgram.header().ttl--;
    
    if(rt->next_hop.has_value()){
        _interfaces[rt->interface_num].send_datagram(dgram, rt->next_hop.value());
        
    }else{
        _interfaces[rt->interface_num].send_datagram(dgram,Address::from_ipv4_numeric(dgram.header().dst));
    }


    
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
