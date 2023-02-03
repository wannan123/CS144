#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

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
