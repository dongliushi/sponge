#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {  // absolute seqno 转成 seqno; isn是seqno的起始
    // DUMMY_CODE(n, isn);
    return isn + n;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // DUMMY_CODE(n, isn, checkpoint);
    uint32_t checkpoint_32 = wrap(checkpoint, isn).raw_value();  // 获取checkpoint在uint32下的映射。
    int32_t offset = n.raw_value() - checkpoint_32;  // 计算checkpoint映射和n的距离，用有符号表示，需要表示负数。
    if (offset < 0 && checkpoint + offset > checkpoint)      // 发生负溢出
        return checkpoint + static_cast<uint32_t>(offset);   // 等同于checkpoint + UINT32_MAX + 1 + offset
    if (offset > 0 && checkpoint + offset < checkpoint)      // 发生正溢出
        return checkpoint - static_cast<uint32_t>(-offset);  // 等同于checkpoint - UINT32_MAX - 1 + offset
    return checkpoint + offset;
}
