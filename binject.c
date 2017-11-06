
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "binject.h"

// --------------------------------------------------------------------------

struct binject_static_s {
  unsigned int tag_size;
  unsigned int data_size;
  unsigned int data_offset;
  char start_tag[1];
};

typedef struct {
  unsigned int tail_position;
  unsigned int len;
  unsigned int max;
  char raw[1];
} binject_data_t;

static long int binject_find_tag(FILE* f, const char* tag, size_t tagsize){
  int c = '\n';
  int count = 0;
  long match = 0;

  long int result = -1;
  if (tagsize <= 0) return result;

  while (!match) {

    c = fgetc(f);
    if (c == EOF) break;

    if (c != tag[count]) count = 0;
    else count += 1;

    if (count >= tagsize) {
      match = 1;
      break;
    }
  }

  if (match) result = ftell(f);
  if (result < 0) result = ACCESS_ERROR;

  return result;
}

static unsigned int container_size(binject_static_t * ds) {
  return ds->data_offset + ds->data_size;
}

static binject_error_t binject_write_data(binject_static_t * ds, FILE * file, long int position){
  long int result = NO_ERROR;

  if (0 != fseek(file, position, SEEK_SET)) result =  ACCESS_ERROR;
  if (result == NO_ERROR)
    if (container_size(ds) != fwrite(ds, 1, container_size(ds), file))
      result = ACCESS_ERROR;

  return result;
}

static int binject_tail_append(unsigned int * pos, const char * binary_path, const char * data, unsigned int size){
  FILE * f = fopen(binary_path, "r+b");
  if (!f) return ACCESS_ERROR;

  if (fseek(f, 0, SEEK_END)) return ACCESS_ERROR;
  if (pos && *pos == 0) {
    *pos = ftell(f);
    if (*pos < 0) return ACCESS_ERROR;
  }

  if (size != fwrite(data, 1, size, f))
      return ACCESS_ERROR;

  fclose(f);
  return NO_ERROR;
}

// --------------------------------------------------------------------

binject_error_t binject_inject(binject_static_t * ds, const char * path){

  FILE * file = fopen(path, "r+b");
  if (!file) return ACCESS_ERROR;
  
  long int position = binject_find_tag(file, ds->start_tag, ds->tag_size)
    - ds->tag_size - offsetof(binject_static_t, start_tag);

  binject_error_t result = INVALID_RESOURCE_ERROR;
  if (position > 0) result = binject_write_data(ds, file, position);
  fclose(file);

  return result;
}

void * binject_data(binject_static_t * ds){
  return ((char*)ds) + ds->data_offset;
}

// --------------------------------------------------------------------

static void binject_use_tail(binject_static_t * DS) {
  binject_data_t * toinj = (binject_data_t *)binject_data(DS);
  toinj->max = 0;
}

static int binject_does_use_tail(binject_static_t * DS) {
  binject_data_t * toinj = (binject_data_t *)binject_data(DS);
  if (toinj->max > 0) return 0;
  return 1;
}

int binject_step(binject_static_t * DS, const char * binary_path, const char * data, unsigned int r){
  binject_data_t * toinj = (binject_data_t *)binject_data(DS);

  if (binject_does_use_tail(DS)) {
      // Tail mode
      return binject_tail_append(0, binary_path, data, r);

  } else {
    if ((long)toinj->len + (long)r < (long)toinj->max-1) {
      // Static arry mode
      memcpy(toinj->raw + toinj->len, data, r);
      toinj->len += r;
      return NO_ERROR;

    } else {
      // Switch to tail mode
      binject_use_tail(DS);
      unsigned int * pos = &(( (binject_data_t *) binject_data(DS) ) -> tail_position);
      if (*pos > 0) pos = NULL;
      int result = binject_tail_append(pos, binary_path, toinj->raw, toinj->len);
      if (NO_ERROR != result) return result;
      return binject_tail_append(pos, binary_path, data, r);
    }
  }
}

int binject_done(binject_static_t * DS, const char * binary_path){
  return binject_inject(DS, binary_path);
}

char * binject_info(binject_static_t * DS, unsigned int * script_size, unsigned int * file_offset){

  if (script_size) *script_size = 0;
  if (file_offset) *file_offset = 0;

  binject_data_t * data = (binject_data_t*) binject_data(DS);
  if (binject_does_use_tail(DS)) {

    if (file_offset) *file_offset = data->tail_position;
    return NULL;

  } else {
    if (script_size) *script_size = data->len;
    return data->raw;
  }
}

// --------------------------------------------------------------------

int binject_aux_tail_get(const char * binary_path, char * buffer, unsigned int size, unsigned int offset){

  // Open file
  FILE * f = fopen(binary_path, "rb");
  if (!f) return ACCESS_ERROR;
  if (0 != fseek(f, offset, SEEK_SET)) return ACCESS_ERROR;

  // Read data
  int actread = fread(buffer, 1, size, f);
  if (0> actread && actread != size) return ACCESS_ERROR;

  // Calc remaining bytes
  if (0 != fseek(f, 0, SEEK_END)) return ACCESS_ERROR;
  int result = ftell(f) - offset - actread;

  // TODO :  close file on error ? it could override errno !
  fclose(f);
  return result;
}

int binject_aux_file_copy(const char * src, const char * dst){
  int r = 0;
  int w = 0;
  char b[128];

  // Open files
  FILE * fs = fopen(src, "rb");
  if (!fs) return ACCESS_ERROR;
  FILE * fd = fopen(dst, "wb");
  if (!fd) return ACCESS_ERROR;

  // Copy from source file to destination
  while (1) {
    r = fread(b, 1, sizeof(b), fs);
    if (0 > r) break;
    w = fwrite(b, 1, r, fd);
    if (r != sizeof(b) || r != w) break;
  }

  // Error report
  if (r != w) return ACCESS_ERROR;

  // TODO :  close file on error ? it could override errno !
  fclose(fd);
  fclose(fs);
  return NO_ERROR;
}

// --------------------------------------------------------------------

