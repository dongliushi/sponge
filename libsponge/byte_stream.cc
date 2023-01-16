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
}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    if (_end_input)
        return 0;
    size_t write_size = min(data.size(), remaining_capacity());
    _buffer.append(data, 0, write_size);
    _writen += write_size;
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    size_t temp_len = min(len, buffer_size());
    return _buffer.substr(0, temp_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    size_t temp_len = min(len, buffer_size());
    _buffer.erase(0, temp_len);
    _readn += temp_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    std::string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _writen - _readn; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _end_input && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _writen; }

size_t ByteStream::bytes_read() const { return _readn; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
