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
    cout << _curindex << "  " << index << "  " << data.size() << "  " << _unassamblebytes << "  " << _unorderstring.begin()->first <<endl;
    if(eof){
        cout << "eof is " << index + data.size() << endl;;
    }
    if(index + data.size() <= _curindex) return;
    int startindex = _curindex > index ? _curindex - index : 0;
    if(eof) {
        _eofindex = index + data.size();
        _isvalideof = true;
    }
    string tobeprocess = data.substr(startindex);
    // cout << _curindex << "  "<< startindex << "  "<< _capacity - _curindex << "  "<< _curindex << "  "<< _curindex << endl;
    if(index == _curindex || startindex != 0) {
        size_t writebytes = 0;
        writebytes = _output.write(tobeprocess);
        _actualwrite += writebytes;
        _curindex += writebytes;
        while(!_unorderstring.empty()){
            auto item = _unorderstring.begin();
            if(item->first > _curindex) break;
            _unassamblebytes -= item->second.size();
            string nextstring = item->second;
            _unorderstring.erase(item);
            // cout << nextstring<< "  " << _curindex<< "  " << item->first<< endl;
            if(_curindex - item->first < nextstring.size()){
                nextstring = nextstring.substr(_curindex - item->first);
                writebytes = _output.write(nextstring);
                _actualwrite += writebytes;
                _curindex += writebytes;
            }
        }
    }
    else if(index > _curindex) {
        size_t realindex = index;
        // find <----  if overlap
        auto upperitem = _unorderstring.upper_bound(index);
        if(upperitem != _unorderstring.begin()){
            upperitem = prev(upperitem);
            if(index < upperitem->first + upperitem->second.size()){ // left point overlao
                realindex = upperitem->first + upperitem->second.size();
                if(realindex >= index + tobeprocess.size()) return; // all overlapped
                tobeprocess = tobeprocess.substr(realindex - index);
            }
        }
        // find ----> if overlap
        upperitem = _unorderstring.upper_bound(index);
        while(upperitem != _unorderstring.end() && upperitem->first < realindex + tobeprocess.size()){
            if(upperitem->first + upperitem->second.size() <= realindex + tobeprocess.size()){ // all overlapped
                auto temp = upperitem;
                upperitem = next(upperitem);
                _unassamblebytes -= temp->second.size();
                _unorderstring.erase(temp);
            }
            else{
                _unassamblebytes -= realindex + tobeprocess.size() - upperitem->first;
                upperitem->second = upperitem->second.substr(realindex + tobeprocess.size() - upperitem->first);
            }
        }
        _unorderstring[realindex] = tobeprocess.substr(0, _capacity - _unassamblebytes);
        _unassamblebytes += tobeprocess.size();
        _unassamblebytes = min(_capacity, _unassamblebytes);
    }
    if(_actualwrite == _eofindex && _isvalideof == true) {
        _output.end_input();
    }
    return ;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassamblebytes; }

bool StreamReassembler::empty() const { return {}; }
