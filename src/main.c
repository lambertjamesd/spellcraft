#include <stdio.h>


#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "render/mesh_load.h"

#include <libdragon.h>

struct Mesh mesh_test;

void setup() {
    mesh_load(&mesh_test, "rom:/meshes/test.mesh");
}

void render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    float aspect_ratio = (float)display_get_width() / (float)display_get_height();
    float near_plane = 1.0f;
    float far_plane = 50.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    glCallList(mesh_test.list);
}

int main(void)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    gl_init();
    dfs_init(DFS_DEFAULT_LOCATION);

    surface_t zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

    debug_init_usblog();
    console_set_debug(true);

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