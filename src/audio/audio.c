#include "audio.h"

#include <stdbool.h>
#include "../math/mathf.h"

#define MAX_ACTIVE_SOUNDS   16
#define SPEED_OF_SOUND      343

struct active_sound {
    wav64_t* wav;
    bool is_3d;
    int16_t priority;
    float volume;
    float frequency;
    struct Vector3 position;
    struct Vector3 velocity;
};

typedef struct active_sound active_sound_t;

struct audio_listener {
    struct Vector3 position;
    struct Vector3 right;
    struct Vector3 velocity;
};

static audio_id active_sound_ids[MAX_ACTIVE_SOUNDS];
static active_sound_t active_sounds[MAX_ACTIVE_SOUNDS];
static audio_id next_id = MAX_ACTIVE_SOUNDS;
static struct audio_listener listener = {
    .position = {0.0f, 0.0f, 0.0f},
    .right = {1.0f, 0.0f, 0.0f},
};

static inline int audio_channel_from_id(audio_id id) {
    return id & (MAX_ACTIVE_SOUNDS - 1);
}

audio_id audio_next_id(int channel) {
    audio_id result = next_id + channel;

    next_id += MAX_ACTIVE_SOUNDS;
    if (next_id == 0) {
        next_id = MAX_ACTIVE_SOUNDS;
    }

    return result;
}

void audio_active_sound_init(active_sound_t* active_sound) {
    active_sound->wav = NULL;
    active_sound->is_3d = false;
    active_sound->priority = 0;
    active_sound->volume = 0;
    active_sound->position = gZeroVec;
    active_sound->velocity = gZeroVec;
    active_sound->frequency = 0.0f;
}

void audio_player_init() {
    audio_init(44100, 4);
    mixer_init(MAX_ACTIVE_SOUNDS);

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        active_sound_ids[i] = 0;
        audio_active_sound_init(&active_sounds[i]);
    }
}

int audio_time_left(int ch) {
    active_sound_t* curr = &active_sounds[ch];

    return curr->wav->wave.len - (int)mixer_ch_get_pos(ch);
}

audio_id audio_find_available_sound(int16_t priority) {
    for (uint16_t* curr = &active_sound_ids[0]; curr < &active_sound_ids[MAX_ACTIVE_SOUNDS]; curr += 1) {
        if (*curr == 0) {
            return audio_next_id(curr - active_sound_ids);
        }
    }

    active_sound_t* best = NULL;
    int best_index = -1;

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        active_sound_t* curr = &active_sounds[i];

        if (curr->priority > priority) {
            continue;
        }

        if (best == NULL || 
            curr->priority < best->priority || 
            audio_time_left(i) < audio_time_left(best_index)
        ) {
            best = curr;
            best_index = i;
        }
    }

    if (best_index == -1) {
        return 0;
    }

    return audio_next_id(best_index);
}

void audio_set_pan_volume(int channel, float volume, float pan) {
    mixer_ch_set_vol(
        channel,
        volume * clampf(1 - pan, 0.0f, 1.0f),
        volume * clampf(pan + 1, 0.0f, 1.0f)
    );
}

void audio_process_3d(active_sound_t* sound, int channel) {
    struct Vector3 offset;
    vector3Sub(&sound->position, &listener.position, &offset);

    float mag_sqrd = vector3MagSqrd(&offset);

    if (mag_sqrd < 0.00001f) {
        mixer_ch_set_vol(channel, 1.0f, 1.0f);
        return;
    }

    float volume = sound->volume / mag_sqrd;

    if (volume < 0.0001f) {
        mixer_ch_set_vol(channel, 0.0f, 0.0f);
        return;
    }

    vector3Scale(&offset, &offset, 1.0f / sqrtf(mag_sqrd));

    float pan = vector3Dot(&offset, &listener.right);

    audio_set_pan_volume(channel, volume, pan);

    float sound_direction = vector3Dot(&sound->velocity, &offset);
    float listener_direction = vector3Dot(&listener.velocity, &offset);

    float factor = (sound_direction - listener_direction) > 0.0f ? 1.0f : -1.0f;

    float denominator = SPEED_OF_SOUND + sound_direction * factor;

    if (denominator > SPEED_OF_SOUND - 1.0f) {
        denominator = SPEED_OF_SOUND - 1.0f;
    }

    mixer_ch_set_freq(channel, sound->frequency * (SPEED_OF_SOUND - listener_direction * factor) / denominator);
}

audio_id audio_play_2d(wav64_t* wav, float volume, float pan, float pitch_shift, int16_t priority) {
    audio_id audio_id = audio_find_available_sound(priority);

    if (!audio_id) {
        return 0;
    }

    int channel = audio_channel_from_id(audio_id);
    active_sound_ids[channel] = audio_id;

    active_sound_t* sound = &active_sounds[channel];
    sound->priority = priority;
    sound->is_3d = false;
    sound->wav = wav;

    mixer_ch_play(channel, &wav->wave);
    audio_set_pan_volume(channel, volume, pan);
    mixer_ch_set_freq(channel, wav->wave.frequency * pitch_shift);

    return audio_id;
}

audio_id audio_play_3d(wav64_t* wav, float volume, struct Vector3* pos, struct Vector3* vel, float pitch_shift, int16_t priority) {
    audio_id audio_id = audio_find_available_sound(priority);

    if (!audio_id) {
        return 0;
    }
    
    int channel = audio_channel_from_id(audio_id);
    active_sound_ids[channel] = audio_id;

    active_sound_t* sound = &active_sounds[channel];
    sound->priority = priority;
    sound->is_3d = true;
    sound->wav = wav;
    sound->position = *pos;
    sound->velocity = *vel;
    sound->volume = volume;
    sound->frequency = pitch_shift * wav->wave.frequency;

    audio_process_3d(sound, channel);
    
    return audio_id;
}

void audio_player_update() {
    if (audio_can_write()) {
        short *buf = audio_write_begin();
        mixer_poll(buf, audio_get_buffer_length());
        audio_write_end();
    }

    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i += 1) {
        if (!active_sound_ids[i]) {
            continue;
        }

        if (!mixer_ch_playing(i)) {
            active_sound_ids[i] = 0;
            continue;
        }
        
        active_sound_t* sound = &active_sounds[i];

        if (sound->is_3d) {
            audio_process_3d(sound, i);
        }
    }
}

void audio_update_position(audio_id id, struct Vector3* pos, struct Vector3* vel) {
    if (!id) {
        return;
    }

    int channel = audio_channel_from_id(id);

    if (active_sound_ids[channel] != id) {
        return;
    }

    
    active_sound_t* sound = &active_sounds[channel];

    if (!sound->is_3d) {
        return;
    }

    sound->position = *pos;
    sound->velocity = *vel;
}