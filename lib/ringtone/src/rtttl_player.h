#ifndef GZM_RTTTL_PLAYER_H
#define GZM_RTTTL_PLAYER_H

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RINGTONE_THREAD_STACK_SIZE CONFIG_GZM_RINGTONE_THREAD_STACK_SIZE
#define RINGTONE_THREAD_PRIORITY   10

struct ringtone_config {
	const char *rtttl_string;
};

struct ringtones {
	const struct pwm_dt_spec *buzzer;
	struct ringtone_config config;
	int default_duration;
	int default_octave;
	int bpm;
	volatile bool playing;
	volatile bool stop_requested;
	struct k_thread thread;
	struct k_sem done_sem;
	k_tid_t tid;
	k_thread_stack_t *stack;
};

/**
 * @brief Initialize the ringtone player.
 *
 * @param rt            Ringtone context.
 * @param buzzer        PWM device spec for the buzzer.
 * @param default_rtttl Default RTTTL string (stored in config).
 * @param def_duration  Default note duration (4 = quarter note).
 * @param def_octave    Default octave (5).
 * @param def_bpm       Default BPM (100).
 * @return 0 on success, negative errno on error.
 */
int ringtones_init(struct ringtones *rt, const struct pwm_dt_spec *buzzer,
		   const char *default_rtttl, int def_duration,
		   int def_octave, int def_bpm);

/**
 * @brief Start playing the RTTTL string in rt->config.rtttl_string.
 *
 * Non-blocking: spawns a thread to play the melody.
 *
 * @param rt Ringtone context.
 * @return 0 on success, negative errno on error.
 */
int ringtones_play(struct ringtones *rt);

/**
 * @brief Stop the currently playing ringtone.
 *
 * @param rt Ringtone context.
 */
void ringtones_stop(struct ringtones *rt);

/**
 * @brief Check if a ringtone is currently playing.
 *
 * @param rt Ringtone context.
 * @return true if playing.
 */
bool ringtones_is_playing(struct ringtones *rt);

/**
 * @brief Wait for the current ringtone to finish.
 *
 * @param rt      Ringtone context.
 * @param timeout Timeout (K_FOREVER to wait indefinitely).
 * @return 0 on success, -EAGAIN on timeout.
 */
int ringtones_wait_done(struct ringtones *rt, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* GZM_RTTTL_PLAYER_H */
