#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):m_capacity(capacity) { }

size_t ByteStream::write(const string &data) {
    size_t remainlen = remaining_capacity();
    if(remainlen == 0 || data.size() == 0) return 0;
    size_t writelen = (remainlen >= data.length()) ? data.length() : remainlen;
    m_data += data.substr(0, writelen);
    m_writednum += writelen;
    return writelen;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return m_data.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    m_readnum += len;
    m_data.erase(m_data.begin(), m_data.begin() + len);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t readnum = len > m_data.size() ? m_data.size() : len;
    string result = peek_output(readnum);
    pop_output(readnum);
    return result;
}

void ByteStream::end_input() {m_endinput = true;}

bool ByteStream::input_ended() const { return m_endinput; }

size_t ByteStream::buffer_size() const { return m_data.size(); }

bool ByteStream::buffer_empty() const { 
    return m_data.empty(); }

bool ByteStream::eof() const { return m_endinput && (m_data.size() == 0); }

size_t ByteStream::bytes_written() const { return m_writednum; }

size_t ByteStream::bytes_read() const { return m_readnum; }

size_t ByteStream::remaining_capacity() const { return m_capacity - m_data.size(); }
