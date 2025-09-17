#ifndef DATETIME_H
#define DATETIME_H

#include <string>
#include <time.h>
#include <cstring>

class DateTime
{
public:
    DateTime();
    DateTime(std::string timestamp);
    DateTime(time_t timestamp)
    {
        struct tm *tm_info = gmtime(&timestamp);
        memcpy(&timeinfo, tm_info, sizeof(tm));
        year_ = tm_info->tm_year + 1900;
        month_ = tm_info->tm_mon + 1;
        day_ = tm_info->tm_mday;
        hour_ = tm_info->tm_hour;
        minute_ = tm_info->tm_min;
        second_ = tm_info->tm_sec;
    }
    bool ok();
    std::string format(const char *fmt)
    {
        return safe_strftime(fmt, &timeinfo);
    }
    std::string niceDate();
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    double diff(const DateTime &other) const
    {
        return difftime(mktime(const_cast<tm *>(&timeinfo)), mktime(const_cast<tm *>(&other.timeinfo)));
    }

private:
    std::string timestamp_;
    struct tm timeinfo;
    int year_;
    int month_;
    int day_;
    int hour_;
    int minute_;
    int second_;

    std::string safe_strftime(const char *fmt, const tm *t);
    const char *dateSuffix() const;
};

#endif // DATETIME_H
