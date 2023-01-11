#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <cmath>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {
    _retransmission_timeout = retx_timeout;
}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno; }

void TCPSender::fill_window() {
    TCPSegment seg;
    size_t current_windows_size = _windows_size > 0 ? _windows_size : 1;
    while (not _fin && current_windows_size - bytes_in_flight() > 0) {
        if (not _syn) {
            _syn = true;
            seg.header().syn = true;
            seg.header().seqno = _isn;
        }
        // 这里减去seg.length_in_sequence_space()是因为如果是SYN段，那么payload_size会为1
        // 导致携带数据，因此为了防止这种情况，选择多减去这一部分。
        // 换个方向想也可以是Tcp段中的SYN占了一个seqno或着说字节，因此payload_size也就相应的减少了
        size_t payload_size =
            min(current_windows_size - bytes_in_flight() - seg.length_in_sequence_space(), TCPConfig::MAX_PAYLOAD_SIZE);
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = _stream.read(payload_size);
        // 判断携带数据后是否还能发送Fin，(前提是字节流已经eof了)
        // 其中seg.length_in_sequence_space() + 1 <= current_windows_size - bytes_in_flight()
        // 是用来判断接收数据的窗口，是否还能接收这Fin的1字节
        if (not _fin && _stream.eof() &&
            seg.length_in_sequence_space() + 1 <= current_windows_size - bytes_in_flight()) {
            _fin = true;
            seg.header().fin = true;
        }
        // 如果segment中未携带数据或SYN/FIN，则跳出循环，并且该segment不必发出
        // 就是防止窗口还没发完，但是buffer里的数据没有了然后一直循环中
        // 调测试样例调出来的结果
        if (seg.length_in_sequence_space() > 0) {
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.emplace(seg);
        } else break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    DUMMY_CODE(ackno, window_size); 
    uint64_t temp_ackno = unwrap(ackno, _isn, _ackno);
    _windows_size = window_size;
    if (not (temp_ackno > _ackno && temp_ackno <= _next_seqno))
        return ;
    _ackno = temp_ackno;
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.emplace(seg);
}
