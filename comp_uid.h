
#ifndef COMP_UID_H_
#define COMP_UID_H_

// TODO: this pass keeps track of uid generation
// for each Component class. This isn't a very elegant approach;
// we shall try to merge it with domino's default UID management later.

// Retrieves a fresh component UID.
unsigned get_comp_uid();

// Init or (re)-set the UID to a higher value.
void init_comp_uid(unsigned s);

#endif // COMP_UID_H_