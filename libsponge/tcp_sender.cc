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
    , _timer(retx_timeout) { }

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // "CLOSED" “waiting for stream to begin (no SYN sent)” ->sent syn
    if(_next_seqno == 0) {
        _synsend = true;
        TCPSegment seg;
        seg.header().syn = true;
        send_nonempty_segment(seg);
        return;
    }
    // "SYN_SENT" "tream started but nothing acknowledged"  -> wait ack
    if(_next_seqno > 0 && _next_seqno == _bytes_in_flight) {
        return;
    }

    // FIN_SENT “stream finished (FIN sent) but not fully acknowledged -> wait fin ack
    if(_stream.eof() && _next_seqno == _stream.bytes_written() + 2 && _bytes_in_flight > 0){
        return;
    }

    // FIN_ACKED stream finished and fully acknowledged
    if(_stream.eof() && _next_seqno == _stream.bytes_written() + 2 && _bytes_in_flight == 0){
        _timer.close();
        return;
    }

    uint16_t window_size = _window_size ? _window_size : 1; // window smallest is 1

    uint64_t remaining;
    while((remaining = window_size - (_next_seqno - _recv_ackno))) {
        uint64_t len = remaining > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE : remaining;
        TCPSegment seg;
        if(_stream.eof() && !_finsend) {
            _finsend = true;
            seg.header().fin = true;
            send_nonempty_segment(seg);
            return;
        }
        seg.payload() = _stream.read(len);
        if(_stream.eof() && (remaining - seg.length_in_sequence_space() > 0) && !_finsend) { // pick with fin
            _finsend = true;
            seg.header().fin = true;
        }
        if(seg.length_in_sequence_space() == 0) {
            return;
        }
        send_nonempty_segment(seg);
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    if(abs_ackno > _next_seqno) return ; // ack not send seg
    if(_recv_ackno > abs_ackno) return ; // ack already acked-seg 
    
    // e.g _recvackno == absackno dont refresh timer
    if(_recv_ackno != abs_ackno)
        _timer.start(); // recv "new ack" restart timer
    _recv_ackno = abs_ackno;
    _window_size = window_size;
    
    

    while(!_segments_tracking.empty()) {
        const TCPSegment &tempseg = _segments_tracking.front();
        if(static_cast<int32_t>(tempseg.length_in_sequence_space()) <= ackno - tempseg.header().seqno) { // the last byte of send seg is seqno + data.size - 1
            _bytes_in_flight -= tempseg.length_in_sequence_space();
            _segments_tracking.pop();
        }
        else break;
    }

    fill_window();

    if(_segments_tracking.empty()) {
        _timer.close();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if(!_synsend || !_timer.isrunning()) return;
    if(_segments_tracking.empty()) {
        _timer.close();
        return ;
    }
    if(_timer.istimeout(ms_since_last_tick)) {
        auto timeoutseg = _segments_tracking.front();
        _segments_out.push(timeoutseg);
        _timer.restart(_window_size);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return this->_timer.get_consecutive_retransmissions(); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_nonempty_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    _segments_tracking.push(seg);
    _segments_out.push(seg);

    if(!_timer.isrunning()) {
        _timer.start();
    }
}
