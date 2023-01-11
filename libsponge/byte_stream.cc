#include "byte_stream.hh"

#include <cmath>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {
    // DUMMY_CODE(capacity);
    _buffer.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    size_t old_w = _writen;
    for (auto &ch : data) {
        if (_writen - _readn < _capacity) {
            _buffer[_writen % _capacity] = ch;
        } else
            break;
        ++_writen;
    }
    return _writen - old_w;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    if (_writen - _readn < len) {
        throw runtime_error("peek_output argument too big");
    }
    std::string ret;
    for (size_t i = 0; i < len; i++) {
        ret.push_back(_buffer[(_readn + i) % _capacity]);
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    if (_writen - _readn < len) {
        throw runtime_error("pop_output argument too big");
    }
    _readn += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    size_t temp_len = min(len, buffer_size());
    std::string ret = peek_output(temp_len);
    pop_output(temp_len);
    return ret;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _writen - _readn; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _end_input && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _writen; }

size_t ByteStream::bytes_read() const { return _readn; }

size_t ByteStream::remaining_capacity() const { return _capacity - (_writen - _readn); }
