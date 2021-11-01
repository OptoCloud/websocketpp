/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef HTTP_PARSER_RESPONSE_IMPL_HPP
#define HTTP_PARSER_RESPONSE_IMPL_HPP

#include <algorithm>
#include <istream>
#include <sstream>
#include <string>
#include <charconv>

#include <websocketpp/http/parser.hpp>

namespace websocketpp {
namespace http {
namespace parser {

inline size_t response::consume(char const * buf, size_t len) {
    if (m_state == DONE) {return 0;}

    if (m_state == BODY) {
        return this->process_body(buf,len);
    }

    // copy new header bytes into buffer
    m_buf->append(buf,len);

    // Search for delimiter in buf. If found read until then. If not read all
    std::string::iterator begin = m_buf->begin();
    std::string::iterator end = begin;


    for (;;) {
        // search for delimiter
        end = std::search(
            begin,
            m_buf->end(),
            header_delimiter.begin(),
            header_delimiter.end()
        );

        m_header_bytes += (end - begin + header_delimiter.size() + 1);
        
        if (m_header_bytes > max_header_size) {
            // exceeded max header size
            throw exception("Maximum header size exceeded.",
                status_code::request_header_fields_too_large);
        }

        if (end == m_buf->end()) {
            // we are out of bytes. Discard the processed bytes and copy the
            // remaining unprecessed bytes to the beginning of the buffer
            std::copy(begin,end,m_buf->begin());
            m_buf->resize(static_cast<std::string::size_type>(end-begin));

            m_read += len;
            m_header_bytes -= m_buf->size();

            return len;
        }

        //the range [begin,end) now represents a line to be processed.

        if (end-begin == 0) {
            // we got a blank line
            if (m_state == RESPONSE_LINE) {
                throw exception("Incomplete Request",status_code::bad_request);
            }

            // TODO: grab content-length
            std::string_view length = get_header("Content-Length");

            if (length.empty()) {
                // no content length found, read indefinitely
                m_read = 0;
            } else {
                std::from_chars_result result = std::from_chars(length.data(), length.data() + length.size(), m_read);

                if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
                    throw exception("Unable to parse Content-Length header", status_code::bad_request);
                }
            }

            m_state = BODY;

            // calc header bytes processed (starting bytes - bytes left)
            size_t read = (
                len - static_cast<std::string::size_type>(m_buf->end() - end)
                + header_delimiter.size()
            );

            // if there were bytes left process them as body bytes
            if (read < len) {
                read += this->process_body(buf+read,(len-read));
            }

            // frees memory used temporarily during header parsing
            m_buf.reset();

            return read;
        } else {
            if (m_state == RESPONSE_LINE) {
                this->process(begin,end);
                m_state = HEADERS;
            } else {
                this->process_header(begin,end);
            }
        }

        begin = end + header_delimiter.size();
    }
}

inline size_t response::consume(std::istream & s) {
    char buf[istream_buffer];
    size_t bytes_read;
    size_t bytes_processed;
    size_t total = 0;

    while (s.good()) {
        s.getline(buf,istream_buffer);
        bytes_read = static_cast<size_t>(s.gcount());

        if (s.fail() || s.eof()) {
            bytes_processed = this->consume(buf,bytes_read);
            total += bytes_processed;

            if (bytes_processed != bytes_read) {
                // problem
                break;
            }
        } else if (s.bad()) {
            // problem
            break;
        } else {
            // the delimiting newline was found. Replace the trailing null with
            // the newline that was discarded, since our raw consume function
            // expects the newline to be be there.
            buf[bytes_read-1] = '\n';
            bytes_processed = this->consume(buf,bytes_read);
            total += bytes_processed;

            if (bytes_processed != bytes_read) {
                // problem
                break;
            }
        }
    }

    return total;
}

inline std::vector<std::uint8_t> response::raw() const {
    // TODO: validation. Make sure all required fields have been set?

    std::string_view version = get_version();
    std::string status_code = std::to_string(m_status_code);
    std::string headers = raw_headers();

    std::vector<std::uint8_t> ret;
    ret.reserve(
                version.size() + 1 + status_code.size() + 1 + m_status_msg.size() + 2 +
                headers.size() + 2 +
                m_body.size()
                );

    ret.insert(ret.end(), version.begin(), version.end());
    ret.insert(ret.end(), { ' ' });
    ret.insert(ret.end(), status_code.begin(), status_code.end());
    ret.insert(ret.end(), { ' ' });
    ret.insert(ret.end(), m_status_msg.begin(), m_status_msg.end());
    ret.insert(ret.end(), { '\r', '\n' });
    ret.insert(ret.end(), headers.begin(), headers.end());
    ret.insert(ret.end(), { '\r', '\n' });
    ret.insert(ret.end(), m_body.begin(), m_body.end());

    return ret;
}

inline void response::set_status(status_code::value code) {
    // TODO: validation?
    m_status_code = code;
    m_status_msg = get_string(code);
}

inline void response::set_status(status_code::value code, std::string_view msg)
{
    // TODO: validation?
    m_status_code = code;
    m_status_msg.assign(msg.begin(), msg.end());
}

inline void response::process(std::string::iterator begin,
    std::string::iterator end)
{
    std::string::iterator cursor_start = begin;
    std::string::iterator cursor_end = std::find(begin,end,' ');

    if (cursor_end == end) {
        throw exception("Invalid response line",status_code::bad_request);
    }

    set_version(std::string(cursor_start,cursor_end));

    cursor_start = cursor_end+1;
    cursor_end = std::find(cursor_start,end,' ');

    if (cursor_end == end) {
        throw exception("Invalid request line",status_code::bad_request);
    }

    int code;

    std::istringstream ss(std::string(cursor_start,cursor_end));

    if ((ss >> code).fail()) {
        throw exception("Unable to parse response code",status_code::bad_request);
    }

    set_status(status_code::value(code),std::string(cursor_end+1,end));
}

inline size_t response::process_body(const char* buf, size_t len) {
    // If no content length was set then we read forever and never set m_ready
    if (m_read == 0) {
        //m_body.append(buf,len);
        //return len;
        m_state = DONE;
        return 0;
    }

    // Otherwise m_read is the number of bytes left.
    size_t to_read;

    if (len >= m_read) {
        // if we have more bytes than we need read, read only the amount needed
        // then set done state
        to_read = m_read;
        m_state = DONE;
    } else {
        // we need more bytes than are available, read them all
        to_read = len;
    }

    m_body.insert(m_body.end(), buf , buf + to_read);
    m_read -= to_read;
    return to_read;
}

} // namespace parser
} // namespace http
} // namespace websocketpp

#endif // HTTP_PARSER_RESPONSE_IMPL_HPP
