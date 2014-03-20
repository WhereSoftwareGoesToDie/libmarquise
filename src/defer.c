#include "structs.h"
#include "macros.h"
#include "defer.h"
#include "envvar.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// All we can do on failure to read a file is swap out the file under the user
// and pray that this one works better. Weird and dodgy. Yes, we lose all the
// data. It was lost anyway. At least this way we can save more datas.
#define recover_if( assertion, ... ) do {                               \
        fail_if( assertion                                              \
               , marquise_deferral_file_close( df );                          \
                 deferral_file *new = marquise_deferral_file_new();           \
                 free( df->path );                                      \
                 *df = *new;                                            \
                 free( new );                                           \
               , "marquise_defer_to_file recovered: %s", strerror( errno ) ); \
        } while( 0 )

const char *const deferral_file_template = "/marquise_defer_file_XXXXXX";

void marquise_defer_to_file(deferral_file * df, data_burst * burst)
{
	struct stat sb;
	recover_if(stat(df->path, &sb) == -1);
	recover_if(fseek(df->stream, 0, SEEK_END));
	recover_if(!fwrite(burst->data, burst->length, 1, df->stream));
	recover_if(!fwrite
		   (&burst->length, sizeof(burst->length), 1, df->stream));
}

// Just log, return NULL and get on with your life.
#define bail_if( assertion, action) do {                                  \
        fail_if( assertion                                                \
               , { action }; return NULL;        \
               , "marquise_retrieve_from_file failed: %s", strerror( errno ) ); \
        } while( 0 )

data_burst *marquise_retrieve_from_file(deferral_file * df)
{

	// Broken file means no bursts left.
	struct stat sb;
	bail_if(stat(df->path, &sb) == -1,);

	// Empty file means no bursts left.
	bail_if(fseek(df->stream, 0, SEEK_END),);
	if (ftell(df->stream) <= 0)
		return NULL;

	size_t length;
	// Grab the length of the burst from the end of the file
	bail_if(fseek(df->stream, -(sizeof(size_t))
		      , SEEK_END),);
	bail_if(!fread(&length, sizeof(length)
		       , 1, df->stream),);

	// Now we seek back to the beginning of the burst.
	bail_if(fseek(df->stream, -((long)length + sizeof(length))
		      , SEEK_END),);

	// Which is where the file will need to be truncated.
	long chop_at = ftell(df->stream);
	bail_if(chop_at == -1,);

	data_burst *burst = malloc(sizeof(data_burst));
	bail_if(!burst,);

	burst->length = length;
	burst->data = malloc(burst->length);
	bail_if(!burst->data, free(burst);
	    );

	bail_if(!fread(burst->data, 1, burst->length, df->stream)
		, free_databurst(burst);
	    );

	// And chop off the end of the file.
	bail_if(ftruncate(fileno(df->stream), chop_at)
		, free_databurst(burst);
	    );

	return burst;
}

deferral_file *marquise_deferral_file_new()
{
	deferral_file *df = malloc(sizeof(deferral_file));
	if (!df)
		return NULL;

	const char *template = deferral_file_template;
	const char *defer_dir = get_deferral_dir();
	char *file_path = malloc(strlen(template) + strlen(defer_dir) + 1);
	if (!file_path) {
		free(df);
		return NULL;
	}

	strcpy(file_path, defer_dir);
	strcat(file_path, template);
	int fd = mkstemp(file_path);
	if (fd == -1)
		goto fail_outro;

	bail_if (flock(fd, LOCK_EX | LOCK_NB) != 0,);
	
	df->fd = fd;	
	df->path = file_path;
	df->stream = fdopen(fd, "w+");
	if (!df->stream)
		goto fail_outro;

	return df;

 fail_outro:
	free(df);
	free(file_path);
	return NULL;
}

int marquise_deferral_file_close(deferral_file * df)
{
	fclose(df->stream);
	return unlink(df->path);
}

void marquise_deferral_file_free(deferral_file * df)
{
	flock(df->fd, LOCK_UN);
	free(df->path);
	free(df);
}
