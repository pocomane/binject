
#ifndef BINJECT_H
#define BINJECT_H

// -------------------------------------------------------------------------------
// -- Manual configuration - set preferences here -- more info in the source file

// Enable the main function in the binject source
#define BINJECT_MAIN_APP 1

// If the main function is enabled, this callback will be called to handle
// the script. It must be a function with the following prototype:
//   int my_run_callback(binject_info_t * info, int argc, char **argv)
// If not defined an internal one will be used: it will just print the script
// to the stdout
//#define BINJECT_SCRIPT_HANDLER my_run_callback

// If the main function is enabled, this callback will be called to inject
// the script. It must be a function with the following prototype:
//   int my_inj_callback(binject_info_t * info, int argc, char **argv)
// If not defined an internal one will be used to just copy the file at argv[1]
//#define BINJECT_SCRIPT_INJECT my_inj_callback

// -------------------------------------------------------------------------------
// -- Export library function

#include <stdio.h>

typedef enum {
  BINJECT_OK = 0,
  BINJECT_UNKNOWN = -1,
  BINJECT_ERROR = -2,
  BINJECT_ERROR_OPEN = -3,
  BINJECT_ERROR_SEEK = -4,
  BINJECT_ERROR_TELL = -5,
  BINJECT_ERROR_WRITE = -6,
  BINJECT_ERROR_READ = -7,
  BINJECT_ERROR_NO_SCRIPT = -8,
  BINJECT_ERROR_NO_TAG = -9,
} binject_response_t;


typedef enum {
  BINJECT_MECHANISM_UNKNOWN = 0,
  BINJECT_INTERNAL_ARRAY,
  BINJECT_TAIL_TAG,
} binject_mechanism_t;

#define MAX_ERROR_LEN 256

typedef union {
  void * force_align;
  char raw[sizeof(struct{
    void*a; void*b; void* c; int d; int e; int f;
  })];
} binject_info_hidden_t;

typedef struct {
  binject_info_hidden_t hidden;
  binject_mechanism_t mecha;
  binject_response_t last_error;
  char last_message[MAX_ERROR_LEN];
} binject_info_t;

#define BINJECT_INIT {0}

static int binject_error(binject_response_t e) { return e <= BINJECT_ERROR; }

// NOTE: path MUST outlive info (path pointer is stored inside info) 
void binject_binary_path(binject_info_t * info, char * path);

void binject_find(binject_info_t * info);
void binject_inject_start(binject_info_t * info, char * scr_path, char * out_path);
void binject_inject_close(binject_info_t * info, char * scr_path, char * out_path);

// If buffer==0 it returns 0:buffer_full, >0:suggested_data_size_to_write
// If a valid buffer/size is provided, it returns the size of written data
// On Error info->last_error will be set (and the return will have no meaning)
size_t binject_write(binject_info_t * info, const char * buffer, size_t size);

// If buffer==0/size==0 it returns 0:no_data_to_read, >0:size_of_readable_data
// If a valid buffer/size is provided, it returns the size of read data.
// On Error info->last_error will be set (and the return will have no meaning)
size_t binject_read(binject_info_t* info, char * buffer, size_t maximum);

// Set the verbosity level. >9 are for debug.
void binject_set_verbosity(int d);

#endif // BINJECT_H

