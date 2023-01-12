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
    size_t current_windows_size = _windows_size > 0 ? _windows_size : 1;
    while (not _fin && current_windows_size - bytes_in_flight() > 0) {
        TCPSegment seg;
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
        if (seg.length_in_sequence_space() == 0)
            break;
        // 接下来就是将nextseqno增加等处理
        _next_seqno += seg.length_in_sequence_space();
        _segments_track.emplace_back(seg);
        _segments_out.emplace(seg);
        // 如果没有启动定时器，则启动定时器
        if (not _is_timer) {
            _is_timer = true;
            _tick_ms = 0;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    uint64_t temp_ackno = unwrap(ackno, _isn, _ackno);
    _windows_size = window_size;
    // 判断ack是否为没收到过的，是收到过的则直接结束
    if (not(temp_ackno > _ackno && temp_ackno <= _next_seqno))
        return;
    _ackno = temp_ackno;
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    // 遍历_segments_track，将已经确认了的segment从中删除
    for (auto iter = _segments_track.begin(); iter != _segments_track.end();) {
        uint64_t abs_seqno = unwrap(iter->header().seqno, _isn, _ackno);
        if (abs_seqno + iter->length_in_sequence_space() <= _ackno) {
            iter = _segments_track.erase(iter);
        } else
            break;
    }
    if (_segments_track.empty()) {  // 当收到一个ACK并且所有包都被ACK了：关闭定时器
        _is_timer = false;
    } else {  // 当收到一个ACK并且仍有包未被ACK：重开定时器
        _is_timer = true;
        _tick_ms = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (not _is_timer)
        return;
    _tick_ms += ms_since_last_tick;
    if (_tick_ms >= _retransmission_timeout) {
        _tick_ms = 0;
        _segments_out.emplace(_segments_track.front());
        _consecutive_retransmissions++;
        if (_windows_size != 0)  // 窗口为0的时候不进行指数退避
            _retransmission_timeout *= 2;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.emplace(seg);
}
