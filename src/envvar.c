#include <stdlib.h>
#include <errno.h>

#include "envvar.h"
#include "marquise.h"

char get_envvar_int(const char *name, int *v)
{
	char *var = getenv(name);
	if (!var) {
		return 0;
	}
	errno = 0;
	*v = strtol(var, NULL, 10);
	if (errno != 0 || *v == 0) {
		return -1;
	}
	return 1;
}

int get_collator_max_messages()
{
	int v;
	int ret = get_envvar_int("LIBMARQUISE_COLLATOR_MAX_MESSAGES", &v);
	if (ret > 0) {
		return v;
	}
	return COLLATOR_MAX_MESSAGES;
}

const char *get_deferral_dir()
{
	const char *dir = getenv("LIBMARQUISE_DEFERRAL_DIR");
	if (!dir) {
		return DEFAULT_DEFERRAL_DIR;
	}
	return dir;
}
