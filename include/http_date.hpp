// Parse an HTTP `Date` header (RFC 7231 IMF-fixdate, always GMT), e.g.
// "Wed, 19 Nov 2026 14:03:21 GMT", into Unix seconds.
//
// Used as a clock fallback when NTP (UDP 123) is blocked, as on many campus
// networks: any HTTPS answer from our API carries the server time.
// Host-testable: no Arduino headers.

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace syncfit {

namespace detail {
// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm).
inline std::int64_t daysFromCivil(std::int64_t y, unsigned m, unsigned d) {
  y -= m <= 2;
  const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}
}  // namespace detail

inline bool parseHttpDate(const char* text, std::int64_t& epoch) {
  if (text == nullptr) return false;
  static const char* kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char weekday[4] = {}, month[4] = {}, zone[4] = {};
  int day = 0, year = 0, hour = 0, minute = 0, second = 0;
  if (std::sscanf(text, "%3s, %d %3s %d %d:%d:%d %3s", weekday, &day, month, &year, &hour, &minute, &second, zone) != 8) {
    return false;
  }
  if (std::strcmp(zone, "GMT") != 0) return false;
  unsigned m = 0;
  for (unsigned i = 0; i < 12; ++i) {
    if (std::strcmp(month, kMonths[i]) == 0) m = i + 1;
  }
  if (m == 0 || day < 1 || day > 31 || year < 2020 || hour > 23 || minute > 59 || second > 60) return false;
  epoch = detail::daysFromCivil(year, m, static_cast<unsigned>(day)) * 86400 + hour * 3600 + minute * 60 + second;
  return true;
}

}  // namespace syncfit
