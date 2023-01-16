#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    if (not _is_active)
        return;

    if (seg.header().rst) { // 收到RST，结束这个连接
        reset_connection(false);
        return;
    }

    _time_since_last_segment_received = 0;
    _receiver.segment_received(seg);

    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) &&
        seg.header().seqno == _receiver.ackno().value() - 1) {  // keep-alive
        _sender.send_empty_segment();
        send_data();
        return;
    }

    if (seg.header().ack) { // 收到ack需要传入_sender
        if (seg.header().ack > _sender.next_seqno_absolute()) // 如果ack不正常直接返回
            return;
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    bool need_send_ack = seg.length_in_sequence_space(); // segment不为空需要回复segment
    _sender.fill_window();

    if (need_send_ack && !_sender.segments_out().empty()) // 如果需要ack且已经有非空segment在队列中，则不需要再发送
        need_send_ack = false;
    
    if (need_send_ack) // 否则需要发送空segment
        _sender.send_empty_segment();
    
    send_data();

    if (!_sender.stream_in().eof() && _receiver.stream_out().input_ended()) {
        // 远端先关闭连接，进入close wait
        _linger_after_streams_finish = false;
    }
    if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
        if (!_linger_after_streams_finish) {
            // 本地发送完毕且不需要linger，则关闭连接
            _is_active = false;
        }
    }
}

bool TCPConnection::active() const { return _is_active; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_data();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (not _is_active)
        return;
    _sender.tick(ms_since_last_tick);
    // 超过重传次数，主动结束连接，发送RST包
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        reset_connection(true);
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;
    send_data();
    if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
        // 发送完毕后等待一段时间，自己结束连接
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _is_active = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_data();
}

void TCPConnection::connect() {
    _is_active = true;
    _sender.fill_window();
    send_data();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            reset_connection(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::send_data() {
    while (!_sender.segments_out().empty()) {
        TCPSegment &seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.emplace(seg);
    }
}

void TCPConnection::reset_connection(bool send_rst) {
    if (send_rst) {
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.emplace(seg);
    }
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _is_active = false;
}