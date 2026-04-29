#ifndef GZM_RINGTONE_H
#define GZM_RINGTONE_H

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ringtone_notification_type {
	RINGTONE_TOUCH_PRESSED,
	RINGTONE_ALARM1,
	RINGTONE_ALARM2,
};

/**
 * @brief Initialize the ringtone player with a PWM buzzer device.
 *
 * The caller resolves the PWM spec from its own Device Tree (e.g. via
 * PWM_DT_SPEC_GET(DT_ALIAS(pwm_buzzer0))) and passes it in. This keeps the
 * library portable across MCUs/vendors that use different DT aliases for
 * the buzzer.
 *
 * @param buzzer Pointer to a PWM device-tree spec describing the buzzer.
 *               Must remain valid for the lifetime of the player.
 * @return 0 on success, negative errno on error.
 */
int ringtone_init(const struct pwm_dt_spec *buzzer);

/**
 * @brief Set PWM buzzer volume as a percentage.
 *
 * Drives the PWM duty cycle directly, useful for quick beeps
 * without parsing an RTTTL string (e.g. confirmation tones).
 * In a passive buzzer the duty cycle is roughly proportional to
 * perceived loudness when combined with a fixed frequency.
 *
 * @param percent Volume 0-100 (0 = off).
 * @return 0 on success, negative errno on error.
 */
int ringtone_set_volume(int percent);

/**
 * @brief Play a predefined notification sound.
 *
 * @param type Notification type.
 * @return 0 on success, negative errno on error.
 */
int ringtone_play_notification(enum ringtone_notification_type type);

/**
 * @brief Play a custom RTTTL ringtone string.
 *
 * @param rtttl_string RTTTL-formatted string.
 * @return 0 on success, negative errno on error.
 */
int ringtone_play_custom(const char *rtttl_string);

/**
 * @brief Stop any currently playing ringtone.
 */
void ringtone_stop(void);

/**
 * @brief Check if a ringtone is currently playing.
 *
 * @return true if playing.
 */
bool ringtone_is_playing(void);

/**
 * @brief Wait for the current ringtone to finish.
 *
 * @param timeout Timeout (K_FOREVER for indefinite).
 * @return 0 on success, -EAGAIN on timeout.
 */
int ringtone_wait_done(k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* GZM_RINGTONE_H */
