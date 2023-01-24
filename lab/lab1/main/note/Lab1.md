# Lab1（stitch substrings into a byte stream）

​		我们知道在TCP传输时当它传入网络核心时由于多主机同时发送的原因，有可能在一个交换机节点中形成一个队列，它会将接收到的信息打乱然后重新发送，由此在TCP接收器内部其实时接收到一堆无序的段，只有将他们拼接后才能读。本次实验是将一段段无序的字符串拼接成一个有序的字符串我们需要实现一个StreamReassembler对象类，他就是将一堆乱序的字符段重新组合成有序的字符流写入网络内存中，也就是我们上一期实验z中实现的的ByteStream,让我们先来了解一下我们前五个实验需要干什么

![image-20230124214052262](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230124214052262.png)

由上图可以知道，Socket相当于一个传送门，你可以把它想象成哆啦A梦里的那个传送门，你从这里发送一条请求，在另一处就会就受到，理解上图，我们本次实验需要做StreamReassembler这个部分，相当于接收到网络传送来的无序的Segment，然后重新组装一下，发送给TCPReceive模块。



## 1.Get start !	

​		本次实验，历经1周的时间，期间也是更着中科大的B站课程把第二单元看完了，实验用时2天的时间，由于第一次实验的经历，本次实验也会顺畅一点，本次实验我更加的熟悉了GIT的操作，以及向小伙伴请教学会了如何调试代码，其实也就是去运行tests里的可执行文件然后看看输出，期间我也是遇到了很多的坑，接下来我来慢慢给大家讲述~

理解本次实验的关键是理解下图：
![image-20230124215052900](https://github.com/wannan123/CS144/blob/main/blob/main/lab0/main/note/picture/image-20230124215052900.png)



期间我们主要是完成图中 capacity 的部分，我们可以理解绿色的段为已经排好序的部分，红色部分为新来的未排序的部分，蓝色的部分为已经写入的（这部分不属于我们操心的范围），它的箭头方向也是说明了index的大小升高趋势，first unacceptable往右是多余的部分，我们需要删除它。指导书中有一个小提示是：如何处理不一致的子字符串？你可以假设它们不存在。也就是说，你可以假设你有一段已经排序号整理好的stream，然后不断地填充，理解这个很重要，我们根据lab0我们在此处也需要用deque来定义一个缓冲队列，这个队列就相当于你假设的排序好的，然后再定义一个bool类型的队列来指明这个队列中拿些字符串已经排序号。如果你还是不能理解，你可以想象它为一个没坐满的火车，然后每次都有人来上火车，直到火车坐满为止~

根据test的内容来看，其实这个模块的调用流程是：创建StreamReassembler-->传来一段字符串-->再传另一段字符串-->最后写入内存中。



## 3.数据结构

​		经过上面的分析我们可以得到private的内容：

```c++
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    size_t unassembled_byte;
    size_t emptyn;
    size_t eof_index=0;
    bool is_eof=false;

    bool empty_;
    std::deque<bool> flag;
    std::deque<char> Stream_Reassemble;
```

这里的empty_和emptyn其实有点多余，我们可以忽略不记



## 4.实现算法



​		我们首先要定义边界，也就是上图中first_read和first_unaccept我们用这两个相当于我们的索引边界，我们为什么不定义unassemble呢？因为它其实是不断动态变化的，所以我们没必要管它，哦对了，其实整个first read到first unaccept其实也是动态变动的，你可以把它想象为一个管道不断地向前移，因为我们其实利用了双向队列的pop和emplace_back方法来实现填充的，理解这一点对下面的代码很重要。

​		1、首先来定义边界：

```c++
	size_t first_read=_output.bytes_written();
    size_t first_unaccept=first_read+_capacity;
```

​		然后我们需要如果碰到不合适的范围直接返回：

```c++
	if(index>=first_unaccept||index+data.length()<first_read){return;}
```

​		2、接着我们需要去裁剪一下范围，就是裁去多余的部分：

```c++
 	size_t index_=index;
    size_t len=data.length();
    if(index_<first_read)index_=first_read;
    if(index+len>=first_unaccept)len=first_unaccept-index;
```

你可以把index理解为初始下表，index+data.length()为最终下表。

​		3、接着我们就可以进行填充啦：

```c++
    for(size_t i=index_;i<index+len;i++){
        //cerr<<"i:"<<i<<endl;
        if(!flag[i-first_read]){
            emptyn++;
            unassembled_byte++;
            Stream_Reassemble[i-first_read]=data[i-index];
            flag[i-first_read]=true;//加空字符没事;

        }
    }
```

这里我们需要

​		4、我们需要填入绿色的部分：

```c++
    //开始写入
    std::string buffer="";
    //cerr<<"flag"<<flag.front()<<endl;
    while(flag.front()){
        buffer+=Stream_Reassemble.front();
        Stream_Reassemble.pop_front();
        flag.pop_front();
        flag.emplace_back(false);
        Stream_Reassemble.emplace_back('\0');
    }

```

我们可以在大脑中想象，如果一列火车(从小放大来看）只来了1，3，4车厢，我们这里在填入的时候只会填入第一列，因为第二列的bool值为0所以只能等下次2来，所以这样就保证了顺序的进行~别忘了在删除第一个后还要去emplace back一下。

​		5、写入：

```c++
    if(buffer.length()>0){
        stream_out().write(buffer);
        unassembled_byte-=buffer.length();
        emptyn-=buffer.length();
    }
```

​		6、最后判断eof:

```c++
    if(eof){
        eof_index=index+len;
        is_eof=true;
    }
    // cerr<<"is_eof:"<<is_eof<<endl;
    if(eof_index==_output.bytes_written()&&is_eof){
        _output.end_input();
        
    }
```

这里可把我坑惨了，由于我一直没理解它eof的意义，所以一直卡着没过去，后来cerr之后才知道，我们需要在对象里声明一个 is_eof=false的bool值，来记录这此发来的字符串是否为eof结尾，所以我们需要有一个变量来记录一下，因为他在不断的传来segment时它的eof是会变的。

最后上完整代码：

```c++
StreamReassembler::StreamReassembler(const size_t capacity) : 
    
    _output(capacity), 
    _capacity(capacity),
    unassembled_byte(0),
    emptyn(0),
    empty_(true),
    flag(capacity,false),
    Stream_Reassemble(capacity,'\0'){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);
    //先定义边界：first_read=写入的，first_unaccept=first_read+cap
    size_t first_read=_output.bytes_written();
    size_t first_unaccept=first_read+_capacity;
    if(index>=first_unaccept||index+data.length()<first_read){return;}
    //delete
    size_t index_=index;
    size_t len=data.length();
    if(index_<first_read)index_=first_read;
    if(index+len>=first_unaccept)len=first_unaccept-index;
    //把没有填过的填入Reasssable,start=index_,end=index_+len;
    //cerr<<"INDEX:"<<index_<<"end:"<<index_+len<<endl;
    for(size_t i=index_;i<index+len;i++){
        //cerr<<"i:"<<i<<endl;
        if(!flag[i-first_read]){
            emptyn++;
            unassembled_byte++;
            Stream_Reassemble[i-first_read]=data[i-index];
            flag[i-first_read]=true;//架空字符没事;

        }
    }
    //开始写入
    std::string buffer="";
    //cerr<<"flag"<<flag.front()<<endl;
    while(flag.front()){
        
        
        buffer+=Stream_Reassemble.front();
        Stream_Reassemble.pop_front();
        flag.pop_front();
        flag.emplace_back(false);
        Stream_Reassemble.emplace_back('\0');
    }

    //cerr<<"HEE:"<<buffer<<"HH:"<<data<<endl;
    if(buffer.length()>0){
        stream_out().write(buffer);
        unassembled_byte-=buffer.length();
        emptyn-=buffer.length();
    }

    //
    //cerr<<index+len<<endl;


    // if(eof&&index+len==_output.bytes_written()){
    //     _output.end_input();
    // }

    if(eof){
        eof_index=index+len;
        is_eof=true;
    }
    // cerr<<"is_eof:"<<is_eof<<endl;
    if(eof_index==_output.bytes_written()&&is_eof){
        _output.end_input();
        
    }
    if(emptyn==0){
        empty_=true;
    }
}

size_t StreamReassembler::unassembled_bytes() const { return {unassembled_byte}; }

bool StreamReassembler::empty() const {return unassembled_bytes()==0; }
```

