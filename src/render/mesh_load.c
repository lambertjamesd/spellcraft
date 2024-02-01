#include "mesh_load.h"

#include <libdragon.h>
#include <GL/gl.h>

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "./coloru8.h"

// MESH
#define EXPECTED_HEADER 0x4D455348

bool mesh_load(struct Mesh* into, const char* path) {
    FILE* meshFile = asset_fopen(path, NULL);

    int header;
    fread(&header, 1, 4, meshFile);
    assert(header == EXPECTED_HEADER);

    uint16_t submeshCount;
    fread(&submeshCount, 2, 1, meshFile);
    meshInit(into, submeshCount);

    for (uint16_t i = 0; i < submeshCount; ++i) {

        uint16_t strLength;
        fread(&strLength, 2, 1, meshFile);
        char materialName[strLength];
        fread(materialName, 1, strLength, meshFile);
        
        // TODO load material

        uint16_t attributes;
        fread(&attributes, 2, 1, meshFile);

        assert(attributes <= MeshAttributesAll);

        GLuint vertexBuffer;
        GLuint vertexArray;
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glGenBuffersARB(1, &vertexBuffer);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);

        int offset = 0;
        int size = meshAttributeSize(attributes);

        if (attributes & MeshAttributesPosition) {
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_HALF_FIXED_N64, size, (void*)offset);

            offset += sizeof(short) * 3;
        }

        if (attributes & MeshAttributesUV) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, size, (void*)offset);

            offset += sizeof(struct Vector2);
        }

        if (attributes & MeshAttributesColor) {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(4, GL_BYTE, size, (void*)offset);

            offset += sizeof(struct Coloru8);
        }

        if (attributes & MeshAttributesNormal) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_BYTE, size, (void*)offset);

            offset += sizeof(char) * 3;
        }

        uint16_t vertexCount;
        fread(&vertexCount, 2, 1, meshFile);

        glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexCount * size, NULL, GL_STATIC_DRAW_ARB);
        void* vertices = glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
        fread(vertices, size, vertexCount, meshFile);
        glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

        int indexSize = vertexCount >= 256 ? 2 : 1;

        uint16_t indexCount;
        fread(&indexCount, 2, 1, meshFile);

        GLuint indexBuffer;
        glGenBuffersARB(1, &indexBuffer);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, indexBuffer);
        
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, indexCount * indexSize, NULL, GL_STATIC_DRAW_ARB);
        void* indices = glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
        fread(indices, indexSize, indexBuffer, meshFile);
        glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

        glBindVertexArray(0);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

        glNewList(into->list + i, GL_COMPILE);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexBuffer);
        glBindVertexArray(vertexArray);
        glDrawElements(GL_TRIANGLES, indexCount, indexSize == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        glEndList();
    }

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindVertexArray(0);

    return true;
}