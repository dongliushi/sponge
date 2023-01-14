#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    if (not _syn) {
        if (not seg.header().syn)
            return;
        _syn = true;
        _isn = seg.header().seqno;
    }
    // abs_seqno 和字节流中的index的关系是abs_seqno - 1 = index，因为有个SYN
    uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1;
    uint64_t curr_abs_seqno = unwrap(seg.header().seqno, _isn, abs_ackno);
    uint64_t stream_index = curr_abs_seqno - 1 + seg.header().syn;
    _reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn) {
        // 因为有SYN，字节流里是从0开始，而abs_sqeno是从1开始，因此字节流中的index需要 + 1
        uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1;
        if (_reassembler.stream_out().input_ended())  // FIN算一个字节
            ++abs_ackno;
        return wrap(abs_ackno, _isn);
    }
    return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
