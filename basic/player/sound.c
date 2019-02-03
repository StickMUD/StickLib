#ifndef _SOUND_H
#include <sound.h>
#endif /* _SOUND_H */

#ifndef PLAYER_C
#include "/basic/player/env.c"
#endif /* PLAYER_C */

varargs public void
sound(string _sound, int _volume, int _duration, int _priority) {
    string sound_enabled = query_env("sound");

    if (interactive() && sound_enabled) {
	if (sound_enabled == SD_PROTOCOL_MSP) {
	    //write(sprintf("!!SOUND(%s V=%d L=%d P=%d U=%s)\n",
	    //_sound, _volume, _duration, _priority, SD_URL));
	    write(sprintf("!!SOUND(%s V=%d L=%d P=%d)\n",
		_sound,
		_volume != 0 ? _volume : SD_VOLUME_NORMAL,
		_duration != 0 ? _duration : SD_DURATION_NORMAL,
		_priority != 0 ? _priority : SD_PRIORITY_NORMAL));
	}
    }
}
