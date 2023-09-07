#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : size(capacity){}

size_t ByteStream::write(const string &data) {
    size_t i;
    for(i = 0;i < data.size() && buf.size() + 1 <= size;i++){
        buf.push_back(data[i]);
        write_cnt++;
    }
    return i;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    int l = min(len,buf.size());
    string res;
    int i = 0;
    for(auto pt = buf.begin();pt != buf.end() && i < l;i++,pt++){
        res.push_back(*pt);
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    int l = min(len,buf.size());
    for(int i = 0;i < l;i++){
        buf.pop_front();
        read_cnt++;
    }
}

void ByteStream::end_input() {is_end = true;}

bool ByteStream::input_ended() const { return is_end;}

size_t ByteStream::buffer_size() const { return buf.size();}

bool ByteStream::buffer_empty() const {return buf.size() == 0;}

bool ByteStream::eof() const { return buffer_empty() && input_ended();}

size_t ByteStream::bytes_written() const { return write_cnt; }

size_t ByteStream::bytes_read() const { return read_cnt; }

size_t ByteStream::remaining_capacity() const { return size - buf.size(); }
