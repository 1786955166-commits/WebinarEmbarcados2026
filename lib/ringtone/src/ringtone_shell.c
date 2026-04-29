#include <gzm/ringtone.h>

#include <stdlib.h>

#include <zephyr/shell/shell.h>

static int ringtone_cmd_play(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: ringtone play <RTTTL string>");
		return -EINVAL;
	}

	shell_print(sh, "Playing: %s", argv[1]);
	ret = ringtone_play_custom(argv[1]);
	if (ret) {
		shell_error(sh, "Play failed: %d", ret);
		return ret;
	}
	return 0;
}

static int ringtone_cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ringtone_stop();
	shell_print(sh, "Ringtone stopped");
	return 0;
}

static const char *ringtone_shell_mario_rtttl =
	"Super Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,"
	"8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,"
	"8a6,16f6,8g6,8e6,16c6,16d6,8b";

static int ringtone_cmd_test(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Playing Mario theme...");
	ret = ringtone_play_custom(ringtone_shell_mario_rtttl);
	if (ret) {
		shell_error(sh, "Test failed: %d", ret);
		return ret;
	}
	return 0;
}

static int ringtone_cmd_alarm(const struct shell *sh, size_t argc, char **argv)
{
	int type;
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: ringtone alarm <1|2>");
		return -EINVAL;
	}

	type = atoi(argv[1]);
	switch (type) {
	case 1:
		ret = ringtone_play_notification(RINGTONE_ALARM1);
		break;
	case 2:
		ret = ringtone_play_notification(RINGTONE_ALARM2);
		break;
	default:
		shell_error(sh, "Invalid alarm type: %d (use 1 or 2)", type);
		return -EINVAL;
	}

	if (ret) {
		shell_error(sh, "Alarm failed: %d", ret);
		return ret;
	}
	shell_print(sh, "Playing alarm %d", type);
	return 0;
}

static int ringtone_cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Ringtone is %s",
		    ringtone_is_playing() ? "playing" : "idle");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ringtone_cmds,
	SHELL_CMD_ARG(play, NULL,
		"Play an RTTTL ringtone string\n"
		"Usage: ringtone play \"Name:d=4,o=5,b=100:note1,note2,...\"",
		ringtone_cmd_play, 2, 0),
	SHELL_CMD_ARG(stop, NULL,
		"Stop the currently playing ringtone\n"
		"Usage: ringtone stop",
		ringtone_cmd_stop, 1, 0),
	SHELL_CMD_ARG(test, NULL,
		"Play the Mario theme (test)\n"
		"Usage: ringtone test",
		ringtone_cmd_test, 1, 0),
	SHELL_CMD_ARG(alarm, NULL,
		"Play a predefined alarm sound\n"
		"Usage: ringtone alarm <1|2>",
		ringtone_cmd_alarm, 2, 0),
	SHELL_CMD_ARG(status, NULL,
		"Check if ringtone is playing\n"
		"Usage: ringtone status",
		ringtone_cmd_status, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ringtone, &ringtone_cmds,
		   "Ringtone player and RTTTL commands", NULL);
