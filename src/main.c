#include <stdio.h>


#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "resource/mesh_cache.h"
#include "resource/material_cache.h"
#include "resource/sprite_cache.h"
#include "scene/camera.h"
#include "math/transform.h"

#include "render/render_batch.h"

#include <libdragon.h>

struct mesh* mesh_test;

struct Camera camera;

void setup() {
    mesh_test = mesh_cache_load("rom:/meshes/cube.mesh");

    camera_init(&camera, 70.0f, 0.5f, 10.0f);
    camera.transform.position.z = 5.0f;
}

float angle = 0.0f;

void render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    float aspect_ratio = (float)display_get_width() / (float)display_get_height();

    camera_apply(&camera, aspect_ratio, NULL);

    static const float gold[13] = { 0.24725, 0.1995, 0.0745, 1.0,      /* ambient */
                   0.75164, 0.60648, 0.22648, 1.0,    /* diffuse */
                   0.628281, 0.555802, 0.366065, 1.0, /* specular */
                   50.0                               /* shininess */
    };

    float goldGlow[4] = {0.2, 0.1, 0.05, 1};

    glEnable( GL_COLOR_MATERIAL );
    glEnable(GL_LIGHTING);
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, gold );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, &gold[4] );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &gold[8] );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, goldGlow );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, gold[12] );

    float blue1[4] = { 0.4, 0.4, 0.6, 1 };
    float blue2[4] = { 0.0, 0, 0.8, 1 };
    float blue3[4] = { 0.0, 0, 0.15, 1 };
    float pos[4] = {2, 0, 0, 1};
    glEnable( GL_LIGHT0 );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, blue1 );
    glLightfv( GL_LIGHT0, GL_SPECULAR, blue2 );
    glLightfv( GL_LIGHT0, GL_AMBIENT, blue3 );
    glLightfv( GL_LIGHT0, GL_POSITION, pos);

    struct render_batch batch;
    render_batch_init(&batch);

    struct render_batch_element* element = render_batch_add(&batch);

    element->list = mesh_test->list;
    element->material = mesh_test->materials[0];

    struct Transform transform;
    transformInitIdentity(&transform);
    quatAxisAngle(&gUp, angle, &transform.rotation);

    transformToMatrix(&transform, element->transform);

    angle += 0.05f;

    render_batch_finish(&batch);
}

int main(void)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    gl_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();

    surface_t zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

    debug_init_usblog();
    console_set_debug(true);
    
    // struct controller_data ctrData;
    // bool wasStart = false;

    // for (;;) {
    //     controller_read(&ctrData);
    //     bool isStart = ctrData.c[0].start != 0;

    //     if (isStart && !wasStart) {
    //         break;
    //     }

    //     wasStart = isStart;
    // }

    setup();

    while(1) {
        surface_t* fb = display_get();

        if (fb) {
            rdpq_attach(fb, &zbuffer);

            gl_context_begin();

            render();

            gl_context_end();

            rdpq_detach_show();
        }  
    }
}