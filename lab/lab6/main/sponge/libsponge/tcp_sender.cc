#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , re_time_(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return {bytes_in_flights}; }

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

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    DUMMY_CODE(ackno, window_size); 

    uint64_t ack_no=unwrap(ackno,_isn,_next_seqno);
    if(ack_no>_next_seqno){
        
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
    re_time_.restart();
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

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
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

unsigned int TCPSender::consecutive_retransmissions() const { 
    return re_time_.count_time;
}

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment;
    tcp_segment.header().seqno = wrap(_next_seqno,_isn);
    _segments_out.push(tcp_segment);
}


