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
    // lab1文档中所提到的三个坐标，其中first_unread未被用到
    // 每一个部分都是 [ , ) 左闭右开
    size_t first_unassembled = _output.bytes_written();
    size_t first_unacceptable = _output.bytes_read() + _capacity;
    size_t data_len = data.size();
    if (not _eof && eof) {
        _eof = eof;
        _eof_index = index + data_len;
    }
    // 数据正常到来
    if (index <= first_unassembled && index + data_len > first_unassembled) {
        // 实际上在这里如果到来的字节数大于capacity，那么对应的处理会将超出的字节舍去，再写入管道
        // 而不是直接舍弃整个数据(说实话，我感觉应该全部舍弃的，但是全部舍弃过不了测试)
        _output.write(data.substr(first_unassembled - index, first_unacceptable - first_unassembled));
    } else if (index > first_unassembled && index < first_unacceptable) {  // 数据乱序到来
        // 截取数据，超过缓冲区舍弃（但是为啥不是全部舍弃呢，晕晕）
        _buffer.push_string(data.substr(0, first_unacceptable - index), index);
    }
    _buffer.reassemble(_output); // 重组缓冲区里信息
    if (_eof && _output.bytes_written() == _eof_index) {
        _buffer.clear();
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer.nbytes(); }

bool StreamReassembler::empty() const { return _buffer.empty() && _output.buffer_empty(); }
