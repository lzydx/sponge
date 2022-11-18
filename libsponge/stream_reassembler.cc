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
    if(index + data.size() < _curindex) return;
    if(eof) {
        _eofindex = index + data.size();
        _isvalideof = true;
    }

    string tobeprocess = data;

    // find if overlapped
    size_t realindex = index;

    // find <----  if overlap
    auto upperitem = _unorderstring.upper_bound(index);
    if(upperitem != _unorderstring.begin()){
        auto loweritem = prev(upperitem);
        if(index <= loweritem->first + loweritem->second.size()){ // left point overlao
            if(loweritem->first + loweritem->second.size() >= index + tobeprocess.size()) return; // all overlapped
            tobeprocess = loweritem->second.substr(0, index - loweritem->first) + tobeprocess;
            realindex = loweritem->first;
            _unassamblebytes -= loweritem->second.size();
            _unorderstring.erase(loweritem);
        }
    }

    // find ----> if overlap
    while(upperitem != _unorderstring.end() && upperitem->first <= realindex + tobeprocess.size()){
        if(upperitem->first + upperitem->second.size() <= realindex + tobeprocess.size()){ // all overlapped
            auto temp = upperitem;
            upperitem = next(upperitem);
            _unassamblebytes -= temp->second.size();
            _unorderstring.erase(temp);
        }
        else{
            _unassamblebytes -= upperitem->second.size();
            tobeprocess = tobeprocess + upperitem->second.substr(realindex + tobeprocess.size() - upperitem->first);
            _unorderstring.erase(upperitem);
            break;
        }
    }


    if(realindex == _curindex){
        size_t writebytes = 0;
        writebytes = _output.write(tobeprocess);
        _actualwrite += writebytes;
        _curindex += writebytes;
    }
    else if(realindex < _curindex){
        if(realindex + tobeprocess.size() <= _curindex) return; // all had been writed
        tobeprocess = tobeprocess.substr(_curindex - realindex);
        size_t writebytes = 0;
        writebytes = _output.write(tobeprocess);
        _actualwrite += writebytes;
        _curindex += writebytes;
    }
    else{
        _unorderstring[realindex] = tobeprocess.substr(0, min(_capacity - _unassamblebytes, tobeprocess.size()));
        _unassamblebytes += tobeprocess.size();
        _unassamblebytes = min(_unassamblebytes, _capacity);
    }

    if(_actualwrite == _eofindex && _isvalideof == true) {
        _output.end_input();
    }
    // cout << _curindex << "  " << index << "  " << data.size() << "  " << _unassamblebytes << "  " << _unorderstring.begin()->first <<endl;
    // cout << "map is" <<endl;
    // for(auto item = _unorderstring.begin(); item != _unorderstring.end(); item++){
    //     cout << item-> first << " " << item->first + item->second.size()<< endl;
    // }
    return ;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassamblebytes; }

bool StreamReassembler::empty() const { return {}; }
