#ifndef GZM_RTC_MGR_H
#define GZM_RTC_MGR_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>

enum rtc_mgr_str_format {
	RTC_MGR_FMT_TIME_HMS,        /* HH:MM:SS */
	RTC_MGR_FMT_TIME_HM,         /* HH:MM    */
	RTC_MGR_FMT_DATE_YMD_DASH,   /* YYYY-MM-DD (ISO 8601) */
	RTC_MGR_FMT_DATE_DMY_DASH,   /* DD-MM-YYYY */
	RTC_MGR_FMT_DATE_YMD_SLASH,  /* YYYY/MM/DD */
	RTC_MGR_FMT_DATE_DMY_SLASH,  /* DD/MM/YYYY */
	RTC_MGR_FMT_DT_YMD_DASH,     /* YYYY-MM-DD HH:MM:SS (ISO 8601) */
	RTC_MGR_FMT_DT_DMY_DASH,     /* DD-MM-YYYY HH:MM:SS */
	RTC_MGR_FMT_TD_YMD_DASH,     /* HH:MM:SS YYYY-MM-DD */
	RTC_MGR_FMT_TD_DMY_DASH,     /* HH:MM:SS DD-MM-YYYY */
	RTC_MGR_FMT_DT_YMD_SLASH,    /* YYYY/MM/DD HH:MM:SS */
	RTC_MGR_FMT_DT_DMY_SLASH,    /* DD/MM/YYYY HH:MM:SS */
	RTC_MGR_FMT_TD_YMD_SLASH,    /* HH:MM:SS YYYY/MM/DD */
	RTC_MGR_FMT_TD_DMY_SLASH,    /* HH:MM:SS DD/MM/YYYY */
	RTC_MGR_FMT_MAX
};

/**
 * @brief Initialize the RTC manager with an RTC device handle.
 *
 * The caller resolves the RTC device from its own Device Tree (e.g. via
 * DEVICE_DT_GET(DT_NODELABEL(rtc))) and passes it in. This keeps the library
 * portable across MCUs/vendors that use different node labels for the RTC.
 *
 * @param rtc Pointer to the RTC device. Must not be NULL.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_init(const struct device *rtc);

/**
 * @brief Set the RTC time from an rtc_time structure.
 *
 * @param time Pointer to the time structure to set.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_set(struct rtc_time *time);

/**
 * @brief Get the current RTC time.
 *
 * @param time Pointer to store the current time.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_get(struct rtc_time *time);

/**
 * @brief Get the current time as a Unix timestamp.
 *
 * @param timestamp Pointer to store the timestamp.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_get_timestamp(uint32_t *timestamp);

/**
 * @brief Set the RTC time from a Unix timestamp.
 *
 * @param timestamp Unix timestamp to set.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_set_timestamp(uint32_t timestamp);

/**
 * @brief Configure the timezone offset (in hours) used by the *_local APIs.
 *
 * The RTC itself is always kept in UTC, this offset is metadata applied
 * only when converting to local representations (get_local_timestamp,
 * get_local). Default after init is 0 (UTC).
 *
 * @param tz_hours Hour offset relative to UTC, range [-12, +14]
 *                 (e.g. -3 for BRT, +1 for CET).
 * @return 0 on success, -EINVAL if out of range.
 */
int rtc_mgr_set_timezone(int8_t tz_hours);

/**
 * @brief Get the current time as a Unix timestamp adjusted to local time.
 *
 * Equivalent to: get_timestamp(&utc); *out = utc + tz_hours * 3600;
 *
 * @param timestamp_local Pointer to store the local-adjusted timestamp.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_get_local_timestamp(uint32_t *timestamp_local);

/**
 * @brief Get the current time as a broken-down rtc_time in local time.
 *
 * @param time_local Pointer to store the local time.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_get_local(struct rtc_time *time_local);

/**
 * @brief Convert a Unix timestamp to an rtc_time structure.
 *
 * @param timestamp Unix timestamp to convert.
 * @param time Pointer to store the converted time.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_timestamp_to_time(uint32_t timestamp, struct rtc_time *time);

/**
 * @brief Format the current time as a string.
 *
 * @param fmt Desired string format.
 * @param buf Buffer to store the formatted string.
 * @param buflen Length of the buffer.
 * @return 0 on success, negative errno on failure.
 */
int rtc_mgr_time_str(enum rtc_mgr_str_format fmt, char *buf, int buflen);

/**
 * @brief Check if a year is a leap year.
 *
 * @param year Year to check.
 * @return true if the year is a leap year, false otherwise.
 */
bool rtc_mgr_is_leap_year(uint16_t year);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* GZM_RTC_MGR_H */
