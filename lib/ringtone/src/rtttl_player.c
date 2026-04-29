#include "rtttl_player.h"

#include <string.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ringtone, CONFIG_LOG_DEFAULT_LEVEL);

/* Thread stack must be defined statically (cannot be inside a struct with C99) */
K_THREAD_STACK_DEFINE(ringtone_stack, RINGTONE_THREAD_STACK_SIZE);

/*
 * Note frequencies for octave 4 (in Hz).
 * Index: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B
 */
static const uint16_t note_freq_oct4[] = {
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
};

/* Map note letter to semitone index: c=0, d=2, e=4, f=5, g=7, a=9, b=11 */
static int ringtone_note_to_index(char note)
{
	switch (note) {
	case 'c': return 0;
	case 'd': return 2;
	case 'e': return 4;
	case 'f': return 5;
	case 'g': return 7;
	case 'a': return 9;
	case 'b': return 11;
	default:  return -1;
	}
}

/* Get frequency for a given semitone index and octave */
static uint32_t ringtone_get_frequency(int index, int octave)
{
	uint32_t freq = note_freq_oct4[index];

	if (octave > 4) {
		freq <<= (octave - 4);
	} else if (octave < 4) {
		freq >>= (4 - octave);
	}

	return freq;
}

/* Skip whitespace */
static const char *ringtone_skip_ws(const char *p)
{
	while (*p == ' ' || *p == '\t') {
		p++;
	}
	return p;
}

/* Parse integer from string, advance pointer */
static int ringtone_parse_int(const char **pp)
{
	const char *p = *pp;
	int val = 0;

	while (*p >= '0' && *p <= '9') {
		val = val * 10 + (*p - '0');
		p++;
	}
	*pp = p;
	return val;
}

/*
 * Parse the RTTTL header: "name:d=D,o=O,b=B:"
 * Returns pointer to the start of the notes section.
 */
static const char *ringtone_parse_header(const char *rtttl, int *duration,
					 int *octave, int *bpm)
{
	/* Skip name (everything before first ':') */
	const char *p = strchr(rtttl, ':');

	if (!p) {
		return NULL;
	}
	p++; /* skip ':' */
	p = ringtone_skip_ws(p);

	/* Parse default values section */
	while (*p && *p != ':') {
		p = ringtone_skip_ws(p);

		if (*p == 'd' && *(p + 1) == '=') {
			p += 2;
			*duration = ringtone_parse_int(&p);
		} else if (*p == 'o' && *(p + 1) == '=') {
			p += 2;
			*octave = ringtone_parse_int(&p);
		} else if (*p == 'b' && *(p + 1) == '=') {
			p += 2;
			*bpm = ringtone_parse_int(&p);
		}

		if (*p == ',') {
			p++;
		}
	}

	if (*p == ':') {
		p++;
	}

	return p;
}

/*
 * Parse a single note from the notes section.
 * Format: [duration]note[#][.][octave][.]
 * Returns pointer to next note (after comma) or end of string.
 * Sets freq_hz to 0 for a pause (note 'p').
 */
static const char *ringtone_parse_note(const char *p, int def_duration,
					int def_octave, int bpm,
					uint32_t *freq_hz,
					uint32_t *duration_ms)
{
	bool sharp = false;
	bool dotted = false;
	char note_char = 0;
	int dur = 0;
	int octave = def_octave;
	int idx = 0;
	uint32_t whole_note_ms = 0;
	uint32_t note_ms = 0;

	p = ringtone_skip_ws(p);
	if (!*p) {
		return NULL;
	}

	/* Optional duration prefix */
	dur = ringtone_parse_int(&p);
	if (dur == 0) {
		dur = def_duration;
	}

	/* Note letter */
	note_char = *p;
	if (note_char >= 'A' && note_char <= 'G') {
		note_char += 32; /* to lowercase */
	}
	p++;

	/* Optional sharp '#' */
	if (*p == '#') {
		sharp = true;
		p++;
	}

	/* Optional dot after note (before octave) */
	if (*p == '.') {
		dotted = true;
		p++;
	}

	/* Optional octave */
	if (*p >= '0' && *p <= '9') {
		octave = ringtone_parse_int(&p);
	}

	/* Optional dot after octave */
	if (*p == '.') {
		dotted = true;
		p++;
	}

	/* Calculate duration in ms: whole note = (60000/bpm)*4 */
	whole_note_ms = (60000UL * 4) / bpm;
	note_ms = whole_note_ms / dur;

	if (dotted) {
		note_ms = note_ms + (note_ms / 2);
	}
	*duration_ms = note_ms;

	/* Calculate frequency */
	if (note_char == 'p') {
		*freq_hz = 0; /* pause */
	} else {
		idx = ringtone_note_to_index(note_char);

		if (idx < 0) {
			*freq_hz = 0;
		} else {
			if (sharp) {
				idx++;
				if (idx > 11) {
					idx = 0;
					octave++;
				}
			}
			*freq_hz = ringtone_get_frequency(idx, octave);
		}
	}

	/* Skip comma separator */
	p = ringtone_skip_ws(p);
	if (*p == ',') {
		p++;
	}

	return p;
}

/* Play a single tone on the buzzer */
static void ringtone_play_tone(const struct pwm_dt_spec *buzzer,
				uint32_t freq_hz)
{
	uint32_t period_ns = 0;
	uint32_t pulse_ns = 0;

	if (freq_hz == 0) {
		/* Silence */
		pwm_set_pulse_dt(buzzer, 0);
	} else {
		/* Set period for the desired frequency, 50% duty cycle */
		period_ns = 1000000000UL / freq_hz;
		pulse_ns = period_ns / 2;

		pwm_set_dt(buzzer, period_ns, pulse_ns);
	}
}

/* Silence the buzzer */
static void ringtone_buzzer_silence(const struct pwm_dt_spec *buzzer)
{
	pwm_set_pulse_dt(buzzer, 0);
}

/* Thread entry point: parses and plays the RTTTL string */
static void ringtone_play_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int duration = 0;
	int octave = 0;
	int bpm = 0;
	uint32_t freq_hz = 0;
	uint32_t duration_ms = 0;
	uint32_t play_ms = 0;
	uint32_t gap_ms = 0;
	const char *rtttl = NULL;
	const char *notes = NULL;
	const char *p = NULL;
	struct ringtones *rt = (struct ringtones *)arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	rtttl = rt->config.rtttl_string;

	if (!rtttl) {
		goto done;
	}

	duration = rt->default_duration;
	octave = rt->default_octave;
	bpm = rt->bpm;

	/* Parse header */
	notes = ringtone_parse_header(rtttl, &duration, &octave, &bpm);

	if (!notes) {
		LOG_ERR("Invalid RTTTL header");
		goto done;
	}

	LOG_INF("Playing: d=%d, o=%d, b=%d", duration, octave, bpm);

	/* Play each note */
	p = notes;

	while (p && *p && !rt->stop_requested) {
		freq_hz = 0;
		duration_ms = 0;

		p = ringtone_parse_note(p, duration, octave, bpm,
					&freq_hz, &duration_ms);
		if (!p) {
			break;
		}

		ringtone_play_tone(rt->buzzer, freq_hz);

		/* Play for 90% of duration, then 10% silence for note separation */
		play_ms = (duration_ms * 9) / 10;
		gap_ms = duration_ms - play_ms;

		if (play_ms > 0) {
			k_msleep(play_ms);
		}

		if (rt->stop_requested) {
			break;
		}

		/* Brief silence between notes */
		if (gap_ms > 0 && freq_hz > 0) {
			ringtone_buzzer_silence(rt->buzzer);
			k_msleep(gap_ms);
		}
	}

done:
	ringtone_buzzer_silence(rt->buzzer);
	rt->playing = false;
	k_sem_give(&rt->done_sem);
}

int ringtones_init(struct ringtones *rt, const struct pwm_dt_spec *buzzer,
		   const char *default_rtttl, int def_duration,
		   int def_octave, int def_bpm)
{
	if (!rt || !buzzer) {
		return -EINVAL;
	}

	memset(rt, 0, sizeof(*rt));
	rt->buzzer = buzzer;
	rt->stack = ringtone_stack;
	rt->config.rtttl_string = default_rtttl;
	rt->default_duration = def_duration > 0 ? def_duration : 4;
	rt->default_octave = def_octave > 0 ? def_octave : 5;
	rt->bpm = def_bpm > 0 ? def_bpm : 100;
	rt->playing = false;
	rt->stop_requested = false;
	rt->tid = NULL;

	k_sem_init(&rt->done_sem, 0, 1);

	return 0;
}

int ringtones_play(struct ringtones *rt)
{
	if (!rt || !rt->buzzer || !rt->config.rtttl_string) {
		return -EINVAL;
	}

	if (rt->playing) {
		ringtones_stop(rt);
	}

	rt->stop_requested = false;
	rt->playing = true;
	k_sem_reset(&rt->done_sem);

	rt->tid = k_thread_create(&rt->thread, rt->stack,
				  RINGTONE_THREAD_STACK_SIZE,
				  ringtone_play_thread_fn, rt, NULL, NULL,
				  RINGTONE_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

void ringtones_stop(struct ringtones *rt)
{
	if (!rt || !rt->playing) {
		return;
	}

	rt->stop_requested = true;

	/* Wait for thread to finish */
	if (rt->tid) {
		k_thread_join(&rt->thread, K_MSEC(5000));
		rt->tid = NULL;
	}
}

bool ringtones_is_playing(struct ringtones *rt)
{
	return rt ? rt->playing : false;
}

int ringtones_wait_done(struct ringtones *rt, k_timeout_t timeout)
{
	int ret;

	if (!rt) {
		return -EINVAL;
	}

	if (!rt->playing) {
		return 0;
	}

	ret = k_sem_take(&rt->done_sem, timeout);

	if (ret == 0 && rt->tid) {
		k_thread_join(&rt->thread, K_MSEC(1000));
		rt->tid = NULL;
	}

	return ret;
}
