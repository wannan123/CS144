

# Lab4（the TCP connection）



lab4是一项非常难的工程，由于这周回到学校了，效率不是很高，这个lab花了我足足1周的时间，文档读了3天左右2天改代码，两天调试，真的很累人，这次的实验算是前面几次里难度最高的，不管是从文档角度来说还是从代码和调试角度来说，都是一个维度的跳跃，因为前面几次的lab我做完后都有核对github上各位大佬的代码，所以我的tcp_sender和receiver的鲁棒性都很高，我在写代码的时候没有遇到雷人的需要去改之前的代码的情况，这虽然方便了自己，但是还是缺少了很多锻炼自己的能力。

本次实验的难点我认为在于理解TCP有限状态机的各个状态，也就是segment_received函数中各个判断条件，这次的实验是结合lab3和lab2的sender和receiver来做的他其实就是想让你站在一个更高的角度来俯瞰整个状态，就是不管是client端还是server段你都需要考虑sender和receiver的状态，sender和receiver结合成connection存在与两端。



## 1.TCP的三次握手和四次挥手

TCP的有限状态机分为三次握手和四次挥手，理解这一点很重要

* 三次握手

![](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/3.png)



我们在还没有建立连接的时候服务端是保持LISTEN状态的，当客户端发起请求的时候，发送SYN=1（第一次握手）服务端接收到这个SYN然后给一个确认ACK=1,并且也发送一个SYN=1（第二次握手）在客户端发送出去SYN的时候他是处于SYN -SENT状态的，服务段发送确认和SYN的时候服务端处于SYN-RCVD状态，当客户端接收到服务端发送的SYN后也要给一个确认给客户端（第三次握手）并且发送数据，这样就达成了建立。

但有人会问为什么不是两次握手呢？就好比小明想给小红表白寄信，第一次小明发送我喜欢你（第一次握手），但是由于快递员记错位置了很长时间都没有送到，于是他又发送了一次我喜欢你，这次小红收到了信，也发送我也喜欢你并且送上了一束花（数据）（第二次握手）从此他们幸福的在一起了，但是突然有一天分手了，这个时候小明一开始发送的第一封信终于送到小红手里了小红以为小明要复合，于是再次发送我也喜欢你并且送上了一束花（数据）但是小明很渣，他就没离小红于是小红一直以为小明没有收到信，所以一直发送数据，这样就导致过度的浪费资源了。但是如果用三次握手的话，第二次小红给小明发送我也喜欢你的时候是不需要送花的，只有当小明知道小红也喜欢他的时候这样小明再发一次那我们在一起吧（第三次握手）并且送花，这样就会避免数据缺失，但是有人会问难道小红发送的时候不会延迟嘛，因为小明他受到的是SYN和一个ACK，所以这个时刻只有小明知道自己之前有没有发送请求。当然也有很多复杂的情况，但是三次握手能相对的解决大部分问题。

* 四次挥手

![](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/4挥手.png)



四次挥手用通俗的话来说就是，小明想要给向红说咱们分手把（第一次挥手）小红收到信后，发送好的我同意你的分手（第二次挥手）但是你得等我给你送一点最后的分手礼物，当小红送完了，发送好了，现在可以分手了（第三次挥手）小明收到后发送，那你关机吧（第四次挥手）这个时候小明会等待一段时间，如果这个时间里在没有收到小红的第三次挥手的信，小明会自动关机，小红收到信后也会关机，这样就保持了双方都能关机。

在客户端的FIN-WAIT-1到接收到服务段的FIN的时候客户端的receiver都是开启的。

代码来说，我们需要注意的点是用seg的头部syn，ack的布尔值和receiver，sender的各个方法来确认个状态，我们用sender发送空的segment来代表ack，并且要注意在建立状态的时候要时不时发送一个空的segment来确保是连接状态

```c++
void TCPConnection::segment_received(const TCPSegment &seg) {
   // 非启动时不接收
   if (!active_) {
       return;
   }

   // 重置连接时间
   ms_since_last = 0;

   // RST标志，直接关闭连接
   if (seg.header().rst) {
       // 在出站入站流中标记错误，使active返回false
       _receiver.stream_out().set_error();
       _sender.stream_in().set_error();
       active_ = false;
   }
   // 当前是Closed/Listen状态
   else if (_sender.next_seqno_absolute() == 0) {
       // 收到SYN，说明TCP连接由对方启动，进入Syn-Revd状态
       if (seg.header().syn) {
           // 此时还没有ACK，所以sender不需要ack_received
           _receiver.segment_received(seg);
           // 我们主动发送一个SYN
           connect();
       }
   }
   // 当前是Syn-Sent状态
   else if (_sender.next_seqno_absolute() == _sender.bytes_in_flight() && !_receiver.ackno().has_value()) {
       if (seg.header().syn && seg.header().ack) {
           // 收到SYN和ACK，说明由对方主动开启连接，进入Established状态，通过一个空包来发送ACK
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_merge_segment();
       } else if (seg.header().syn && !seg.header().ack) {
           // 只收到了SYN，说明由双方同时开启连接，进入Syn-Rcvd状态，没有接收到对方的ACK，我们主动发一个
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_merge_segment();
       }
   }
   // 当前是Syn-Revd状态，并且输入没有结束
   else if (_sender.next_seqno_absolute() == _sender.bytes_in_flight() && _receiver.ackno().has_value() &&
            !_receiver.stream_out().input_ended()) {
       // 接收ACK，进入Established状态
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
   }
   // 当前是Established状态，连接已建立
   else if (_sender.next_seqno_absolute() > _sender.bytes_in_flight() && !_sender.stream_in().eof()) {
       // 发送数据，如果接到数据，则更新ACK
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       if (seg.length_in_sequence_space() > 0) {
           _sender.send_empty_segment();
       }
       _sender.fill_window();
       send_merge_segment();
   }
   // 当前是Fin-Wait-1状态
   else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
            _sender.bytes_in_flight() > 0 && !_receiver.stream_out().input_ended()) {
       if (seg.header().fin) {
           // 收到Fin，则发送新ACK，进入Closing/Time-Wait
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_merge_segment();
       } else if (seg.header().ack) {
           // 收到ACK，进入Fin-Wait-2
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           send_merge_segment();
       }
   }
   // 当前是Fin-Wait-2状态
   else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
            _sender.bytes_in_flight() == 0 && !_receiver.stream_out().input_ended()) {
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       _sender.send_empty_segment();
       send_merge_segment();
   }
   // 当前是Time-Wait状态
   else if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
            _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
       if (seg.header().fin) {
           // 收到FIN，保持Time-Wait状态
           _sender.ack_received(seg.header().ackno, seg.header().win);
           _receiver.segment_received(seg);
           _sender.send_empty_segment();
           send_merge_segment();
       }
   }
   // 其他状态
   else {
       _sender.ack_received(seg.header().ackno, seg.header().win);
       _receiver.segment_received(seg);
       _sender.fill_window();
       send_merge_segment();
   }
}
```

## 2.write部分

这一部分应该是相对来说简单的，因为指导书上有说到如何开始，我们可以从write部分开始。

如何发送呢？其实connection就是将receiver的ack和窗口大小与sender中的segment相结合然后放在一个队列上，发送出去（和sender的原理很像）这里我们需要自己写一个函数来解决合并问题

```c++
void TCPConnection::send_merge_segment() {
   // 将sender中的数据保存到connection中
   while (!_sender.segments_out().empty()) {
       TCPSegment seg = _sender.segments_out().front();
       _sender.segments_out().pop();
       // 设置ackno和window_size
       if (_receiver.ackno().has_value()) {
           seg.header().ack = true;
           seg.header().ackno = _receiver.ackno().value();
           seg.header().win = _receiver.window_size();
       }
       _segments_out.push(seg);
   }
   // 如果发送完毕则结束连接
   if (_receiver.stream_out().input_ended()) {
       if (!_sender.stream_in().eof()) {
           _linger_after_streams_finish  = false;
       }

       else if (_sender.bytes_in_flight() == 0) {
           if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
               active_ = false;
           }
       }
   }
}
```

其次我们需要一个解决rsp问题的函数，也就是如果发生了错误，我们需要发送一个rsp段并且关闭连接，这个错误通常在于超时次数过多。

```c++
void TCPConnection::deal_rsp(bool send_rst) {
   // 发送RST标志
   if(send_rst){
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
   }


   // 在出站入站流中标记错误，使active返回false
   _receiver.stream_out().set_error();
   _sender.stream_in().set_error();
   active_ = false;
}
```

和判断是否超时的函数

```c++
void TCPConnection::tick(const size_t ms_since_last_tick) {
   // 非启动时不接收
   if (!active_) {
       return;
   }

   // 保存时间，并通知sender
   ms_since_last+= ms_since_last_tick;
   _sender.tick(ms_since_last_tick);

   // 超时需要重置连接
   if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
       deal_rsp(true);
       return;
   }
   send_merge_segment();
}
```

接下来就是相对比较简单的连接函数，写函数，以及结束发送函数了

```c++
size_t TCPConnection::write(const string &data) {
   if (data.empty()) {
       return 0;
   }

   // 在sender中写入数据并发送
   size_t size = _sender.stream_in().write(data);
   _sender.fill_window();
   send_merge_segment();
   return size;
}

void TCPConnection::connect() {
   // 主动启动，fill_window方法会发送Syn
   _sender.fill_window();
   send_merge_segment();
}
//析构函数
TCPConnection::~TCPConnection() {
   try {
       if (active()) {
           cerr << "Warning: Unclean shutdown of TCPConnection\n";

           // Your code here: need to send a RST segment to the peer
           deal_rsp(false);
       }
   } catch (const exception &e) {
       std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
   }
}
```

因为sender的fill_window自带发送fin和syn的效果，所以直接调用就行了。

调试的话就用wireshark抓包，具体可以上知乎查查细节，本次实验是模拟一个虚拟网卡在本机上建立服务器和客户端来通信的，最后可以运行玩完，自此CS144的5个lab终于做完了，我也在思考要不要继续往下走，做完lab5,6

加油吧！