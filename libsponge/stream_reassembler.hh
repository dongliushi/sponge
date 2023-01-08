#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <list>
#include <memory>
#include <queue>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    class ByteNodeList {  // 存储需要重组的字节信息
        class ByteNode {  // 标识字节的index和len的结构
          public:
            std::shared_ptr<std::string> _data_ptr;
            size_t _index;
            size_t _len;
            ByteNode(const std::string &data, const size_t index)
                : _data_ptr(std::make_shared<std::string>(data)), _index(index), _len(data.size()) {}
        };
        auto find_first_insert(size_t index) {  // 找到第一个最前的可插入的节点
            for (auto iter = _storage.begin(); iter != _storage.end(); ++iter) {
                if (index < iter->_index + iter->_len) {
                    return iter;
                }
            }
            return _storage.end();
        }
        std::list<ByteNode> _storage = {};  // 存储节点的list

      public:
        size_t nbytes() const {  // 返回所存节点的字节总数
            size_t ret = 0;
            for (auto &node : _storage) {
                ret += node._len;
            }
            return ret;
        }
        void reassemble(ByteStream &output) {  // 将能重组的字节输入到管道中
            for (auto iter = _storage.begin(); iter != _storage.end();) {
                if (iter->_index <= output.bytes_written()) {
                    if (iter->_index + iter->_len > output.bytes_written()) {  // 这里判断插入是否有效
                        output.write(iter->_data_ptr->substr(output.bytes_written() - iter->_index));
                    }
                    _storage.erase(iter++);
                } else
                    break;
            }
        }
        bool empty() const { return _storage.empty(); }
        void clear() { _storage.clear(); }
        void push_string(const std::string &data, size_t index) {
            size_t data_len = data.size();
            auto iter = find_first_insert(index);
            size_t end = index + data_len;
            if (iter != _storage.end()) {                    // 判断是否有找到能插入的节点
                if (index <= iter->_index) {                 // 插入数据的首字节在被插入节点的前面
                    if (end >= iter->_index + iter->_len) {  // 情况1： 节点被覆盖，将该节点去除，重新插入
                        _storage.erase(iter);
                        push_string(data, index);
                    } else if (end <= iter->_index) {  // 情况2： 被插入节点和插入数据无接触，直接插入
                        _storage.insert(iter, ByteNode(data, index));
                    } else if (end > iter->_index) {  // 情况3： 和被插入节点有交集，分割数据后重新插入
                        push_string(data.substr(0, iter->_index - index), index);
                    }
                } else {  // 插入数据首字节在被插入节点里面
                    if (end >
                        iter->_index + iter->_len) {  // 插入数据长度需要大于被插入节点，否则直接抛弃(因为被覆盖了)
                        push_string(data.substr(iter->_index + iter->_len - index), iter->_index + iter->_len);
                    }
                }
            } else {  // 没找到，插入最后面
                _storage.insert(iter, ByteNode(data, index));
            }
        }
    };
    ByteNodeList _buffer = {};
    bool _eof = false;
    size_t _eof_index = 0;

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
