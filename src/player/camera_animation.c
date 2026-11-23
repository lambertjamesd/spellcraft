#include "camera_animation.h"

#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <libdragon.h>
#include "../util/hash_map.h"
#include "../util/file.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"

void camera_animation_list_init(struct camera_animation_list* list, int count, uint32_t rom_location) {
    list->animations = malloc(sizeof(struct camera_animation) * count);
    list->animation_count = count;
    list->rom_location = rom_location;

    for (int i = 0; i < count; ++i) {
        list->animations[i].name = NULL;
    }
}

void camera_animation_list_destroy(struct camera_animation_list* list) {
    for (int i = 0; i < list->animation_count; ++i) {
        free(list->animations[i].name);
    }
    free(list->animations);
}

struct camera_animation* camera_animation_lookup(struct camera_animation_list* list, const char* name) {
    if (!list) {
        return NULL;
    }

    struct camera_animation* end = list->animations + list->animation_count;

    for (struct camera_animation* curr = list->animations; curr < end; ++curr) {
        if (strcmp(name, curr->name) == 0) {
            return curr;
        }
    }

    return NULL;
}

void camera_animations_load(struct camera_animation_list* list, const char* filename, FILE* file) {
    uint16_t count;
    fread(&count, sizeof(count), 1, file);
    camera_animation_list_init(list, count, 0);

    int filename_len = strlen(filename);
    char modified_filename[filename_len + 1];
    strcpy(modified_filename, filename + strlen("rom:/"));
    strcpy(filename_find_ext(modified_filename), ".sanim");

    list->rom_location = dfs_rom_addr(modified_filename);

    assert(list->rom_location);

    for (int i = 0; i < count; ++i) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        char* name = malloc(name_length + 1);
        fread(name, name_length, 1, file);
        name[name_length] = '\0';

        struct camera_animation* animation = &list->animations[i];

        animation->name = name;
        fread(&animation->frame_count, sizeof(uint16_t), 1, file);
        fread(&animation->rom_offset, sizeof(uint32_t), 1, file);
        animation->rom_offset += list->rom_location;

        fread(&animation->event_size, 2, 1, file);
        fread(&animation->event_offset, sizeof(uint32_t), 1, file);
        animation->event_offset += list->rom_location;
    }
}

static hash_map_t animation_map_lookup;

void camera_animation_map(camera_animation_list_t* list, entity_id id) {
    if (!hash_map_is_ready(&animation_map_lookup)) {
        hash_map_init(&animation_map_lookup, 4);
    }

    hash_map_set(&animation_map_lookup, id, list);
}

void camera_animation_unmap(entity_id id) {
    hash_map_delete(&animation_map_lookup, id);

    if (animation_map_lookup.count == 0) {
        hash_map_destroy(&animation_map_lookup);
    }
}

camera_animation_list_t* camera_animation_get(entity_id id) {
    return hash_map_get(&animation_map_lookup, id);
}

camera_event_runner_t* camera_anim_load_events(camera_animation_t* anim) {
    if (!anim->event_size) {
        return NULL;
    }

    FILE* file = pifile_open(anim->event_offset, anim->event_size, 0);

    uint16_t event_count;
    fread(&event_count, 2, 1, file);

    camera_event_runner_t* result = malloc(sizeof(camera_event_runner_t) + sizeof(camera_event_t) * event_count);

    camera_event_t* curr = result->all_events;
    result->next_event = result->all_events;
    result->signal[0] = '\0';

    for (int i = 0; i < event_count; i += 1) {
        fread(&curr->type, 1, 1, file);
        fread(&curr->frame, 2, 1, file);
        curr->unused = 0;
        
        switch (curr->type) {
            case CAMERA_EVENT_TYPE_END:
                break;
            case CAMERA_EVENT_TYPE_RUN:
                curr->data.run = cutscene_load_file(file);
                break;
            case CAMERA_EVENT_SIGNAL:
            case CAMERA_EVENT_WAIT: {
                uint8_t name_len;
                fread(&name_len, 1, 1, file);
                curr->data.signal= malloc(name_len + 1);
                fread(curr->data.signal, name_len, 1, file);
                curr->data.signal[name_len] = '\0';
                break;
            }
        }

        curr += 1;
    }

    fclose(file);

    return result;
}

void camera_anim_free_events(camera_event_runner_t* events) {
    if (!events) {
        return;
    }

    camera_event_t* curr = events->all_events;

    while (curr->type) {
        switch (curr->type) {
            case CAMERA_EVENT_TYPE_RUN:
                cutscene_runner_cancel(curr->data.run);
                cutscene_free(curr->data.run);
                break;
            case CAMERA_EVENT_SIGNAL:
            case CAMERA_EVENT_WAIT:
                free(curr->data.wait);
                break;
        }

        ++curr;
    }

    free(events);
}

bool camera_anim_run_events(camera_event_runner_t* events, uint16_t frame) {
    if (!events) {
        return true;
    }

    while (events->next_event->frame <= frame) {
        camera_event_t* curr = events->next_event;
        
        switch (curr->type)
        {
            case CAMERA_EVENT_TYPE_END:
                return true;
            case CAMERA_EVENT_TYPE_RUN:
                cutscene_runner_run(&(cutscene_run_args_t){
                    .cutscene = curr->data.run,
                    .function_index = 0,
                    .channel = CUTSCENE_CHANNEL_ANIMATION,
                    .finish_callback = NULL,
                    .data = NULL,
                    .subject = 0,
                });
                break;
            case CAMERA_EVENT_SIGNAL:
                assert(strlen(curr->data.signal) < MAX_SIGNAL_LENGTH);
                strcpy(events->signal, curr->data.signal);
                break;
            case CAMERA_EVENT_WAIT:
                if (strcmp(events->signal, curr->data.wait) != 0) {
                    // TODO check for deadlock somehow
                    return false;
                }
                break;
        }

        ++events->next_event;
    }

    return true;
}