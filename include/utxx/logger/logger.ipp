//----------------------------------------------------------------------------
/// \file  logger_impl.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of logger.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma  once

#include <utxx/logger/logger.hpp>
#include <utxx/time_val.hpp>
#include <utxx/convert.hpp>
#include <boost/filesystem/path.hpp>
#include <algorithm>

namespace utxx {

template <typename Fun>
inline bool logger::dolog(
    log_level           a_level,
    const std::string&  a_category,
    const Fun&          a_fun,
    const char*         a_src_loc,
    std::size_t         a_src_loc_len)
{
    if (!is_enabled(a_level))
        return false;

    return m_queue.emplace(a_level, a_category, a_fun, a_src_loc, a_src_loc_len);
}

inline bool logger::dolog(
    log_level           a_level,
    const std::string&  a_category,
    const char*         a_buf,
    std::size_t         a_size,
    const char*         a_src_loc,
    size_t              a_src_loc_len)
{
    if (!is_enabled(a_level))
        return false;

    std::string sbuf(a_buf, a_size);

    return m_queue.emplace(a_level, a_category, sbuf, a_src_loc, a_src_loc_len);
}

template <int N>
inline bool logger::logcs(
    log_level           a_level,
    const std::string&  a_category,
    const char*         a_buf,
    std::size_t         a_size,
    const char        (&a_src_loc)[N])
{
    return dolog(a_level, a_category, a_buf, a_size, a_src_loc, N);
}

template <int N, typename... Args>
inline bool logger::logfmt(
    log_level           a_level,
    const std::string&  a_category,
    const char        (&a_src_loc)[N],
    const char*         a_fmt,
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    char buf[1024];
    int  n = snprintf(buf, sizeof(buf), a_fmt, a_args...);
    std::string sbuf(buf, std::min<int>(n, sizeof(buf)-1));
    return m_queue.emplace(a_level, a_category, sbuf, a_src_loc, N-1);
}

template <int N, typename... Args>
inline bool logger::logs(
    log_level           a_level,
    const std::string&  a_category,
    const char        (&a_src_loc)[N],
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    detail::basic_buffered_print<1024> buf;
    buf.print(std::forward<Args>(a_args)...);
    return m_queue.emplace(a_level, a_category, buf.to_string(), a_src_loc, N-1);
}

template <int N>
inline bool logger::log(
    log_level a_level, const std::string& a_category, const std::string& a_msg,
    const char (&a_src_loc)[N])
{
    if (!is_enabled(a_level))
        return false;

    return m_queue.emplace(a_level, a_category, a_msg, a_src_loc, N-1);
}

template <int N, typename... Args>
inline bool logger::async_logs(
    log_level           a_level,
    const std::string&  a_category,
    const char        (&a_src_loc)[N],
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    auto fun = [=](const char* pfx, size_t psz, const char* sfx, size_t ssz) {
        detail::basic_buffered_print<1024> buf;
        buf.sprint(pfx, psz);
        buf.print (std::forward<Args>(a_args)...);
        buf.sprint(sfx, ssz);
        return buf.to_string();
    };
    return m_queue.emplace(a_level, a_category, fun, a_src_loc, N-1);
}

// TODO: make synchronous string formatting
template <int N, typename... Args>
inline bool logger::async_logfmt(
    log_level           a_level,
    const std::string&  a_category,
    const char         (&a_src_loc)[N],
    const char*         a_fmt,
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    auto fun = [=](char* a_buf, size_t a_size) {
        return snprintf(a_buf, a_size, a_fmt, std::forward<Args>(a_args)...);
    };
    return m_queue.emplace(a_level, a_category, fun, a_src_loc, N-1);
}

} // namespace utxx
