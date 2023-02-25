

# Lab5（the network interface）



OK，又是忙碌的一周，由于对于网络层和链路层的不熟悉，我花了2，3天的时间去补郑老师第5章的内容，对路由和网络层有一个大致的了解后开启了本次的lab之旅。

本次lab是让你实现一个network接口，也就是相当于网络层和链路层的接口，将IPv4地址（网络层）转化为MAC地址（链路层）的协议（ARP协议），是一个很有意思的lab，由于Stanford以及提供好官方的IPv4地址和ARPMessage，以太网地址（MAC）的类分装，我们只需要实现ARP协议的接口即可，相对lab4会简单很多啦

![](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/lab5.png)

这是整个实验的大致图，我们本次实验实现的是对Internet datagrams（IP数据报）解析为Ethernet frames（以太网数据报）的接口，在实验前，我们需要了解一ARP协议以及IPv4数据报的结构。



## 1.ARP协议



那么什么是ARP协议呢，ARP协议是地址解析协议，大致是将IP地址转化为MAC地址的一种协议。我们知道，主机或者路由器是不具备MAC地址的，只有他们的网络接口具备，我们的计算机网络是一层一层的结构，从TCP包分装成IP数据包，在分装成一帧打出去，MAC地址确定了这一帧打出去的目的地。

没台主机或者路由都具备一个临时ARP表，上面相当于写好了局域网内各个IP对应MAC地址的一个集合，我们每次需要发送帧的时候需要查找ARP表如果在这个表上，我们直接用，如果不在表上，会进行一种Broadcast（广播）的操作，就相当于我们向多个主机询问IP以及MAC地址，然后去收集信息存到自己的表里。每个表里存放着目的IP地址，MAC地址，以及存活时间TTL（因为这是临时的表，所以需要）

![](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/MAC.png)

那么为什么需要有MAC地址呢？

* 链路层不仅仅是为IP和因特网提供服务的
* IP地址是临时的，如果重启电脑或者换个地方安装主机或者路由器，都会重新配置



## 2.网络结构实现



首先我们需要明确我们的数据结构

* arp_table ：ARP表
* wait_to_address：用于ARP广播的时候存放已发出去的ARP请求但没有相应的包
* wait_to_ip_address ：用于存放需要发送的带有数据的帧，但没有明确mac地址的包

```c++
  struct arp
    {
        EthernetAddress ethernet_addrr;
        size_t ttl; 
    };
    //arp表,uint32_t是ipv4地址
    std::map<uint32_t ,arp> arp_table={};
    const size_t table_default_time=1000*30;
    std::map<uint32_t,size_t>wait_to_address={};
    const size_t wait_arp_ttl =1000*5;
    std::list<std::pair<Address,InternetDatagram>> wait_to_ip_address={};  

    //! Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
    EthernetAddress _ethernet_address;

    //! IP (known as internet-layer or network-layer) address of the interface
    Address _ip_address;

    //! outbound queue of Ethernet frames that the NetworkInterface wants sent
    std::queue<EthernetFrame> _frames_out{};
```

ps：这里我主要补了一下C++容器的知识以及迭代器的是用，这里不做过多介绍。



* **send_datagram（）函数**

    

    这里的send函数可以主要判断是否在ARP表里，如果不在，则进行广播，如果在就直接发送就好，这里发送的原理和前几个lab的原理一样，放入队列即可。这里需要注意的是，我们是将ARPMessage数据报写好目的和原地址，然后分装成一个EthernetFrame(帧)发送的。在广播的时候是不需要写目的MAC地址的，因为广播的时候会将MAC地址自动写为**（FF-FF-FF-FF-FF-FF）**，这样做的目的是为了让**所有**子网上的其他适配器都接收到。

```c++
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
```



* **recv_frame （）函数**



​		这里补了一下C++ optional的知识，大致意思就是让你的函数有两种返回值一种是像bool一样的对错返回值，一种是你想要的数据类型返回值（比如int，double，"InternetDatagram"），这里主要分为以下几种情况   ：如果是IPV4的包就返回正确，如果不是则需返回错误nullopt（情况分为1.不是自己的包，2.是一个ARP包）



```c++
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
            frame_.header()={ethernet_src,_ethernet_address,EthernetHeader::TYPE_ARP};
            frame_.payload()=message_reply.serialize();
            _frames_out.push(frame_);

        }
        //更新arp_table表
        if(is_send_to_me ||is_respone){
            arp_table[ip_src]={ethernet_src,table_default_time};
            //删除wait里的
            for(auto item=wait_to_ip_address.begin();item!=wait_to_ip_address.end();){
                if(item->first.ipv4_numeric()==ip_src){
                    //重新发送附带正确Ethernet地址的帧
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
```



* **tick（）函数**

    这里的函数很类似与前面的lab的计时器，需要注意的点在更新arp表

```c++
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
            //如果超时，则重新广播
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
```

