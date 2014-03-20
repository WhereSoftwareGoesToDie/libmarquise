#ifndef H_MARQUISE_DEFER
#define H_MARQUISE_DEFER

#include "structs.h"

void marquise_defer_to_file(deferral_file * df, data_burst * burst);

data_burst *marquise_retrieve_from_file(deferral_file * df);

deferral_file *marquise_deferral_file_new();

/* Close and delete the deferral file. Returns 0 on success, -1 on
 * error. If -1, will set errno according to unlink(). */
int marquise_deferral_file_close(deferral_file * df);

void marquise_deferral_file_free(deferral_file * df);

extern const char *const deferral_file_template;
#endif
