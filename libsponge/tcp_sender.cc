#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{std::random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , timer(retx_timeout)
    , rto(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t sum = 0;
    for(auto pt = st.begin();pt != st.end();pt++)
        sum += pt->length_in_sequence_space();
    return sum;
}

void TCPSender::fill_window() {
    if(!is_syn_set){
        std::string tmp = "";
        TCPSegment segment = get_TCPSegment(tmp);
        st.push_back(segment);
        //更新_next_seqno
        _next_seqno += segment.length_in_sequence_space();
        //发送报文
        _segments_out.push(segment);
        return;
    }
    if(is_syn_set && _stream.buffer_empty() && !_stream.input_ended())
        return;
    if(is_fin_set)
        return;
    size_t _rwnd = rwnd ? rwnd : 1;
    size_t dataLen = min(_rwnd - bytes_in_flight(),TCPConfig::MAX_PAYLOAD_SIZE);
    //判断窗口还有多少
    if(dataLen <= 0)
        return;

    //通过函数得到TCPSegment
    std::string str = _stream.peek_output(dataLen);
    _stream.pop_output(dataLen);
    TCPSegment segment = get_TCPSegment(str);
    st.push_back(segment);

    //更新_next_seqno
    _next_seqno += segment.length_in_sequence_space();

    //发送报文
    _segments_out.push(segment);

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno,_isn,_next_seqno);
    //说明ackno无效
    if(abs_ackno > _next_seqno)
        return false;
    //说明ackno更新了，需要重置计时器
    if(abs_ackno > last_abs_ackno){
        last_abs_ackno = abs_ackno;
        rto = _initial_retransmission_timeout;
        resend_cnt = 0;
        timer.reset(rto);
    }

    std::list<TCPSegment> newst;
    for(auto pt = st.begin();pt != st.end();pt++){
        uint64_t leftno = unwrap(pt->header().seqno,_isn,_next_seqno);
        uint64_t rightno = leftno += pt->length_in_sequence_space();
        if(abs_ackno >= rightno)
            continue;
        newst.push_back(*pt);
    }
    st = newst;

    //如果打开了新空间（指窗口变大），TCPSender可能需要再次填充窗口。
    rwnd = window_size;
    if(rwnd - bytes_in_flight())
        fill_window();
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(st.empty()){//空的就关闭计时器
        timer.reset(rto);
        return;
    }
    timer.update_time(ms_since_last_tick);
    if(!timer.is_expired())
        return;
    TCPSegment resendSegment;
    uint64_t min_reqno = 0xffffffffffffffff;
    for(auto pt = st.begin();pt != st.end();pt++){
        if(unwrap(pt->header().seqno,_isn,_next_seqno) < min_reqno) {
            min_reqno = unwrap(pt->header().seqno, _isn, _next_seqno);
            resendSegment = *pt;
        }
    }
    resend_cnt += 1;
    rto *= 2;
    timer.reset(rto);
    _segments_out.push(resendSegment);
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return resend_cnt;
}

void TCPSender::send_empty_segment() {
    //生成一个序列空间长度为0的报文，只用填充头部
    WrappingInt32 seqno = wrap(_next_seqno,_isn);
    TCPHeader header;
    header.seqno = seqno;
    TCPSegment segment;
    segment.header() = header;

    //直接发送，不用记录
    _segments_out.push(segment);
}

TCPSegment TCPSender::get_TCPSegment(std::string& str){
    //设置header
    WrappingInt32 seqno = wrap(_next_seqno,_isn);
    TCPHeader header;
    header.seqno = seqno;
    if(!is_syn_set) header.syn = is_syn_set = true;
    if(!is_fin_set && _stream.input_ended() && str.length() < rwnd - bytes_in_flight()) header.fin = is_fin_set = true;

    //设置buffer
    Buffer payload(std::move(str));
    TCPSegment segment;
    segment.header() = header;
    segment.payload() = payload;

    return segment;
}
