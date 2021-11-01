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

#ifndef WEBSOCKETPP_UTILITIES_IMPL_HPP
#define WEBSOCKETPP_UTILITIES_IMPL_HPP

#include <algorithm>
#include <string>
#include <vector>
#include <span>

namespace websocketpp {
namespace utility {

constexpr void to_lower_impl(char* data, std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] += 32;
        }
    }
}

constexpr void to_hex_impl(const std::uint8_t* in, char* out, std::size_t inSize) {
    constexpr std::string_view hexchars = "0123456789ABCDEF";
    for (std::size_t i = 0; i < inSize; i++) {
        std::size_t j = i * 3;
        out[j + 0] = hexchars[(in[j] & 0xF0) >> 4];
        out[j + 1] = hexchars[(in[j] & 0x0F) >> 0];
        out[j + 2] = ' ';
    }
}

inline std::string to_lower(std::string_view in) {
    std::string out(in.begin(), in.end());

    to_lower_impl(out.data(), out.size());

    return out;
}

inline std::string to_hex(std::string_view input) {
    std::string output;
    output.resize(input.size() * 3);

    to_hex_impl(reinterpret_cast<const std::uint8_t*>(input.data()), output.data(), input.size());

    return output;
}

inline std::string to_hex(std::span<const std::uint8_t> input) {
    std::string output;
    output.resize(input.size() * 3);

    to_hex_impl(input.data(), output.data(), input.size());

    return output;
}

inline std::string to_hex(const char* input,size_t length) {
    const std::uint8_t* ptr = (const std::uint8_t*)input;
    return to_hex(std::span<const std::uint8_t>{ ptr, ptr + length });
}

inline std::vector<std::uint8_t> to_vec(std::string_view input) {
    return std::vector<std::uint8_t>(input.begin(), input.end());
}
inline std::string to_str(std::string_view input) {
    return std::string(input);
}
inline std::string to_str(std::span<const std::uint8_t> input) {
    return std::string(input.begin(), input.end());
}
inline std::string_view to_strview(std::span<const std::uint8_t> input) {
    return { reinterpret_cast<const char*>(input.data()), input.size() };
}

inline std::string string_replace_all(std::string subject, const std::string&
    search, const std::string& replace)
{
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

} // namespace utility
} // namespace websocketpp

#endif // WEBSOCKETPP_UTILITIES_IMPL_HPP
