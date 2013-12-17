#include <stdio.h>
#include "structs.h"

void as_defer_to_file( deferral_file *df, data_burst *burst );
data_burst *as_retrieve_from_file( deferral_file *df );
deferral_file *as_deferral_file_new();
void as_deferral_file_close( deferral_file *df );
void as_deferral_file_free( deferral_file *df );

