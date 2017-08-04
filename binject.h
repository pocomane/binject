
#ifndef BINJECT_H
#define BINJECT_H

// -------------------------------------------------------------------------------
// -- Manual configuration - set preferences here -- more info in the source file

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

// If the INTERNAL ARRAY mechanism was chosen, the script will be kept in
// in a string with the following length. It should be
// compatible with BINJECT_ARRAY_SIZE_FORMAT (see the source file)
#ifndef BINJECT_ARRAY_SIZE_FORMAT_LENGTH
#define BINJECT_ARRAY_SIZE_FORMAT_LENGTH 32
#endif // BINJECT_ARRAY_SIZE_FORMAT_LENGTH

// If the INTERNAL ARRAY mechanism was chosen, this will be placed between the
// binary and the script. It will be checked durinng the execution to detect the
// start of the script.
#ifndef BINJECT_ARRAY_EDGE
#define BINJECT_ARRAY_EDGE "\nTHE\0ARRAY\0SCRIPT\0STARTS\0JUST\0AFTER\0THESE\0TWO\0IDENTICAL\0TAGS\n"
#endif // BINJECT_ARRAY_EDGE

// Size of the data for the INTERNAL ARRAY mechanism. It should be
// a positive integer
#ifndef BINJECT_ARRAY_SIZE
#define BINJECT_ARRAY_SIZE (9216)
#endif // BINJECT_ARRAY_SIZE

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
    void* v[4];
    int i[3];
    binject_mechanism_t m;
  })];
} binject_info_hidden_t;

typedef struct {
  char mechanism[2];
  char size[BINJECT_ARRAY_SIZE_FORMAT_LENGTH];
  char edge[2*sizeof(BINJECT_ARRAY_EDGE)-2];
  char empty[BINJECT_ARRAY_SIZE];
} script_array_t;

#define BINJECT_STATIC_DATA { \
  .mechanism = "\0", \
  .size = "0", \
  .edge = BINJECT_ARRAY_EDGE BINJECT_ARRAY_EDGE, \
}

typedef struct {
  binject_info_hidden_t hidden;
  binject_response_t last_error;
  char last_message[MAX_ERROR_LEN];
} binject_info_t;

int binject_error(binject_response_t e);

// NOTE: static_data must be static defined to be equal to BINJECT_STATIC_DATA
// NOTE: path MUST outlive info (path pointer is stored inside info) 
binject_info_t binject_info_init(script_array_t * static_data, char * path);

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

