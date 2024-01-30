#include "mesh.h"


void meshInit(struct Mesh* mesh) {
    mesh->list = glGenLists(1);
    glNewList(mesh->list, GL_COMPILE);

    

    glEndList();
}