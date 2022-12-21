#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _time_since_last_segment_received = 0;
    if(!_active) return;
    
    if(seg.header().rst) {
        set_rst();
        return;
    }

    _receiver.segment_received(seg);

    bool right_ack = seg.header().ack ? _sender.ack_received(seg.header().ackno, seg.header().win) : false;
    
    if(seg.header().syn && !_established) {
        if(right_ack) {
            _established = true;
        }
        else {
            _sender.fill_window();
        }
    }
    else if(!_established && right_ack) {
        // syn_receive -> establiashed
        _established = true;
    }
    if(seg.length_in_sequence_space() != 0 || !_sender.segments_out().empty()) {
        fill_outqueue();
    }
    test_end();

}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t writenum = _sender.stream_in().write(data);
    _sender.fill_window();
    fill_outqueue();
    test_end();
    return writenum;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _time_since_last_segment_received += ms_since_last_tick;
    if(_sender.tick(ms_since_last_tick)) {
        if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            set_rst();
            return;
        }
        fill_outqueue();
    }
    test_end();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    fill_outqueue();
    test_end();
}

void TCPConnection::connect() {
    if(_sender.next_seqno_absolute() != 0) return;
    _sender.fill_window();
    fill_outqueue();
    _active = true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            set_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}


void TCPConnection::set_rst() {
    _active = false;
    _rst = true;
    _linger_after_streams_finish = false;
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    if(_established) {
        fill_outqueue();
    }
}

void TCPConnection::test_end() {
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof() && _sender.next_seqno_absolute() > 0) {
        _linger_after_streams_finish = false;
    }
    else if(_receiver.stream_out().eof() && _sender.stream_in().eof() && unassembled_bytes() == 0 
            && bytes_in_flight() == 0 && _sender.fin_send()) {
        if(!_linger_after_streams_finish) _active = false;
        else if(_time_since_last_segment_received >= 10 * _cfg.rt_timeout) _active = false;
    } 
}

void TCPConnection::fill_outqueue() {
    auto &stream_queue = _sender.segments_out();
    if(stream_queue.empty()) {
        _sender.send_empty_segment();
    }

    if(_rst) {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }

        seg.header().rst = _rst;
        size_t winsize = _receiver.window_size();
        seg.header().win = winsize < std::numeric_limits<uint16_t>::max() ? static_cast<uint16_t>(winsize)
                                                                          : std::numeric_limits<uint16_t>::max();
        _segments_out.push(seg);
    }

    while(!stream_queue.empty()) {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }

        seg.header().rst = _rst;
        size_t winsize = _receiver.window_size();
        seg.header().win = winsize < std::numeric_limits<uint16_t>::max() ? static_cast<uint16_t>(winsize)
                                                                          : std::numeric_limits<uint16_t>::max();
        _segments_out.push(seg);
    }
    return;
}

bool TCPConnection::in_syn_sent() {
    return _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == _sender.bytes_in_flight();
}

bool TCPConnection::in_listen() { return !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0; }