

# Lab3（the TCP sender）



omg，终于做完lab3了，lab3真的是一个难度的大跳跃，整个难度一下子就上来了，本次实验用时大概有5天的时间，从读文档到最后完成lab，真的是一段艰难的旅途。本次实验的文档让人很难理解，就是读完其实也是很模糊它到底想让我们实现什么内容，所以让我花费的时间很多，大概足足有2天半的时间来读文档和思考，不像之前的实验读完就比较好理解到底是想做什么，本次实验我也是在写代码前做了很多思维导图，去整理逻辑。在改bug的时候也是异常艰难，我感觉不能再单纯的是用cerr了，得找个时间好好学习一下gdb或者其他调试工具。总之这次的实验让我非常头疼hhh不过想到lab4会更难，我还是坚持了下来！还是先看看总体图吧！：

![image-20230124214052262](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230124214052262.png)

我们这次实验是实现一个TCP Sender，也就是发送器，其中也有两大难点：

* 重发计时器的启动，关闭，重启，判断超时。
* 理解Sender的工作流程以及原理。

那么我们开启Lab3之旅吧！

## 1.实现重发计时器

首先要理解文档里的重发计时器到底是怎么工作的，一开始我也很纳闷如果不用系统时间函数那怎么记时呢？随着看完他的函数申明后，是给一个*ms_since_last_tick*d 的参数的，也就是测试用例会时不时的调用tick函数给一个距离上次调用这个函数的时间，也就是说测试用例会给你一个数字，代表过去的时间。根据文档的建议，我将它封装成一个time_类，然后实现它的功能。

第二点我们要知道重发计时器有什么用：我们知道报文在传输的时候会遇到各种困难，有可能会丢失，为了解决这个问题我们需要重新发送报文，但是这个重新发送需要有一定的限制，它的时间阈值是动态的，每当超过这个阈值sender还没有收到receiver的ack时我们需要重新发送之前的报文。在一开始会认为是每次发送一段segment都要记录一下，其实不然。本实验的意思就是记录最初没有收到ack的报文，当收到报文的ack时重新发送这段数据包就可以啦，其次是这个动态阈值是如何变化的，为何变化的：因为网络的拥堵不是定值，只要发生一次需要重传，我们都需要将阈值扩大，因为既然发生了拥堵，就说明我们需要多等待一会儿。

我们还需要一个值来记录在一次segment发送后到接收到ack一共经历了多少次重传,这里默认重发计时器是关闭的

```c++
class time_{
  private:

    bool is_start=false;
    unsigned int init_time;
    unsigned int now_time;
    unsigned int judge_time;

  public:
  unsigned int count_time=0;
    time_(const uint16_t retx_time)
      :init_time(retx_time)
      ,now_time(0)
      ,judge_time(retx_time){}
    bool isstart(){
      return is_start;
    }
    void active(){
      is_start=true;
      judge_time=init_time;
      count_time=0;
      now_time=0;
    }
    void close_(){
      is_start=false;
      judge_time=init_time;
      count_time=0;
      now_time=0;
    }
    void restart(){
      is_start=true;
      now_time=0;
    }
    void double_time(const size_t _last_window_size,TCPSegment &seg){
      if(!is_start){
        return;
      }
      if(_last_window_size!=0||seg.header().syn){
          
        judge_time=judge_time<<1;

        count_time++;
      }
      
    }
    bool tick_time(const size_t ms_since_last_tick){
      
      if(!is_start){
        return false;
      }
      now_time+=ms_since_last_tick;
      if(now_time>=judge_time){
          return true;
      }
      
      return false;
    }
};
```

注：这里的启动关闭和重启函数的实现要具体到调用，其中我的逻辑是一旦接收到ack就关闭计时器，将所有参数归为最原始的数据。如果在普通发送的时候计时器没有开启，我们便将它开启调用active()函数即可。在检查是否超时的时候，如果超时了就重发，重发只需要修改累计时间和累计次数，我们需要调用tick_time()，double_time()，以及restart()函数，所以每一个函数的实现都是由讲究滴~





## 2.Sender的具体实现

首先我们需要明确Sender的工作流程：

	1. 调用fill_window（）函数，判断是否是段尾（FIN) 或者段头（SYN）如果是SYN是不需要传数据的，直接修改TCPsegment的头部即可，FIN在本实验中是可以携带信息的，可以在后面处理。
 	2. 调用send_segment（）函数，这个函数是自己写的，也就是发送函数，这个函数的使用者是fill_window（）
 	3. 时不时调用tick函数来检查是否超时，如果超时就重新发送。
 	4. 调用ack_received（）函数来接收已经成功发送的段的ack，并关闭重发计时器。

我们需要注意几个细节。

	*	文档里说了窗口的设置：如果receiver返回的窗口大小为0，也就是说明接收器满了，希望sender再别发送了，这时候其实就有个问题，sender什么时候知道receiver有空能接收数据呢？我们需要手动将window改为1，这样就可以不断的发送小的数据报，直到接收器由空他就可以返回ack，继续发送了
	*	segment一直放在wait_seg里，每当要重新传时，放入seg_out里，当接收到ack时就pop掉wait_seg。每次需要发送的时候就取wait_seg的头部就好了，这里不需要pop掉seg_out，这里的pop好像在测试用例还是哪里已经帮我们实现了
	*	所谓的发送的数据报就是放到seg_out里
	*	我们需要理解一下边界：

![](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/QQ图片20230209013111.png)

#### 1.fill_window（）函数

我们需要注意发送的内容时从stream里read()出来的，大小也就是上图中黄色的部分，并且我们需要判断一下正在发送的内容有没有越界，

```c++
void TCPSender::fill_window() {
    size_t window_sizes = receiver_window_size_ > 0 ?receiver_window_size_ : 1;
    TCPSegment segment;
    if(is_fin){
        return;
    }

    if(!is_syn){
        
        segment.header().syn=true;
        segment.header().seqno=next_seqno();
        wait_ack.push(segment);
        send_segment(segment);
        is_syn=true;
        return;
    }
    
    if (_next_seqno == bytes_in_flights) {
        return;
    }

    if(_stream.eof() && window_sizes>_next_seqno-rece_seqno){
        is_fin=true;
        segment.header().fin=true;
        segment.header().seqno=next_seqno();
        wait_ack.push(segment);
        send_segment(segment);
        return;
    }
    while(!_stream.buffer_empty()&&window_sizes>_next_seqno-rece_seqno){
        size_t size=min(TCPConfig::MAX_PAYLOAD_SIZE,static_cast<size_t>(window_sizes-(_next_seqno-rece_seqno)));
        
        segment.payload()=Buffer(_stream.read(min(size,_stream.buffer_size())));
        if(_stream.eof()&&segment.length_in_sequence_space()<window_sizes){
            is_fin=true;
            segment.header().fin=true;
        }
        if (segment.length_in_sequence_space() == 0)return;
        segment.header().seqno=next_seqno();
        wait_ack.push(segment);
        send_segment(segment);
    }


}
```



#### 2.send_segment()函数

这里就需要记录一下已经发送但还没有接收的字节有多少，并且开启计时器

```c++
void TCPSender::send_segment(TCPSegment &segment) {
 
    _segments_out.push(segment);
    bytes_in_flights+=segment.length_in_sequence_space();
    
    _next_seqno+=segment.length_in_sequence_space();
    //累计的，不是每次重启
    if(!re_time_.isstart()){
 
        re_time_.active();
        re_time_.restart();//reboot;
    }
}
```

#### 3.ack_received()函数

我们首先要判断一下接收到的ack是否是在蓝色的段中，也就是正在发送的范围内，并且记录接收器的窗口大小，重启计时器，并且判断是否需要关闭计时器。

```c++
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    DUMMY_CODE(ackno, window_size); 

    uint64_t ack_no=unwrap(ackno,_isn,_next_seqno);
    if(ack_no>_next_seqno){
        cerr<<receiver_window_size_<<endl;
        return;
    }
    receiver_window_size_=window_size;
   
    if(ack_no<=rece_seqno){
        return;
    }
    if(ack_no>rece_seqno){
        rece_seqno=ack_no;
        
    }

    pop_=false;
    re_time_.active(); 
    
    while (!wait_ack.empty())
    {   
        
        TCPSegment seg=wait_ack.front();

        if (ack_no <unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space()) {
           return;
        }

        wait_ack.pop();
        bytes_in_flights-=seg.length_in_sequence_space();
        pop_=true;

          
    }
    if(pop_){
        fill_window();
    }
    if(wait_ack.empty()){
        re_time_.close_();
    }


}
```

#### 4.tick()函数

这里有有个点要注意，我们重传的时候不用是用send函数，直接吧segment塞到队列即可，因为我们的send函数是累计有多少为确认的功能的。

```c++
void TCPSender::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
 
    if(!re_time_.tick_time(ms_since_last_tick)){
        return;
    }
    
    if(wait_ack.empty()){
        re_time_.close_();
        return;
    }
    
    TCPSegment seg=wait_ack.front();
    //这里不能掉send函数，因为不需要记录
    _segments_out.push(wait_ack.front());
    re_time_.double_time(receiver_window_size_,seg);
    re_time_.restart();


    
}
```

