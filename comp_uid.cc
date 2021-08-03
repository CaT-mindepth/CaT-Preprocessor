#include "comp_uid.h"

// TODO: see comp_uid.h.

static unsigned _domino_CaT_component_UID = 0;

// Retrieves a fresh component UID.
unsigned get_comp_uid() {
    return _domino_CaT_component_UID++;
}

// init or reset component UID to a larger value.
void init_comp_uid(unsigned s) {
    _domino_CaT_component_UID = s;
}