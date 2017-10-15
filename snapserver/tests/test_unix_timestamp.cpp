// Snap Websites Server -- verify that our Snap signal macros create valid
//                         unix timestamps (equal to mktime)
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#pragma GCC diagnostic ignored "-Wformat"

#include <snapwebsites/plugins.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char *argv[])
{
    struct tm t;
    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = 1;
    t.tm_mon = 0;
    t.tm_year = 70;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = -1;
    time_t localtime_timestamp = mktime(&t);

    int err(0);
    for(int month = 1; month <= 12; ++month) {
        for(int day = 1; day <= 31; ++day) {
            for(int hour = 0; hour < 24; ++hour) {
                for(int minute = 0; minute < 60; ++minute) {
                    for(int second = 0; second < 60; ++second) {
                        time_t snap_timestamp = SNAP_UNIX_TIMESTAMP(1970, month, day, hour, minute, second);
                        struct tm t2;
                        t2.tm_sec = second;
                        t2.tm_min = minute;
                        t2.tm_hour = hour;
                        t2.tm_mday = day;
                        t2.tm_mon = month - 1;
                        t2.tm_year = 1970 - 1900;
                        t2.tm_wday = 0;
                        t2.tm_yday = 0;
                        t2.tm_isdst = 0;
                        time_t mktime_timestamp = mktime(&t2) - localtime_timestamp;
//if(hour == 0 && minute == 0 && second == 0)
//printf("%d-%d-%d -> %d\n", 1970, month, day, (int)_SNAP_UNIX_TIMESTAMP_YDAY(1970, month, day));
                        if(snap_timestamp != mktime_timestamp) {
                            fprintf(stderr, "error: invalid conversion with %d-%d-%d %d:%d:%d -> %td != %td (diff %td)\n",
                                1970, month, day, hour, minute, second,
                                snap_timestamp, mktime_timestamp, snap_timestamp - mktime_timestamp);
                            err = 1;
                            exit(1);
                        }
                    }
                }
            }
        }
    }
    for(int year = 1970; year < 2068; ++year) {
        for(int month = 1; month <= 12; ++month) {
            for(int day = 1; day <= 31; ++day) {
                time_t snap_timestamp = SNAP_UNIX_TIMESTAMP(year, month, day, 0, 0, 0);
                struct tm t3;
                t3.tm_sec = 0;
                t3.tm_min = 0;
                t3.tm_hour = 0;
                t3.tm_mday = day;
                t3.tm_mon = month - 1;
                t3.tm_year = year - 1900;
                t3.tm_wday = 0;
                t3.tm_yday = 0;
                t3.tm_isdst = 0;
                time_t mktime_timestamp = mktime(&t3) - localtime_timestamp;
//printf("%d-%d-%d -> %d\n", year, month, day, (int)_SNAP_UNIX_TIMESTAMP_YDAY(year, month, day));
                if(snap_timestamp != mktime_timestamp) {
                    fprintf(stderr, "error: invalid conversion with %d-%d-%d 00:00:00 -> %td != %td (diff %td)\n",
                        year, month, day,
                        snap_timestamp, mktime_timestamp, snap_timestamp - mktime_timestamp);
                    err = 1;
                    exit(1);
                }
            }
        }
    }

    return err;
}
#pragma GCC diagnostic pop

// vim: ts=4 sw=4 et
