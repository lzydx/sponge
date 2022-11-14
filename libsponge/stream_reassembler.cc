#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(_capacity + _curindex <= index) return;
    if(index + data.size() < _curindex) return;
    int startindex = _curindex > index ? _curindex - index : 0;
    if(eof) {
        _eofindex = index + data.size();
        _isvalideof = true;
    }
    string tobeprocess = data.substr(startindex, _capacity - _curindex);
    if(index == _curindex || startindex != 0) {
        _actualwrite += _output.write(tobeprocess);
        _curindex += tobeprocess.size();
        while(!_unorderstring.empty()){
            auto item = _unorderstring.begin();
            if(item->first > _curindex) break;
            _unassamblebytes -= item->second.size();
            string nextstring = item->second;
            _unorderstring.erase(item);
            // cout << nextstring<< "  " << _curindex<< "  " << item->first<< endl;
            if(_curindex - item->first < nextstring.size()){
                nextstring = nextstring.substr(_curindex - item->first);
                _actualwrite += _output.write(nextstring);
                _curindex += nextstring.size();
            }
        }
    }
    else if(index > _curindex) {
        // auto upper = _unorderstring.upper_bound(index);
        // if(upper == _unorderstring.begin()){
        //     tobeprocess = tobeprocess.substr(0, upper->first - index);
        // }
        // else{
        //     auto lower = prev(upper);
        //     int startindex = 
        // }
        // auto lower = prev(upper);



        _unassamblebytes += tobeprocess.size();
        _unorderstring[index] = tobeprocess;
    }
    if(_actualwrite == _eofindex && _isvalideof == true) {
        _output.end_input();
    }
    return ;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassamblebytes; }

bool StreamReassembler::empty() const { return {}; }
