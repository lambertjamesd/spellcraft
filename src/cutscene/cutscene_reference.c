#include "cutscene_reference.h"
#include "../scene/scene.h"
#include "cutscene_runner.h"

#include <assert.h>

#define FUNC_PREFIX     "func:"

void cutscene_ref_init_null(cutscene_ref_t* ref) {
    ref->type = CUTSCENE_REF_NONE;
}

void cutscene_ref_init(cutscene_ref_t* ref, const char* name) {
    if (name == NULL || *name == '\0') {
        ref->type = CUTSCENE_REF_NONE;
    } else if (strncmp("rom:/", name, strlen("rom:/")) == 0) {
        ref->type = CUTSCENE_REF_DIRECT;
        ref->data.direct.cutscene = cutscene_load(name);
        // TODO look at #suffix
    } else if (strncmp(FUNC_PREFIX, name, strlen(FUNC_PREFIX)) == 0) {
        ref->type = CUTSCENE_REF_SCENE;
        name += strlen(FUNC_PREFIX);
        int name_len = strlen(name);
        ref->data.scene.name = malloc(name_len + 1);
        ref->data.scene.name[name_len] = '\0';
        strcpy(ref->data.scene.name, name);
    } else {
        assert(false);
    }
}

void cutscene_ref_destroy(cutscene_ref_t* ref) {
    switch (ref->type) {
        case CUTSCENE_REF_DIRECT:
            cutscene_runner_cancel(ref->data.direct.cutscene);
            cutscene_free(ref->data.direct.cutscene);
            break;
        case CUTSCENE_REF_SCENE:
            free(ref->data.scene.name);
            break;
        default:
            break;
    }
}

void cutscene_ref_run_then_callback(cutscene_ref_t* ref, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    switch (ref->type) {
        case CUTSCENE_REF_DIRECT:
            cutscene_runner_run(ref->data.direct.cutscene, 0, finish_callback, data, subject);
            break;
        case CUTSCENE_REF_SCENE:
            int fn_index = current_scene && current_scene->cutscene ? cutscene_find_function_index(current_scene->cutscene, ref->data.scene.name) : -1;

            if (fn_index != -1) {
                cutscene_runner_run(current_scene->cutscene, fn_index, finish_callback, data, subject);
            } else if (finish_callback) {
                finish_callback(NULL, data);
            }
            break;
        default:
            break;
    }
}

void cutscene_ref_run(cutscene_ref_t* ref, entity_id subject) {
    cutscene_ref_run_then_callback(ref, NULL, NULL, subject);
}

struct nested_on_finish {
    cutscene_finish_callback on_finish;
    void *data;
    enum cutscene_ref_type ref_type;
};

void cutscene_runner_destroy_with_callback(struct cutscene* cutscene, void* data) {
    if (!data) {
        return;
    }

    struct nested_on_finish* finish = (struct nested_on_finish*)data;
    finish->on_finish(cutscene, finish->data);
    
    if (finish->ref_type == CUTSCENE_REF_DIRECT) {
        cutscene_runner_free_on_finish()(cutscene, NULL);
    }
}

void cutscene_ref_run_then_destroy(cutscene_ref_t* ref, entity_id subject, cutscene_finish_callback on_finish, void* data) {
    struct nested_on_finish* finish = NULL;
    if (on_finish) {
        finish = malloc(sizeof(struct nested_on_finish));
        finish->on_finish = on_finish;
        finish->data = data;
        finish->ref_type = ref->type;
    }

    switch (ref->type) {
        case CUTSCENE_REF_DIRECT:
            cutscene_runner_run(ref->data.direct.cutscene, 0, finish ? cutscene_runner_destroy_with_callback : cutscene_runner_free_on_finish(), finish, subject);
            break;
        case CUTSCENE_REF_SCENE:
            int fn_index = current_scene && current_scene->cutscene ? cutscene_find_function_index(current_scene->cutscene, ref->data.scene.name) : -1;
            free(ref->data.scene.name);
            ref->data.scene.name = NULL;

            if (fn_index != -1) {
                cutscene_runner_run(current_scene->cutscene, fn_index, cutscene_runner_destroy_with_callback, finish, subject);
            }
            break;
        default:
            break;
    }
    ref->type = CUTSCENE_REF_NONE;
}