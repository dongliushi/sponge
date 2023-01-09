#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    if (not _syn && seg.header().syn == true) {
        _syn = true;
        _isn = seg.header().seqno;
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
    } else if (_syn) {
        uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _reassembler.first_unassembled());
        // abs_seqno 和字节流中的index的关系是abs_seqno - 1 = index，因为有个SYN
        _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    } else
        return;

    if (_reassembler.stream_out().input_ended()) { // FIN算一个字节
        _ackno = wrap(_reassembler.first_unassembled() + 2, _isn);
    } else {    // 因为有SYN，字节流里是从0开始，而abs_sqeno是从1开始，因此字节流中的index需要 + 1
        _ackno = wrap(_reassembler.first_unassembled() + 1, _isn);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn)
        return _ackno;
    return {};
}

size_t TCPReceiver::window_size() const { return _reassembler.first_unacceptable() - _reassembler.first_unassembled(); }
