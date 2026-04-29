#include <gzm/ringtone.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "rtttl_player.h"

LOG_MODULE_REGISTER(ringtone, CONFIG_LOG_DEFAULT_LEVEL);

static const char *ringtone_touch_pressed_rtttl = "Touch:d=16,o=6,b=180:c,p";
static const char *ringtone_alarm1_rtttl = "Alarm1:d=8,o=5,b=160:c6,p,c6,p,c6,p,c6";
static const char *ringtone_alarm2_rtttl = "Alarm2:d=4,o=5,b=200:c6,g5,c6,g5,c6,g5,c6";

static const struct pwm_dt_spec *g_buzzer;
static struct ringtones g_ringstones;
static bool g_initialized;

int ringtone_init(const struct pwm_dt_spec *buzzer)
{
	int ret;

	if (buzzer == NULL) {
		LOG_ERR("ringtone_init: NULL buzzer spec");
		return -EINVAL;
	}

	if (!pwm_is_ready_dt(buzzer)) {
		LOG_ERR("PWM device %s is not ready", buzzer->dev->name);
		return -EIO;
	}

	g_buzzer = buzzer;
	pwm_set_pulse_dt(g_buzzer, 0);

	ret = ringtones_init(&g_ringstones, g_buzzer,
			     ringtone_touch_pressed_rtttl, 4, 5, 100);
	if (ret) {
		LOG_ERR("Failed to initialize ringtones: %d", ret);
		return ret;
	}

	g_initialized = true;
	LOG_INF("Ringtone player initialized");
	return 0;
}

int ringtone_set_volume(int percent)
{
	uint32_t pulse_width;
	int ret;

	if (!g_initialized) {
		return -ENODEV;
	}

	if (percent < 0 || percent > 100) {
		return -EINVAL;
	}

	pulse_width = (g_buzzer->period * percent) / 100;
	ret = pwm_set_pulse_dt(g_buzzer, pulse_width);
	if (ret) {
		LOG_ERR("Failed to set pulse width: %d", ret);
		return ret;
	}
	return 0;
}

int ringtone_play_notification(enum ringtone_notification_type type)
{
	if (!g_initialized) {
		return -ENODEV;
	}

	if (ringtones_is_playing(&g_ringstones)) {
		ringtones_stop(&g_ringstones);
	}

	switch (type) {
	case RINGTONE_TOUCH_PRESSED:
		g_ringstones.config.rtttl_string = ringtone_touch_pressed_rtttl;
		break;
	case RINGTONE_ALARM1:
		g_ringstones.config.rtttl_string = ringtone_alarm1_rtttl;
		break;
	case RINGTONE_ALARM2:
		g_ringstones.config.rtttl_string = ringtone_alarm2_rtttl;
		break;
	default:
		return -EINVAL;
	}

	return ringtones_play(&g_ringstones);
}

int ringtone_play_custom(const char *rtttl_string)
{
	if (!rtttl_string) {
		return -EINVAL;
	}
	if (!g_initialized) {
		return -ENODEV;
	}

	if (ringtones_is_playing(&g_ringstones)) {
		ringtones_stop(&g_ringstones);
	}

	g_ringstones.config.rtttl_string = rtttl_string;
	return ringtones_play(&g_ringstones);
}

void ringtone_stop(void)
{
	if (g_initialized) {
		ringtones_stop(&g_ringstones);
	}
}

bool ringtone_is_playing(void)
{
	return g_initialized && ringtones_is_playing(&g_ringstones);
}

int ringtone_wait_done(k_timeout_t timeout)
{
	if (!g_initialized) {
		return -ENODEV;
	}
	return ringtones_wait_done(&g_ringstones, timeout);
}
