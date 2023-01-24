#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.d

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
//123
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
