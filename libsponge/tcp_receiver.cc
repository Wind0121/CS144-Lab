#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    bool ret = false;
    static size_t abs_seqno = 0;
    size_t length = 0;
    if(seg.header().syn){
        if(_syn_flag)
            return false;
        _syn_flag = true;
        ret = true;
        abs_seqno = 1;
        _base = 1;
        _isn = seg.header().seqno;
        length = seg.length_in_sequence_space() - 1;
        if(length == 0)
            return true;
    }else if(!_syn_flag){
        return false;
    }else{
        abs_seqno = unwrap(seg.header().seqno,_isn,abs_seqno);
        length = seg.length_in_sequence_space();
    }

    if(seg.header().fin){
        if(_fin_flag)
            return false;
        _fin_flag = true;
        ret = true;
    }

    if(seg.length_in_sequence_space() == 0 && abs_seqno == _base)
        return true;
    else if(abs_seqno >= _base + window_size() || abs_seqno + length <= _base)
        if(!ret)
            return false;

    _reassembler.push_substring(seg.payload().copy(),abs_seqno - 1,seg.header().fin);
    _base = _reassembler.head_index() + 1;
    if(_reassembler.input_ended())
        _base++;

    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_syn_flag)
        return wrap(_base,_isn);
    else
        return {};
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
