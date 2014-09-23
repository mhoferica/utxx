//----------------------------------------------------------------------------
/// \file  time.hpp
//----------------------------------------------------------------------------
/// \brief Time-related functions
/// \author:  Serge Aleynikov
// Parts of code taken from:
// http://home.roadrunner.com/~hinnant/date_algorithms.html
//----------------------------------------------------------------------------
// Copyright (c) 2013-09-07 Howard Hinnant
// Created: 2003-07-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_TIME_VAL_PTIME_HPP_
#define _UTXX_TIME_VAL_PTIME_HPP_

#include <utxx/time_val.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/error.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#if __cplusplus >= 201103L
#  include <chrono>
#endif

namespace utxx {

    /// Returns true if y is a leap year
    inline bool is_leap(unsigned y) noexcept {
        unsigned  n =  y / 100, rem100 = y - n*100;
        bool rem400 = (n & 0x3) || rem100; // Is not divisible by 400
        return !rem400             // 1. The year divisible by 400 is always leap
            || (rem100 &&          // 2. The year divisible by 100 is not leap
               !(rem100 & 0x3));   // 3. The year divisible by 4   is leap
    }

    /// Returns number of days in a month (no range checking!)
    /// @param a_month        is a month number in range 1 to 12.
    /// @param a_is_leap_year indicates if this is a leap year
    inline unsigned days_in_a_month(unsigned a_month, bool a_is_leap_year) {
        assert(a_month >= 1 && a_month <= 12);
        static const unsigned s_ndays[2][12] = {
            {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
            {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
        };
        return s_ndays[a_is_leap_year][a_month-1];
    }

    /// Returns number of days singe the beginning of year \a y.
    inline int days_since_beg_of_year(int y, int m, int d) noexcept {
        assert(m >= 1 && d <= 12);
        static const unsigned s_ndays[2][12] = {
            {0, 31, 31+28, 59+31, 90+30, 120+31, 151+30, 181+31, 212+31, 243+30, 273+31, 304+30},
            {0, 31, 31+29, 60+31, 91+30, 121+31, 152+30, 182+31, 213+31, 244+30, 274+31, 305+30}
        };
        int leap = is_leap(y);
        return s_ndays[leap][m-1] + d;
    }

    /// Return number of days between two y/m/d pairs.
    inline int date_diff(unsigned y1, unsigned m1, unsigned d1,
                  unsigned y2, unsigned m2, unsigned d2)
    {
        if (unlikely(m1 < 1 || m1 > 12 || m2 < 1 || m2 > 12))
            throw badarg_error("Invalid month value");

        auto days_since_year0 =
            [] (unsigned y) { return y*365 + y/400 + y/4 - y/100; };

        return days_since_year0(y2) - days_since_year0(y1)
            + days_since_beg_of_year(y2, m2, d2)
            - days_since_beg_of_year(y1, m1, d1);
    }

    /// Returns number of days since epoch 1970-01-01.
    /// Negative values indicate days prior to 1970-01-01.
    /// @param y represents a year in the Gregorian calendar
    ///          in range (numeric_limits<Int>::min()/366 to
    ///          numeric_limits<Int>::max()/366)
    /// @param m represents a month in the Gregorian calendar (1 to 12)
    /// @param d represents a day of month in the Gregorian calendar (1 ...)
    inline int to_gregorian_days(int y, unsigned m, unsigned d)
    {
        static_assert(std::numeric_limits<unsigned>::digits >= 18,
                "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<int>::digits >= 20,
                "This algorithm has not been ported to a 16 bit signed integer");
        if (utxx::unlikely(m < 1 || m > 12 || d < 1 || d > 31))
            throw badarg_error("Invalid range of month/day argument (m=",m, ", d=",d,')');

        const int yr  = y - (m <= 2);
        const int era = (yr >= 0 ? yr : yr-399) / 400;
        const unsigned yoe = static_cast<unsigned>(yr - era * 400);     // [0, 399]
        const unsigned doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;  // [0, 365]
        const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
        return era * 146097 + int(doe) - 719468;
    }

    /// Returns year/month/day triple in Gregorian calendar.
    /// @param a_days is number of days since 1970-01-01 and is in the range:
    ///     (numeric_limits<int>::min() to numeric_limits<int>::max()-719468).
    inline std::tuple<int, unsigned, unsigned> from_gregorian_days(int a_days) noexcept
    {
        static_assert(std::numeric_limits<unsigned>::digits >= 18,
                "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<int>::digits >= 20,
                "This algorithm has not been ported to a 16 bit signed integer");
        const int     days = a_days + 719468;
        const int      era = (days >= 0 ? days : days - 146096) / 146097;
        const unsigned doe = static_cast<unsigned>(days - era * 146097);      // [0, 146096]
        const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365; // [0, 399]
        const int        y = static_cast<int>(yoe) + era * 400;
        const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);               // [0, 365]
        const unsigned  mp = (5*doy + 2)/153;                                 // [0, 11]
        const unsigned   d = doy - (153*mp+2)/5 + 1;                          // [1, 31]
        const unsigned   m = mp + (mp < 10 ? 3 : -9);                         // [1, 12]
        return std::tuple<int, unsigned, unsigned>(y + (m <= 2), m, d);
    }

    /// Split seconds since epoch to y/m/d
    inline std::tuple<int, unsigned, unsigned> from_gregorian_time(time_t a_days) noexcept {
        return from_gregorian_days(a_days / 86400);
    }

    template <class To, class Rep, class Period>
    To round_down(const std::chrono::duration<Rep, Period>& d) {
        To t = std::chrono::duration_cast<To>(d);
        return (t > d) ? t-1 : t;
    }

    /// Returns day of week in civil calendar [0, 6] -> [Sun, Sat].
    /// @param a_days is number of days since 1970-01-01 and is in the range:
    ///  [numeric_limits<Int>::min(), numeric_limits<Int>::max()-4].
    constexpr int weekday_from_days(int a_days) noexcept {
        return a_days >= -4 ? (a_days+4) % 7 : (a_days+5) % 7 + 6;
    }

    /// Convert y/m/d into seconds since epoch 1970-1-1
    inline time_t mktime_utc(int y, unsigned m, unsigned d) {
        return to_gregorian_days(y, m, d) * 86400;
    }

    /// Convert date into seconds since epoch 1970-1-1
    inline time_t mktime_utc(int      year, unsigned month, unsigned day,
                             unsigned hour, unsigned min,   unsigned sec) {
        time_t res = to_gregorian_days(year, month, day) * 86400;
        res += 3600*hour + 60*min + sec;
        return res;
    }

    /// Convert tm structure into time since epoch 1970-1-1
    inline time_t mktime_utc(struct tm *tm) {
        return mktime_utc(tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                          tm->tm_hour, tm->tm_min, tm->tm_sec);
    }

#if __cplusplus >= 201103L

    // Example maketm_utc(std::chrono::system_clock::now())
    template <class Duration>
    std::tm
    to_tm_utc(std::chrono::time_point<std::chrono::system_clock, Duration> tp)
    {
        using namespace std;
        using namespace std::chrono;
        typedef duration<int, ratio_multiply<hours::period, ratio<24>>> days;
        // t is time duration since 1970-01-01
        Duration t = tp.time_since_epoch();
        // d is days since 1970-01-01
        days d = round_down<days>(t);
        // t is now time duration since midnight of day d
        t -= d;
        // break d down into year/month/day
        int year, month, day;
        std::tie(year, month, day) = from_gregorian_days(d.count());

        int h = duration_cast<hours>(t).count();   t -= hours(tp);
        int m = duration_cast<minutes>(t).count(); t -= minutes(tp);
        int s = duration_cast<seconds>(t).count();

        std::tm tm = {
            s, m, h, day, month-1, year-1900,
            weekday_from_days(d.count()), d.count() - to_gregorian_days(year,1,1),
            0, 0, 0
        };

        return tm;
    }

#endif


} // namespace utxx

#endif //_UTXX_TIME_HPP_

