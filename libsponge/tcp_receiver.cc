#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    bool ret = false;
    TCPHeader header = seg.header();
    uint64_t index;
    size_t seq_length = seg.length_in_sequence_space();
    size_t win_size;
    if(header.syn){
        if(is_syn_set)//重复设置
            return false;
        is_syn_set = true;
        ret = true;
        isn = header.seqno;
        index = 1;
        checkpoint = 1;
        abs_seq = 1;
        seq_length--;
        if(seq_length == 0)
            return true;
    }else if(!is_syn_set){
        return false;
    }else{
        index = unwrap(header.seqno,isn,checkpoint);
        checkpoint = index;
    }
    if(header.fin){
        if(!is_syn_set)
            return false;
        is_syn_set = false;
        seq_length--;
    }
    //设置接收窗口大小
    win_size = window_size();
    win_size = win_size ? win_size : 1;
    //设置序列长度
    seq_length = seq_length ? seq_length : 1;

    //判断是否在窗口内
    //序列：[index,index + seq_length)
    //窗口：[abs_seq,abs_seq + win_size)
    if(index >= abs_seq + win_size || abs_seq >= index + seq_length)
        return false;
    //推入序列
    _reassembler.push_substring(seg.payload().copy(),index - 1,header.fin);
    //更新abs_seq
    abs_seq = _reassembler.get_index() + 1;
    if(header.fin)
        abs_seq += 1;
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(is_syn_set)
        return wrap(abs_seq,isn);
    else
        return {};
}

size_t TCPReceiver::window_size() const {
    return _capacity - unassembled_bytes();
}
