#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

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


size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
