#pragma once
#include <charconv>
#include <string_view>
#include <cctype>
#include <type_traits>
#include "fast_float/fast_float.h"


namespace PolyMesh::V3 {


namespace off_file_helper {
    inline void skip_ws(std::string_view& in) noexcept {
        while (!in.empty() && std::isspace(static_cast<unsigned char>(in.front())))
            in.remove_prefix(1);
    }

    // Parses the next number and removes it from the front of the view
    template<class T>
    bool parse_next(std::string_view& in, T& out) noexcept {
        skip_ws(in);
        const char* first = in.data();
        const char* last  = first + in.size();
        std::from_chars_result r;
        if constexpr (std::is_floating_point_v<T>) {
            auto fr = fast_float::from_chars(first, last, out);
            r = {fr.ptr, fr.ec};
        } else {
            r = std::from_chars(first, last, out);
        }
        if (r.ec != std::errc{}) return false;
        in.remove_prefix(static_cast<size_t>(r.ptr - first));
        return true;
    }
};


};