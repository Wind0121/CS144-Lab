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
    insert(data,index);
    while(mp.count(_Index)){
        string tmp(1,mp[_Index]);
        _output.write(tmp);
        mp.erase(_Index);
        _Index++;
    }
    if(eof)
        eof_flag = true;
    if(eof_flag && mp.size() == 0)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const {return mp.size();}

bool StreamReassembler::empty() const { return mp.size() == 0;}

void StreamReassembler::insert(const std::string &data, const uint64_t index) {
    if(data.size() == 0) return;
    if(index + data.size() <= _Index) return;
    std::string newdata = data;
    uint64_t newindex = index;
    if(newindex < _Index) {
        newdata = newdata.substr(_Index - newindex);
        newindex = _Index;
    }
    for(uint64_t i = 0;i < newdata.size();i++)
        if(newindex + i < _Index + _capacity - _output.buffer_size())//落在接收窗口内
            mp[newindex + i] = newdata[i];
}


