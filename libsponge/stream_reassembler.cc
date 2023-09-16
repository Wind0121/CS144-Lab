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
    //先写入缓存
    insert(data,index);
    while(!buf.empty()){
        char c = buf.front().first;
        uint64_t idx = buf.front().second;
//        cout << "idx: " << idx << " c: " << c << endl;
        if(idx != _Index)
            break;
        last_str += c;
        string tmp(1,c);
        _output.write(tmp);
        _Index = idx + 1;
        buf.pop_front();
    }

    if(eof)
        eof_flag = true;
    if(eof_flag && empty())
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return buf.size();}

bool StreamReassembler::empty() const { return buf.empty();}

void StreamReassembler::insert(const std::string &data, const uint64_t index) {
    //空串不用考虑
    if(data.size() == 0)
        return;
    //完全包含不用考虑了
    if(index + data.size() - 1 < _Index)
        return;
    std::string newdata = data;
    uint64_t newidx = index;
//    cout << "last_str:" << last_str << endl;
    //有重叠情况
    if(_Index && index <= _Index - 1){
        uint64_t p1 = index,p2 = 0;
        while(p1 < _Index && p2 < newdata.size() && last_str[p1] == newdata[p2])
            p1++,p2++;
        newdata = newdata.substr(p2,newdata.size() - p2);
        newidx += p2;
    }
//    cout << "newdata:" << newdata << endl;
    for(uint64_t i = 0;i < newdata.size();i++){
        uint64_t idx = newidx + i;
        //找到第一个大于等于当前idx的指针
        auto it = buf.begin();
        for(;it != buf.end();it++)
            if(it->second >= idx)
                break;
        if(it == buf.end()) {
//            cout << "idx:" << idx << endl;
            buf.push_back({newdata[i], idx});
        }
        else{
            //it就是第一个大于等于当前idx的指针
            if(it->second == idx)
                continue;
            buf.insert(it,{newdata[i],idx});
        }
    }
}
