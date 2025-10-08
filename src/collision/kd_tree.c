#include "kd_tree.h"

#include <assert.h>

#define MAX_DEPTH   16

uint16_t kd_tree_transform_value(float min, float size_inv, float input) {
    float result = size_inv * (input - min);

    if (result > 0xFFFF) {
        return 0xFFFF;
    } 
    if (result < 0) {
        return 0;
    }
    return (uint16_t)result;
}

void kd_tree_transform_arr(kd_tree_t* tree, float* input, uint16_t* output) {
    float* min = VECTOR3_AS_ARRAY(&tree->min);
    float* size_inv = VECTOR3_AS_ARRAY(&tree->size_inv);
    for (int i = 0; i < 3; i += 1) {
        output[i] = kd_tree_transform_value(min[i], size_inv[i], input[i]);
    }
}

void kd_tree_lookup(kd_tree_t* tree, struct Box3D* box, void *data, kd_triangle_callback callback, int collision_layers) {
    kd_tree_node_t nodes[MAX_DEPTH];
    kd_tree_node_t* curr = &nodes[0];
    *curr = *(kd_tree_node_t*)tree->nodes;

    uint16_t box_min[3];
    uint16_t box_max[3];

    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(&box->min), box_min);
    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(&box->max), box_max);

    while (curr >= nodes) {
        switch (curr->branch.node_type) {
            case KD_TREE_LEAF_NODE: {
                int max_index = curr->leaf.triangle_offset + curr->leaf.triangle_count;

                for (int i = curr->leaf.triangle_offset; i < max_index; i += 1) {
                    callback(tree, data, i, collision_layers);
                }

                --curr;
                break;
            }
            case KD_TREE_BRANCH_NODE: {
                kd_tree_node_t* next = curr + 1;
                curr->branch.node_type = KD_TREE_BRANCH_SECOND_NODE;

                assert (next < &nodes[MAX_DEPTH]);

                if (curr->branch.a_max >= box_min[curr->branch.axis]) {
                    *next = *(kd_tree_node_t*)((char*)tree->nodes + curr->branch.a_offset);
                    ++curr;
                }
                break;
            }
            case KD_TREE_BRANCH_SECOND_NODE:
                if (curr->branch.b_min <= box_max[curr->branch.axis]) {
                    *curr = *(kd_tree_node_t*)((char*)tree->nodes + curr->branch.b_offset);
                } else {
                    --curr;
                }
                break;
        } 
    }
}

bool mesh_triangle_shadow_cast(struct mesh_triangle_indices indices, struct Vector3* vertices, struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

bool kd_tree_shadow_cast(kd_tree_t* tree, struct Vector3* starting_point, struct mesh_shadow_cast_result* result) {
    kd_tree_node_t nodes[MAX_DEPTH];
    kd_tree_node_t* curr = &nodes[0];
    *curr = *(kd_tree_node_t*)tree->nodes;

    uint16_t pos_local[3];

    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(starting_point), pos_local);
    bool has_hit = false;

    while (curr >= nodes) {
        switch (curr->branch.node_type) {
            case KD_TREE_LEAF_NODE: {
                int max_index = curr->leaf.triangle_offset + curr->leaf.triangle_count;

                for (int i = curr->leaf.triangle_offset; i < max_index; i += 1) {
                    struct mesh_shadow_cast_result check;
                    if (mesh_triangle_shadow_cast(tree->indices[i], tree->vertices, starting_point, &check)) {
                        if (!has_hit || result->y > check.y) {
                            *result = check;
                            has_hit = true;
                        }
                    }
                }

                --curr;
                break;
            }
            case KD_TREE_BRANCH_NODE: {
                kd_tree_node_t* next = curr + 1;
                curr->branch.node_type = KD_TREE_BRANCH_SECOND_NODE;

                assert (next < &nodes[MAX_DEPTH]);

                if (curr->branch.axis == 1 || pos_local[curr->branch.axis] <= curr->branch.a_max) {
                    *next = *(kd_tree_node_t*)((char*)tree->nodes + curr->branch.a_offset);
                    ++curr;
                }
                break;
            }
            case KD_TREE_BRANCH_SECOND_NODE:
                if (pos_local[curr->branch.axis] >= curr->branch.b_min) {
                    *curr = *(kd_tree_node_t*)((char*)tree->nodes + curr->branch.b_offset);
                } else {
                    --curr;
                }
                break;
        } 
    }

    return has_hit;
}