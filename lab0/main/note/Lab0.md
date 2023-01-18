# Lab0（networking warmup）



## 1. 前言

​		lab0是作为整个实验的热身实验，本实验让我体验到了计算机网络是如何交互的，通过第一个小实验感受到了TCP的建立过程，在第三个实验中亲自动手写了一个Socket来感受第一个小实验的建立过程，在第四个部分中自己写了一个网络内存更是让我了解到底层的原理。本次实验耗时大概有2，3天的时间，因为网络基础的问题，我是跟着中科大的计网B站课做的，本实验对应着我第一个单元的视频学习完成。.

​		本实验对我最大的感受就是让我深刻体会到客户端和服务器端的交互过程以及网络IO的使用，其中因为吸加加的基础不够完善，中途补了很多关于吸加加的内容以及关于网络socket的知识，重要的一点是一定要多阅读官方文档，以及理解实验想要让你做什么。希望我能坚持把这一套CS144 LAB做完吧！

实验要求：

​	•  使用 https://en.cppreference.com 上的语言文档作为资源。  

​	• 切勿使用 malloc() 或 free()。  

​	• 切勿使用新建或删除。 

​	• 基本上从不使用原始指针(*)，只有在必要时才使用“智能”指针（唯一指针或共享指针）。  

​	• 避免使用模板、线程、锁和虚函数。  （您不需要在 CS144 中使用它们。） 

​	• 避免使用 C 风格的字符串 (char *str) 或字符串函数（strlen()、strcpy()）这些很容易出错请改用 std::string。  

​	• 永远不要使用 C 风格的转换（例如，(FILE *)x）。 如果必须，请使用 C++ 静态转换。  

​	• 更喜欢通过 const 引用传递函数参数（例如：const Address & address）。 

​	• 将每个变量设为常量，除非它需要改变。  

​	• 将每个方法设为常量，除非它需要改变对象。  

​	• 避免使用全局变量，并为每个变量提供尽可能小的范围。  

​	• 在提交作业之前，请运行make format 规范编码风格。

## 2.手工搭建网络（networking warmup）

​	这一部分就按照指导书上一步一步来就好啦

​	在 Web 浏览器中，访问 http://cs144.keithw.org/hello 并观察结果。你会看到Hello CS144!

​	现在，您将手动执行与浏览器相同的操作。

1. 远程链接

```
telnet cs144.keithw.org http
```

	2. 这告诉服务器 URL 的路径部分

```
GET /hello HTTP/1.1
```

	3. 这告诉服务器 URL 的主机部分

```
Host: cs144.keithw.org
```

再按一次回车建立连接。

如果一切顺利，您将看到与您的浏览器看到的相同的响应，前面是告诉浏览器如何解释响应的 HTTP 标头。

此实验对应着第3个实验，由于我没有斯坦福的邮箱，所以邮件实验做不了



## 3.搭建虚拟服务器（Listening and connecting）

​	现在是时候尝试成为一个简单的服务器：等待客户端连接到它的那种程序。

1. 在一个终端窗口中，在您的 VM 上运行 

```
netcat -v -l -p 9090
```

	2. 保持 netcat 运行。 在另一个终端窗口中

```
telnet localhost 9090
```

​	如果一切顺利，netcat 将打印类似“收到来自本地主机 53500 的连接！这样你可以再两个终端上面进行交互聊天啦！



## 4 使用OS流套接字编写网络程序（OS stream socket）

​	您将利用Linux内核和大多数其他操作系统提供的功能：在两个程序（一个在您的计算机上运行，另一个在Internet上的不同计算机上运行）之间创建可靠的双向有序字节流的能力（例如，Web服务器，如Apache或nginx，或netcat程序）。此功能称为流套接字

​	对于您的程序和 Web 服务器，套接字看起来像一个普通的文件描述符（类似于磁盘上的文件，或者类似于 stdin 或 stdout I/O 流）。当连接两个流套接字时，写入一个套接字的任何字节最终将以相同的顺序从另一台计算机上的另一个套接字输出，套接字就相当于管道一样。

​	通常，连接两端操作系统的工作是将“尽力而为的数据报”（Internet 提供的抽象）转换为“可靠的字节流”（应用程序通常需要的抽象）。两台计算机必须合作以确保流中的每个字节最终都在其适当的位置上被传送到另一端的流套接字。

​	您将简单地使用操作系统预先存在的对传输控制协议的支持。您将编写一个名为“webget”的程序来创建一个 TCP 流套接字、连接到一个 Web 服务器并获取一个页面——就像您之前在本实验中所做的那样。在以后的实验中，您将实现这种抽象的另一面，通过自己实现传输控制协议来从不那么可靠的数据报中创建可靠的字节流

​	本实验主要需要仔细阅读4篇官方文档：

1. https://cs144.github.io/doc/lab0/class_file_descriptor.html（）

2. [Sponge: Socket Class Reference (cs144.github.io)](https://cs144.github.io/doc/lab0/class_socket.html)

3. [Sponge: TCPSocket Class Reference (cs144.github.io)](https://cs144.github.io/doc/lab0/class_t_c_p_socket.html)

4. [Sponge: Address Class Reference (cs144.github.io)](https://cs144.github.io/doc/lab0/class_address.html)

   主要对印着每个函数的介绍，当然啦，你也可以直接去 **libsponge/util ** 里面找到相应的底层函数，此实验你可以仿照文件的读写来实现。

```c++
void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.
    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).
    TCPSocket sock1;
    sock1.connect(Address(host,"http"));
    
    sock1.write("GET "+path+" HTTP/1.1\r\nHost: "+host+"\r\n\r\n");
    sock1.shutdown(SHUT_WR);
    
    while(!sock1.eof()){
        cout<<sock1.read();
    }
    sock1.close();
    

    cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    cerr << "Warning: get_URL() has not been implemented yet.\n";
}
```

这里我们首先要实现一个TCPSocket对象，我们从接收到的两个不可变的字符引用 **const string &host, const string &path** 中可以知道，我们需要连接这个地址，那么我们用Address类来实现，他的作用是将url转换成 **IPv4地址**，这里的**shutdown()**函数则是关闭这个套接字，让他不再具有读写功能，一定要记住，**read()** 函数要在循环里面，不然会出现无限循环哦，如果一切顺利，则会出现Hello CS144!



## 5.内存中的可靠字节流(memory reliable byte stream)

​	此实验则是让你去写一个读写内存，相当于一个buffer(缓冲区)，他的任务是你需要实现读写函数，分别对缓冲区进行修改，就是实现底层的内存逻辑。模拟单线程输入和输出，因此也不存在锁、并发等问题，是一个比较简单但是较为全面的小考验。

​	本实验需要让你再private中设计数据结构，我的思路是先阅读需要实现的函数，然后再去思考需要到的数据结构那么我们来分析一下每个函数。

1. **构造函数**

```c++
ByteStream(const size_t capacity):stream_len(capacity){};
```

参数为capacity，代表内存的大小，因此我们需要的参数就有一个要代表内存的大小，这里用**stream_len**表 示，内存是用来存储的，因此当然还要有一个变量来存储写入的字节流，这里用**stream**表示。而该构造函数的目的即是为了将内存的大小进行赋值。由于缓冲区是先进先出的，并且需要可以在前后都能操作，所以我们选择使用双向队列  *std::deque<**char**> stream={};*

2. **写函数**

```c++
size_t write(const std::string &data);
```

顾名思义我们需要接收字符串并且写入缓冲区，这里需要判断是否越界，

即**data_len**是否大于**stream_len-stream.size()**，如果大于的话则等于**stream_len-stream.size()**

然后需要记录写下的字节数，我们用hasWrite来表示

完整代码：

```c++
size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);

    size_t data_len=data.length();//how many byte to wirte
    //如果datalen+stream的内存大于能力的话，dadalen=capacity-stream的内存
    if(data_len>stream_len-stream.size()){
        data_len=stream_len-stream.size();
    }
    hasWrite+=data_len;
    for(size_t i=0;i<data_len;i++){
        stream.push_back(data[i]);
        
    }
    return data_len;
}
```



3. **查看下次输出函数**

```c++
std::string peek_output(const size_t len) const;
```

我们需要从缓冲区中读取长度为len的字节，并返回给一个字符串，此函数将会用于读函数，我们需要注意越界问题，和上一个函数的逻辑是一样的

完整代码：

```c++
string ByteStream::peek_output(const size_t len) const {
    //DUMMY_CODE(len);
    size_t stream_len_=len;
    std::string stream_peek;//what I want to read
    if (stream_len_>stream.size()){
        stream_len_=stream.size();
    }
    for(size_t i=0;i<stream_len_;i++){
        stream_peek.push_back(stream[i]);
        
    }
    //stream_peek=string().assign(stream.begin(),stream.begin()+stream_len_);
    return stream_peek;
}
```

4. **清除已读数据函数**

```c++
void ByteStream::pop_output(const size_t len){}
```

逻辑和上一个函数一样，我们需要将缓冲区的字节都删除，我们用到erase函数来实现，在这里还是需要注意缓冲区越界问题，

完整代码：

```c++
void ByteStream::pop_output(const size_t len) { 
    //DUMMY_CODE(len); 
    size_t stream_len_=len;

    if (stream_len_>stream.size()){
        stream_len_=stream.size();
    }
    hasRead+=stream_len_;
    
    for(size_t i=0;i<stream_len_;i++){
        stream.erase(stream.begin());
        
    }
}
```

5. **读函数**

```c++
std::string ByteStream::read(const size_t len) 
```

如果实现了上面两个函数，这个函数就变得相当简单了，只需要调用即可

```c++
std::string ByteStream::read(const size_t len) {
    //DUMMY_CODE(len);

    std::string output=peek_output(len);
    pop_output(len);
    return output;
}
```

6. **数据结构**

由此我们可以得出我们想要的数据结构有哪些，并且放在private里

```c++
  private:
    // Your code here -- add private members as necessary.

    
    std::deque<char> stream={};//stream is memory,
    size_t stream_len;//memory's cmakeapacity
    size_t hasWrite=0;//how many Byte has write
    size_t hasRead=0;
    bool is_end=false;
    bool empty =true;//is empty?
```

7. **函数完整代码**

```c++
#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):stream_len(capacity){ 
    //DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);

    size_t data_len=data.length();//how many byte to wirte
    //如果datalen+stream的内存大于能力的话，dadalen=capacity-stream的内存
    if(data_len>stream_len-stream.size()){
        data_len=stream_len-stream.size();
    }
    hasWrite+=data_len;
    for(size_t i=0;i<data_len;i++){
        stream.push_back(data[i]);
        
    }
    return data_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //DUMMY_CODE(len);
    size_t stream_len_=len;
    std::string stream_peek;//what I want to read
    if (stream_len_>stream.size()){
        stream_len_=stream.size();
    }
    for(size_t i=0;i<stream_len_;i++){
        stream_peek.push_back(stream[i]);
        
    }
    //stream_peek=string().assign(stream.begin(),stream.begin()+stream_len_);
    return stream_peek;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    //DUMMY_CODE(len); 
    size_t stream_len_=len;

    if (stream_len_>stream.size()){
        stream_len_=stream.size();
    }
    hasRead+=stream_len_;
    
    for(size_t i=0;i<stream_len_;i++){
        stream.erase(stream.begin());
        
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    //DUMMY_CODE(len);

    std::string output=peek_output(len);
    pop_output(len);
    return output;
}

void ByteStream::end_input() {is_end=true;}

bool ByteStream::input_ended() const { return is_end; }

size_t ByteStream::buffer_size() const { return stream.size(); }

bool ByteStream::buffer_empty() const { return stream.empty(); }//error point

bool ByteStream::eof() const { return buffer_empty()&&input_ended(); }

size_t ByteStream::bytes_written() const { return hasWrite; }

size_t ByteStream::bytes_read() const { return hasRead; }

size_t ByteStream::remaining_capacity() const { return stream_len-stream.size(); }

```

