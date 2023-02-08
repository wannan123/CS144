#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"
#include <iostream>
#include <functional>
#include <queue>
using namespace std;
//time有4个功能：启动，关闭，重启，判断超时；
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

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};
    uint64_t rece_seqno{0};

    uint64_t bytes_in_flights{0};
    uint64_t receiver_window_size_ = 1;
    std::queue<TCPSegment> wait_ack{};
    bool is_syn=false;
    bool is_fin=false;
    bool pop_=false;
    time_ re_time_;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    
    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();
    
    void send_empty_segment();


    void send_segment(TCPSegment &segment);
    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
    
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
