#include "repair_part.h"    
    
void repair_part_init(repair_part_t* repair_part, struct repair_part_definition* definition, entity_id entity_id) {
    repair_part->position = definition->position;
}

void repair_part_destroy(repair_part_t* repair_part, struct repair_part_definition* definition) {
    
}

void repair_part_common_init() {

}

void repair_part_common_destroy() {

}
