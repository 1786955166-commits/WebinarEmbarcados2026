#include <gzm/rtc_mgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtc_mgr, CONFIG_LOG_DEFAULT_LEVEL);

#define SIZE_BUFFER_FORMAT     25
#define RTC_MGR_TIMEZONE_MIN   -12
#define RTC_MGR_TIMEZONE_MAX    14
#define SECONDS_PER_HOUR        3600

static const struct device *g_rtc;
static int8_t g_timezone;   /* default 0 (UTC) */

/*
 * Array of format strings indexed by enum rtc_mgr_str_format
 */
static const char *const rtc_mgr_default_formats[RTC_MGR_FMT_MAX] = {
	[RTC_MGR_FMT_TIME_HMS]       = "%H:%M:%S",
	[RTC_MGR_FMT_TIME_HM]        = "%H:%M",
	[RTC_MGR_FMT_DATE_YMD_DASH]  = "%Y-%m-%d",
	[RTC_MGR_FMT_DATE_DMY_DASH]  = "%d-%m-%Y",
	[RTC_MGR_FMT_DATE_YMD_SLASH] = "%Y/%m/%d",
	[RTC_MGR_FMT_DATE_DMY_SLASH] = "%d/%m/%Y",
	[RTC_MGR_FMT_DT_YMD_DASH]    = "%Y-%m-%d %H:%M:%S",
	[RTC_MGR_FMT_DT_DMY_DASH]    = "%d-%m-%Y %H:%M:%S",
	[RTC_MGR_FMT_TD_YMD_DASH]    = "%H:%M:%S %Y-%m-%d",
	[RTC_MGR_FMT_TD_DMY_DASH]    = "%H:%M:%S %d-%m-%Y",
	[RTC_MGR_FMT_DT_YMD_SLASH]   = "%Y/%m/%d %H:%M:%S",
	[RTC_MGR_FMT_DT_DMY_SLASH]   = "%d/%m/%Y %H:%M:%S",
	[RTC_MGR_FMT_TD_YMD_SLASH]   = "%H:%M:%S %Y/%m/%d",
	[RTC_MGR_FMT_TD_DMY_SLASH]   = "%H:%M:%S %d/%m/%Y",
};

static int rtc_mgr_calc_week_day(struct rtc_time *time);
static uint32_t rtc_mgr_calc_timestamp(struct rtc_time *time);

static int rtc_mgr_calc_week_day(struct rtc_time *time)
{
	int ret = 0;
	uint16_t weekday_calc = 0;
	int16_t adjustment = 0;
	int16_t month = 0;
	int16_t year = 0;

	if (time->tm_year < 2000) {
		time->tm_year += 2000;
	}

	adjustment = (14 - time->tm_mon) / 12;
	month = time->tm_mon + 12 * adjustment - 2;
	year = time->tm_year - adjustment;

	time->tm_mday = time->tm_mday - 1;
	weekday_calc = (time->tm_mday + (13 * month - 1) / 5 + year +
			year / 4 - year / 100 + year / 400) % 7;
	time->tm_mday++;

	if (weekday_calc < 8) {
		time->tm_wday = (weekday_calc + 1) % 7;

		/* If all is as expected returns a true */
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static uint32_t rtc_mgr_calc_timestamp(struct rtc_time *time)
{
	uint16_t year_val = 0;
	uint16_t month_val = 0;
	uint16_t day_val = 0;
	uint32_t timestamp = 0;

	year_val = time->tm_year;
	month_val = time->tm_mon;
	day_val = time->tm_mday;

	/* January and February are counted as months 13 and 14 of the previous year */
	if (month_val <= 2) {
		month_val += 12;
		year_val -= 1;
	}

	/* Convert years to days */
	timestamp = (365 * year_val) + (year_val / 4) - (year_val / 100) +
		    (year_val / 400);

	/* Convert months to days */
	timestamp += (30 * month_val) + (3 * (month_val + 1) / 5) + day_val;

	/* Unix time starts on January 1st, 1970 */
	timestamp -= 719561;

	/* Convert days to seconds */
	timestamp *= 86400;

	/* Add hours, minutes and seconds */
	timestamp += (3600 * time->tm_hour) + (60 * time->tm_min) + time->tm_sec;

	return timestamp;
}

int rtc_mgr_init(const struct device *rtc)
{
	int ret = 0;
	struct rtc_time time;

	if (rtc == NULL) {
		LOG_ERR("rtc_mgr_init: NULL device");
		return -EINVAL;
	}

	if (!device_is_ready(rtc)) {
		LOG_ERR("rtc_mgr_init: device %s not ready", rtc->name);
		return -ENODEV;
	}

	g_rtc = rtc;

	memset(&time, 0, sizeof(struct rtc_time));

	ret = rtc_mgr_get(&time);
	if (ret) {
		time.tm_year = 2024;
		time.tm_mon = 1;
		time.tm_mday = 1;
		time.tm_hour = 23;
		time.tm_min = 50;
		time.tm_sec = 0;
		ret = rtc_mgr_set(&time);
	}

	return ret;
}

int rtc_mgr_get(struct rtc_time *time)
{
	int ret = 0;

	ret = rtc_get_time(g_rtc, time);
	if (!ret) {
		time->tm_year += 1900;
		time->tm_mon += 1;
		ret = rtc_mgr_calc_week_day(time);
	}

	return ret;
}

int rtc_mgr_get_timestamp(uint32_t *timestamp)
{
	int ret = 0;
	struct rtc_time time;

	memset(&time, 0, sizeof(struct rtc_time));
	ret = rtc_get_time(g_rtc, &time);
	if (ret) {
		return ret;
	}

	time.tm_year += 1900;
	time.tm_mon += 1;
	ret = rtc_mgr_calc_week_day(&time);
	if (ret) {
		return ret;
	}

	*timestamp = rtc_mgr_calc_timestamp(&time);
	return 0;
}

int rtc_mgr_set_timestamp(uint32_t timestamp)
{
	int ret = 0;
	struct rtc_time time;

	memset(&time, 0, sizeof(struct rtc_time));
	ret = rtc_mgr_timestamp_to_time(timestamp, &time);

	if (ret) {
		LOG_ERR("rtc_mgr: timestamp_to_time failed (%d)", ret);
		return ret;
	}

	LOG_INF("rtc_mgr: setting time to %d-%02d-%02d %02d:%02d:%02d",
		time.tm_year, time.tm_mon, time.tm_mday,
		time.tm_hour, time.tm_min, time.tm_sec);

	ret = rtc_mgr_set(&time);
	if (ret) {
		LOG_ERR("rtc_mgr: set failed (%d)", ret);
	}

	return ret;
}

int rtc_mgr_set_timezone(int8_t tz_hours)
{
	if (tz_hours < RTC_MGR_TIMEZONE_MIN || tz_hours > RTC_MGR_TIMEZONE_MAX) {
		LOG_ERR("rtc_mgr_set_timezone: %d out of range [%d..%d]",
			tz_hours, RTC_MGR_TIMEZONE_MIN, RTC_MGR_TIMEZONE_MAX);
		return -EINVAL;
	}
	g_timezone = tz_hours;
	LOG_INF("rtc_mgr: timezone set to UTC%+d", tz_hours);
	return 0;
}

int rtc_mgr_get_local_timestamp(uint32_t *timestamp_local)
{
	uint32_t utc;
	int ret;

	if (timestamp_local == NULL) {
		return -EINVAL;
	}

	ret = rtc_mgr_get_timestamp(&utc);
	if (ret) {
		return ret;
	}

	*timestamp_local = utc + (int32_t)g_timezone * SECONDS_PER_HOUR;
	return 0;
}

int rtc_mgr_get_local(struct rtc_time *time_local)
{
	uint32_t local;
	int ret;

	if (time_local == NULL) {
		return -EINVAL;
	}

	ret = rtc_mgr_get_local_timestamp(&local);
	if (ret) {
		return ret;
	}

	return rtc_mgr_timestamp_to_time(local, time_local);
}

int rtc_mgr_timestamp_to_time(uint32_t timestamp, struct rtc_time *time)
{
	uint32_t year_adj = 0;
	uint32_t julian_day = 0;
	uint32_t century = 0;
	uint32_t day_of_year = 0;
	uint32_t month_calc = 0;
	uint32_t day_of_month = 0;
	int err = 0;

	/* Negative Unix time values are not supported */
	if (timestamp < 1) {
		return -EINVAL;
	}

	memset(time, 0, sizeof(struct rtc_time));

	/* Retrieve hours, minutes and seconds */
	time->tm_sec = timestamp % 60;
	timestamp /= 60;
	time->tm_min = timestamp % 60;
	timestamp /= 60;
	time->tm_hour = timestamp % 24;
	timestamp /= 24;

	/* Convert Unix time to date */
	year_adj = (uint32_t)((4 * timestamp + 102032) / 146097 + 15);
	julian_day = (uint32_t)(timestamp + 2442113 + year_adj - (year_adj / 4));
	century = (20 * julian_day - 2442) / 7305;
	day_of_year = julian_day - 365 * century - (century / 4);
	month_calc = day_of_year * 1000 / 30601;
	day_of_month = day_of_year - month_calc * 30 - month_calc * 601 / 1000;

	/* January and February are counted as months 13 and 14 of the previous year */
	if (month_calc <= 13) {
		century -= 4716;
		month_calc -= 1;
	} else {
		century -= 4715;
		month_calc -= 13;
	}

	/* Retrieve year, month and day */
	time->tm_year = century;
	time->tm_mon = month_calc;
	time->tm_mday = day_of_month;

	/* Calculate day of week */
	err = rtc_mgr_calc_week_day(time);

	return err;
}

int rtc_mgr_set(struct rtc_time *time)
{
	int ret = 0;
	struct rtc_time rtc_time_copy;

	/* Invalid date */
	if (time->tm_year <= 2020) {
		LOG_WRN("rtc_mgr: invalid year %d (must be > 2020)", time->tm_year);
		return -EINVAL;
	}

	/* Copy to avoid modifying original struct */
	memcpy(&rtc_time_copy, time, sizeof(struct rtc_time));

	/* Convert to RTC driver format (year since 1900, month 0-11) */
	rtc_time_copy.tm_year -= 1900;

	if (rtc_time_copy.tm_mon) {
		rtc_time_copy.tm_mon -= 1;
	}

	rtc_time_copy.tm_wday = 0;
	ret = rtc_set_time(g_rtc, &rtc_time_copy);

	return ret;
}

int rtc_mgr_time_str(enum rtc_mgr_str_format fmt, char *buf, int buflen)
{
	int ret = 0;
	struct tm time_info;
	struct rtc_time time;

	if (buf == NULL || buflen <= 0 || fmt >= RTC_MGR_FMT_MAX) {
		return -EINVAL;
	}

	ret = rtc_mgr_get(&time);
	if (ret) {
		return ret;
	}

	time_info.tm_year = time.tm_year;
	time_info.tm_mon = time.tm_mon;
	time_info.tm_mday = time.tm_mday;
	time_info.tm_hour = time.tm_hour;
	time_info.tm_min = time.tm_min;
	time_info.tm_sec = time.tm_sec;

	if (time_info.tm_year) {
		time_info.tm_year -= 1900;
	}

	if (time_info.tm_mon) {
		time_info.tm_mon -= 1;
	}

	strftime(buf, buflen, rtc_mgr_default_formats[fmt], &time_info);

	return 0;
}

bool rtc_mgr_is_leap_year(uint16_t year)
{
	if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
		return true;
	}

	return false;
}

