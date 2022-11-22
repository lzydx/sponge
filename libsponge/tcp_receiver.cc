#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t absseq = 0;
    bool iseof = false;
    if(seg.header().syn) {
        if(_isrecvisn) return;
        _isrecvisn = true;
        _isn = seg.header().seqno;
        absseq = 1;
        _checkpoint = 0;
    }
    else{
        absseq = unwrap(seg.header().seqno, _isn, _checkpoint);
        _checkpoint = absseq;
    }
    if(!_isrecvisn) return;
    if(seg.header().fin){
        iseof = true;
    }

    _reassembler.push_substring(seg.payload().copy(), absseq - 1, iseof);
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_isrecvisn){
        // intput end need fin so +2
        if(_reassembler.stream_out().input_ended()) return wrap(_reassembler.stream_out().bytes_written() + 2, _isn);
        return wrap(_reassembler.stream_out().bytes_written() + 1, _isn);
    }
    return {};
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
