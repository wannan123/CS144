# Lab2（the TCP receiver）



OK！完成Lab2啦，本次实验用时3天，在这一周内补完了第三章传输层的理论知识，本次实验主要是实现了一个TCP接收器，它结合了前两次实验：ByteStream和重组器，对它两进行一个上层分装，我们还是来看看总体图：

![image-20230124214052262](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230124214052262.png)

我们可以看到Receiver是在重组器之上的，所以我们这个实验的总体思路是Reciever接收到乱序的段然后塞给重组器最后写入内存中，本次实验还是有点难度，值得思考的（对于我这个小白来说~）那么我们开启Lab2的欢快之旅吧！



## 1. 64位索引和32位索引的转化（Translating）



我们先上TCP段图:

![image-20230204000755747](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230204000755747.png)

可以看到第二行是序号（Sequno），代表着这一段的序号，我们在Lab1中有用到每一次发来字符段的索引，那个索引就是用序号转化过来的，当然，我们不可能直接将seqno拿过来当index，因为在TCP生成序号的时候，初始序号（SYN）是32位的一个随机数，这样保证了TCP接收多个sender传来的segment的时候造成的混乱，所以给每个sender都随机生成一个数字。但是我们不能直接拿这个随机数来当索引，要不然太麻烦了，所以我们想把它转化一下，也就是所谓的标准化，这样方便重组。

因为32位序号包含的范围不是很大，所以TCP的序号它采用一种环形的序号组，也就是如果越界的时候就从头开始。但是我们的index可不想这样做，所以我们采用64位编码方式，理解了这个我们看看它的大致样子：

![image-20230204001616541](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230204001616541.png)

我们接收到的是seqno，我们需要将它转化为absolute seqno，因为index只需要加一，所以不需要管。我们需要实现两个函数： **wrap** 和 **unwrap** 



* **wrap() ：**      

    Convert absolute seqno → seqno，这个相当比较轻松，我们只需要将absolute seqno加上seqno的初始序号（SYN）再对对2的32次方取余，即可得到seqno，因为seqno是环形的，所以取余即可

    ```c++
    WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
        DUMMY_CODE(n, isn);
        uint64_t t=isn.raw_value()+n;
        uint32_t seqno=t % (1ul << 32);
        //uint32_t temp=static_cast<uint32_t>(t);
        return WrappingInt32{seqno};
    }
    ```

    

* **unwrap() ：** 

    Convert seqno → absolute seqno，这个就相对难去想了，首先，seqno是环形的，它一个值可能会对应多个abs seq 例如。  ISN 为零时，seqno “17” 对应于 17 的绝对 seqno，还有 2^32 + 17，或 2^33 + 17，或 2^34 + 17，等等，所以我们这里需要引入一个checkpoint的值，这个checkpoint相当于Receiver的ACKno也就是下一个希望接收到的序号，这样也就让我们知道到底对应哪一个值了，其实想理解这个函数，我觉得不需要特别复杂的数学算法，我们只需要拿出我们的小本本，随手画一画就能找到规律啦！

    ![image-20230204013815242](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230204013815242.png)

    主题思路是我们把checkpoint转化为seqno的然后看看它与n的offset，然后再转回来加上这个offset就搞定了。因为我们这里用的是int_64来存abs_n，它的范围为[-2^32,2^32-1]所以如果超过2的32次方，则会变为负数，所以我们需要加上2^32。

    ```c++
    uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
        DUMMY_CODE(n, isn, checkpoint);
        WrappingInt32 s_check=wrap(checkpoint,isn);
        int32_t offset=n-s_check;
        int64_t ret=checkpoint+offset;
        if(ret<0){
            return ret+(1ul << 32);
        }else{
            return ret;
        }
    ```

## 2. 实现TCPReceiver



实现这个部分还是需要一点时间的，我花了基本上一天的时间去阅读tcp_helpers里面的 tcp_segment 和tcp_header 两个实例和官方文档，我觉得这个工作还是很有必要的，segment包装了header，header是格外重要的，我们看到最上面那个tcp segment 图，payload以上的部分为head，其实阅读源码就可以更加理解校验和是个怎样工作的，我们可以看这个这段代码：

```c++
    const uint8_t fl_b = p.u8();                  // byte including flags
    urg = static_cast<bool>(fl_b & 0b0010'0000);  // binary literals and ' digit separator since C++14!!!
    ack = static_cast<bool>(fl_b & 0b0001'0000);
    psh = static_cast<bool>(fl_b & 0b0000'1000);
    rst = static_cast<bool>(fl_b & 0b0000'0100);
    syn = static_cast<bool>(fl_b & 0b0000'0010);
    fin = static_cast<bool>(fl_b & 0b0000'0001);
```

其实就是原码取反然后相加最后为0FFFFFFFFF的一个过程，这里可以去补一下USTC的这块讲解，我记得实在UDP那里讲过，感觉原理差不多。我们之后需要tcp_segment里的head来进行编写。

我们需要实现四个函数，我们一个一个攻破！

* **void segment_received(const TCPSegment &seg);**

    这个函数是这一部分的难点，你需要判断到来的序号是否为SYN，如果是第一次出现SYN你需要记录一下，然后在此之后出现SYN的话就说明是多余的，所以直接return即可，这里其实埋下了一个坑点，我也是查看网上博客才知道的，这里SYN和FIN都是带有数据包的，所以不能直接返回，不然是过不了测试的。所以我们需要对第一次到来的SYN也要进行处理（给reassembler），而且你不能遇到FIN就返回（这也是我出现错误的点，足足让我debug好长时间...）另一个点在于checkpoint的选取，你不能直接选ackno，因为这个段是乱序的，有可能给你发一个很后的段，所以我们采用写了多少个段来解决，最后写入reassembler就行。

    ```c++
    void TCPReceiver::segment_received(const TCPSegment &seg) {
        DUMMY_CODE(seg);
        if(!is_syn && !seg.header().syn){return;}
        if(is_syn && seg.header().syn){return;}
        //if(is_fin){return;}//不要返回，万一提前遇到fin就寄了。
        if(seg.header().fin){
            is_fin=true;
        }
        if(seg.header().syn && !is_syn){
            syn=seg.header().seqno;
            is_syn=true;
            //return;//这是个坑，syn其实也有数据
        }
        //这里一定要用written,seg的ackno有可能是在真正checkpoint前后的。
        uint64_t checkpoint=_reassembler.stream_out().bytes_written()+1;
        uint64_t abs_seg=unwrap(seg.header().seqno,syn,checkpoint);
    
        uint64_t _index = abs_seg - 1 + static_cast<uint64_t>(seg.header().syn);
        string segment=seg.payload().copy();
        _reassembler.push_substring(segment,_index,seg.header().fin);
        
    }
    ```

* **std::optional<WrappingInt32> ackno() const;**

    这里就相对简单了，这里的ackno也是采取用written来解决，值得注意的是，SYN和FIN是带有数据的，别忘了就好~

    ```c++
    optional<WrappingInt32> TCPReceiver::ackno() const { 
        if(!is_syn){
            return{};
        }
        uint64_t has_written=_reassembler.stream_out().bytes_written()+1;
        if (stream_out().input_ended()){
            return wrap(has_written+1,syn);
        }
        return wrap(has_written,syn);
    
    }
    ```

* **size_t TCPReceiver::window_size() const**

    ```c++
    size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
    ```

当我们思考完函数后其实我们就知道我们还需要再private里添加几个值，这也是我一直的思考问题的方式

```c++
class TCPReceiver {
    //! Our data structure for re-assembling bytes.
    

    StreamReassembler _reassembler;
    //! The maximum number of bytes we'll store.
    size_t _capacity;
    WrappingInt32 syn{0};
    bool is_syn=false;
    bool is_fin=false;
```

