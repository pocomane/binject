
// NOTE: The code is still a bit messy, but it works.
// TODO : CLEAN UP !

#include "binject.h"
#include <stdio.h>
#include <string.h>
#include "unistd.h"

// ---------------------------------------------------------------------------------
// -- Manual config - Defaults can be overwritten in an external header

// If the INTERNAL ARRAY mechanism was chosen, the script size will be kept
// in the following (scanf) format. Note: A size string starting with
// '\0' means no script in the array!
// TODO : define the data type also (for more coherence in config) ???
#ifndef BINJECT_ARRAY_SIZE_FORMAT
#define BINJECT_ARRAY_SIZE_FORMAT "%u"
#endif // BINJECT_ARRAY_SIZE_FORMAT

// If the "Append" mechanism was chosen, this will be placed between the binary
// and the script. It will be checked to detect the start of the script.
#ifndef BINJECT_TAIL_TAG_EDGE
#define BINJECT_TAIL_TAG_EDGE "\0THE\0TAIL\0SCRIPT\0IS\0JUST\0AFTER\0THESE\0TWO\0IDENTICAL\0TAGS\n"
#endif // BINJECT_TAIL_TAG_EDGE

// ---------------------------------------------------------------------------------
// -- Verbosity selection

// levels > 9 are for debug.
// default = 0 = do not print nothing about the "Normal" operation

#ifndef VERB_LEVEL
#define VERB_LEVEL 0
#endif

#define verbprint(D, ...) do{ \
  if (VERB_LEVEL > D) { \
    if (VERB_LEVEL > 9) \
      printf("DEBUG at %d: ", __LINE__); \
    printf(__VA_ARGS__); \
    fflush(stdout);\
  } \
}while(0)

// ---------------------------------------------------------------------------------
// -- Utility stuff

typedef struct {
  FILE * binary;
  FILE * out;
  char * path;
  int aux_counter;
  int script_offset;
  int script_size;
} private_info_t;

static private_info_t* private_info(binject_info_t * info) {
  return (private_info_t*)&(info->hidden);
}

#define PRINT_MESSAGE(I, ...) do{ \
  binject_info_t * macro_protect_I = (I); \
  snprintf(macro_protect_I->last_message, \
    sizeof(macro_protect_I->last_message)-1, \
    __VA_ARGS__); \
}while(0)

#define ERRWRAP(C, I, E, M, N) do{ \
  binject_info_t * macro_protect_I = (I); \
  if (C) { \
    macro_protect_I->last_error = -__LINE__; \
    goto error; \
  } \
  macro_protect_I->last_error = BINJECT_OK; \
}while(0)

#define FILE_OPEN(I, P, M, F) ERRWRAP( \
  (*(F) = fopen(P, M)) == 0, \
  I, BINJECT_ERROR_OPEN, "while opening", P \
)

#define FILE_TELL(I, F, P, N) ERRWRAP( \
  (*(P) = ftell(F)) < 0, \
  I, BINJECT_ERROR_TELL, "while accessing", N \
)

#define FILE_GOTO(I, F, O, N) ERRWRAP( \
  fseek(F, O, SEEK_SET) != 0, \
  I, BINJECT_ERROR_SEEK, "while accessing", N \
)

#define FILE_GOTO_END(I, F, N) ERRWRAP( \
  fseek(F, 0, SEEK_END) != 0, \
  I, BINJECT_ERROR_SEEK, "while accessing", N \
)

#define FILE_READ(I, F, B, S, N) ERRWRAP( \
  fread(B, 1, S, F) != (S), \
  I, BINJECT_ERROR_READ, "while reading", N \
)

#define FILE_WRITE(I, F, B, S, N) ERRWRAP( \
  fwrite(B, 1, S, F) != (S), \
  I, BINJECT_ERROR_WRITE, "while writing", N \
)

#define FILE_PRINT(I, F, N, ...) ERRWRAP( \
  fprintf(F, __VA_ARGS__) <0, \
  I, BINJECT_ERROR_WRITE, "while writing", N \
)

static FILE * binject_copy_binary(binject_info_t * info, char * out_path) {
  private_info_t * pinfo = private_info(info);
  FILE * out = pinfo->out;
  verbprint(8, "General - Copyng binary %s into %s\n", pinfo->path, out_path);

  // TODO : CLOSE out FILE ON ERROR !!?

  // Prepare the buffer
  FILE_GOTO_END(info, pinfo->binary, 0);
  int binary_size = 0;
  FILE_TELL(info, pinfo->binary, &binary_size, 0);
  FILE_GOTO(info, pinfo->binary, 0, 0);
  int bufsize = binary_size + 1;
  { // Scope block to avoid goto and variable length issue

    // Copy the binary
    char buf[bufsize];
    FILE_READ(info, pinfo->binary, buf, binary_size, 0);
    FILE_WRITE(info, out, buf, binary_size, out_path);
  }

  // All is right
  pinfo->out = out;

  return out;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR;
  PRINT_MESSAGE(info, "[%d] Can not copy the binary\n", info->last_error);
  return 0;
}

// ---------------------------------------------------------------------------------
// -- Auxilary API Functions

#define COMPILE_TIME_CHECK(condition) ((void)sizeof(char[!(condition)?-1:1]))

void binject_binary_path(binject_info_t * info, char * path) {
  private_info_t * pinfo = private_info(info);

  // Global check. They must be in a function. Placed here to avoid
  // "Unused function warning" or ad-hoc function export
  COMPILE_TIME_CHECK(sizeof(binject_info_hidden_t) == sizeof(private_info_t));

  if (pinfo->binary) fclose(pinfo->binary);
  pinfo->binary = 0;
  pinfo->path = path;
  pinfo->script_offset = 0;
  pinfo->script_size = 0;
  info->last_error = BINJECT_OK;
  info->last_message[0] = '\0';
}

// ---------------------------------------------------------------------------------
// -- Append Mechanism - Read

static void binject_find_tail_tag(binject_info_t * info) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Tail Tag - searching script from the bottom\n");

  if (!pinfo->binary)
    FILE_OPEN(info, pinfo->path, "rb", &(pinfo->binary));

  if ('T' != script_array.mechanism[0]){
      info->last_error = -__LINE__;
      goto error;
  }

  int size = -1;
  int offset = -1;

  if (1 != sscanf(script_array.size, BINJECT_ARRAY_SIZE_FORMAT, &offset)){
      info->last_error = -__LINE__;
      goto error;
  }

  FILE_GOTO_END(info, pinfo->binary, 0);
  FILE_TELL(info, pinfo->binary, &size, 0);
  size -= offset;

  info->mecha = BINJECT_TAIL_TAG;
  pinfo->script_offset = offset;
  pinfo->script_size = size;

  verbprint(8, "Tail Tag - Set current mode\n");
  return;

error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_NO_SCRIPT;
  pinfo->script_size = 0;
  pinfo->script_offset = 0;
  PRINT_MESSAGE(info, "[%d] Invalid Tail Tag\n", info->last_error);
  return;
}

size_t binject_tail_tag_read(binject_info_t* info, char * buffer, size_t maximum) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Tail Tag - Reading a chunk\n");

  // TODO : test multstep data read !!!!
  // TODO : test attempt to read more data !!!!

  if (!buffer || maximum<=0) {
    if (pinfo->script_size <= 0) {
      info->last_error = -__LINE__;
      goto error;
    }
    return pinfo->script_size;
  }

  int position = pinfo->script_offset;
  if (position <= 0) {
    info->last_error = -__LINE__;
    goto error;
  }

  // Go to the script offset
  if (pinfo->aux_counter < position)
    pinfo->aux_counter = position;
  FILE_GOTO(info, pinfo->binary, pinfo->aux_counter, 0);

  // Load the script
  if (maximum < pinfo->script_size) maximum = pinfo->script_size;
  FILE_READ(info, pinfo->binary, buffer, maximum, 0);
  pinfo->aux_counter += maximum;

  return maximum;

error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_READ;
  PRINT_MESSAGE(info, "[%d] Can not read the script\n", info->last_error);
  return 0;
}

// ---------------------------------------------------------------------------------
// -- Append Mechanism - Write

static void binject_inject_tail_tag_start(binject_info_t * info, char * scr_path, char * out_path){
  private_info_t * pinfo = private_info(info);
  FILE* out = pinfo->out;
  const int tag_size = sizeof(BINJECT_TAIL_TAG_EDGE) - 1;
  verbprint(8, "Tail Tag - First write: appending %d byte tag\n", tag_size);

  // TODO : remove out_path argument ???
  // TODO : CLOSE out FILE ON ERROR !!?

  FILE_GOTO_END(info, out, 0);

  // Repeat two time to avoid false match in the .data section
  FILE_WRITE(info, out, BINJECT_TAIL_TAG_EDGE, tag_size, out_path);
  FILE_WRITE(info, out, BINJECT_TAIL_TAG_EDGE, tag_size, out_path);

  pinfo->aux_counter = 0;
  info->last_error = BINJECT_OK;
  
  return;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR;
  PRINT_MESSAGE(info, "[%d] Can not start the injection\n", info->last_error);
  return;
}

static size_t binject_inject_tail_tag_write(binject_info_t * info, const char * buffer, size_t size) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Tail Tag - Write a %d byte chunk\n", (int)size);

  // TODO : test multstep data injection !!!!
  // TODO : test attempt to write more data !!!!

  if (!buffer || size<=0) goto error;

  FILE * out = pinfo->out;

  FILE_WRITE(info, out, buffer, size, out_path);

  return size;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_WRITE;
  PRINT_MESSAGE(info, "[%d] Can not inject script chunk\n", info->last_error);
  return 0;
}

static void binject_inject_tail_tag_close(binject_info_t * info, char * scr_path, char * out_path){
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Tail Tag - Finalizing a %d byte script\n", pinfo->aux_counter);

  // TODO : test multstep data injection !!!!
  // TODO : test attempt to write more data !!!!

  // TODO : CLOSE out FILE ON ERROR !!!  

  // Store the script offset
  FILE * out = pinfo->out;
  FILE_GOTO_END(info, pinfo->binary, 0);
  int binary_size = 0;
  FILE_TELL(info, pinfo->binary, &binary_size, 0);
  FILE_GOTO(info, pinfo->binary, 0, 0);
  int script_end;
  FILE_TELL(info, out, &script_end, out_path); 
  const int tag_size = sizeof(BINJECT_TAIL_TAG_EDGE) - 1;

  info->last_error = BINJECT_OK;

  int max = pinfo->script_offset - (script_array.empty - script_array.size);
  FILE_GOTO(info, pinfo->out, max, 0);
  FILE_PRINT(info, pinfo->out, out_path, BINJECT_ARRAY_SIZE_FORMAT, binary_size + 2 * tag_size);
  FILE_GOTO(info, pinfo->out, pinfo->script_offset, 0);
  FILE_WRITE(info, pinfo->out, script_array.empty, sizeof(script_array.empty), out_path);
  FILE_GOTO(info, pinfo->out, max-2, 0);
  FILE_WRITE(info, pinfo->out, "T", 1, out_path);

  return;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR;
  PRINT_MESSAGE(info, "[%d] Can not finalize the injection\n", info->last_error);
  return;
}

// ---------------------------------------------------------------------------------
// -- Array Mechanism - Read

static void binject_find_array(binject_info_t * info) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Internal Array - searching for array boundary\n");

  // A size string starting with '\0' always means
  // no script in the array!
  if (script_array.size[0] == 0) goto error;

  if ('S' != script_array.mechanism[0]){
    info->last_error = -__LINE__;
    goto error;
  }

  unsigned int size;
  if (1 != sscanf(script_array.size, BINJECT_ARRAY_SIZE_FORMAT, &size)){
    info->last_error = -__LINE__;
    goto error;
  }

  // Zero size means no script
  if (size == 0) {
    info->last_error = -__LINE__;
    goto error;
  }

  info->mecha = BINJECT_INTERNAL_ARRAY;
  verbprint(8, "Internal Array - Set current mode\n");
  pinfo->script_size = size;
  info->last_error = BINJECT_OK;

  return;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_NO_SCRIPT;
  PRINT_MESSAGE(info, "[%d] Can not find the script\n", info->last_error);
  return;
}

size_t binject_array_read(binject_info_t* info, char * buffer, size_t maximum) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Internal Array - Reading a chunk\n");

  // TODO : TEST multistep read
  // TODO : TEST read more data

  size_t toread = sizeof(script_array.empty) - pinfo->aux_counter;
  if (toread <= 0) {
    info->last_error = BINJECT_ERROR;
    return 0;
  }
  if (toread > pinfo->script_size) toread = pinfo->script_size;
  if (!buffer || maximum <= 0) return toread;
  if (maximum < toread) toread = maximum;
  memcpy(buffer, script_array.empty, toread);
  pinfo->aux_counter += toread;

  return toread;
}

// ---------------------------------------------------------------------------------
// -- Array Mechanism - Write

static int binject_search_double_tag_forward(binject_info_t * info, const char *tag, int tag_size){

  private_info_t * pinfo = private_info(info);
  int match = 0;
  while (1) {

    // Get next char
    char c;
    FILE_READ(info, pinfo->binary, &c, 1, 0);

    // Select the first or second tag repetition character
    char m;
    if (match > tag_size-1) m = tag[match - tag_size];
    else m = tag[match];

    // If a match fails, it retry with the last char of the tag
    if (m != c) match = 0;

    // Match next char as needed
    if (m == c) match += 1;

    // Done !
    if (match > 2*tag_size-1)
      break;

  }

  return 0;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_NO_SCRIPT;
  PRINT_MESSAGE(info, "[%d] Can not find the script\n", info->last_error);
  return info->last_error;
}

static void binject_inject_array_start(binject_info_t * info, char * scr_path, char * out_path) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Internal Array - Check array boundary\n");

  FILE_GOTO(info, pinfo->binary, 0, 0);

  // Script Start tag
  if (binject_search_double_tag_forward(info, BINJECT_ARRAY_EDGE, sizeof(BINJECT_ARRAY_EDGE)-1))
    goto error;
  FILE_TELL(info, pinfo->binary, &(pinfo->script_offset), 0);

  if (pinfo->script_offset < 0) goto error;

  pinfo->aux_counter = 0;
  info->mecha = BINJECT_INTERNAL_ARRAY;
  verbprint(8, "Internal Array - Set current mode\n");

  return;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_READ;
  info->mecha = BINJECT_TAIL_TAG;
  pinfo->script_size = 0;
  pinfo->script_offset = 0;
  PRINT_MESSAGE(info, "[%d] Can not start injection\n", info->last_error);
  return;
}

static size_t binject_inject_array_write(binject_info_t * info, const char * buffer, size_t size) {
  private_info_t * pinfo = private_info(info);
  verbprint(8, "Internal Array - Write a %d byte chunk\n", (int)size);

  // TODO : test multstep data injection !!!!
  // TODO : test attempt to write more data !!!!

  //info->mecha = BINJECT_TAIL_TAG;
  // verbprint(8, "Tail Tag - Set current mode\n");

  if (pinfo->aux_counter + size < sizeof(script_array.empty)) { // maximum script size
    memcpy(script_array.empty + pinfo->aux_counter, buffer, size);
    pinfo->aux_counter += size;

  } else {
    // Switch to file append mode
    info->mecha = BINJECT_TAIL_TAG;
    verbprint(8, "Internal Array - Switching to Tail Tag mode\n");
    int aux = pinfo->aux_counter;
    binject_inject_tail_tag_start(info, "script", "output");
    binject_inject_tail_tag_write(info, script_array.empty, aux);
  }

  return size;
}

static void binject_inject_array_close(binject_info_t * info, char * scr_path, char * out_path) {
  private_info_t * pinfo = private_info(info);
  int max = pinfo->script_offset - (script_array.empty - script_array.size);
  verbprint(8, "Internal Array - Finalizing a %d byte script at 0x%x\n", pinfo->aux_counter, max);

  // TODO : test multstep data injection !!!!
  // TODO : test attempt to write more data !!!!

  FILE_GOTO(info, pinfo->out, max, 0);
  FILE_PRINT(info, pinfo->out, out_path, BINJECT_ARRAY_SIZE_FORMAT, pinfo->aux_counter);
  FILE_GOTO(info, pinfo->out, pinfo->script_offset, 0);
  FILE_WRITE(info, pinfo->out, script_array.empty, sizeof(script_array.empty), out_path);
  FILE_GOTO(info, pinfo->out, max-2, 0);
  FILE_WRITE(info, pinfo->out, "S", 1, out_path);

  return;
error:
  if (info->last_error == BINJECT_OK) info->last_error = BINJECT_ERROR_WRITE;
  PRINT_MESSAGE(info, "[%d] can not finalize injection\n", info->last_error);
  return;
}

// --------------------------------------------------------------------------------
// -- Mechanism Dispatch / API Functions

void binject_find(binject_info_t * info){
  binject_mechanism_t * m = &( info->mecha );

  if (BINJECT_INTERNAL_ARRAY == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_find_array(info);

  if (BINJECT_TAIL_TAG == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_find_tail_tag(info);

  if (info->last_error != BINJECT_OK){
    info->last_error = -__LINE__;
    PRINT_MESSAGE(info, "[%d] Can not find the script\n", info->last_error);
  }
  return;
}

int binject_error(binject_response_t e) {
  return e <= BINJECT_ERROR;
}

size_t binject_read(binject_info_t* info, char * buffer, size_t maximum) {
  binject_mechanism_t * m = &( info->mecha );
  size_t result;
  switch (*m) {

    break; case BINJECT_INTERNAL_ARRAY: 
      result = binject_array_read(info, buffer, maximum);
      if (info->last_error != BINJECT_OK) goto error;

    break; case BINJECT_TAIL_TAG:
      result = binject_tail_tag_read(info, buffer, maximum);
      if (info->last_error != BINJECT_OK) goto error;

    break; default: goto error;
  }

  return result;
error:
  if (info->last_error != BINJECT_OK)
    PRINT_MESSAGE(info, "[%d] Can not read the chunk\n", info->last_error);
  return 0;
}

void binject_inject_start(binject_info_t * info, char * scr_path, char * out_path){
  verbprint(8, "General - using %s as output file\n", out_path);
  private_info_t * pinfo = private_info(info);
  binject_mechanism_t * m = &( info->mecha );

  // Open the output file
  if (!pinfo->out)
    FILE_OPEN(info, out_path, "wb", &(pinfo->out));
  if (!binject_copy_binary(info, out_path)){
    // TODO : WRAP IN MACRO ???
    info->last_error = -__LINE__;
    // TODO : something else to do ??
    goto error;
  }
  FILE_GOTO_END(info, pinfo->out, 0);

  if (BINJECT_INTERNAL_ARRAY == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_inject_array_start(info, scr_path, out_path);

  if (BINJECT_TAIL_TAG == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_inject_tail_tag_start(info, scr_path, out_path);

error:
  if (info->last_error != BINJECT_OK)
    PRINT_MESSAGE(info, "[%d] Can not start injection\n", info->last_error);
  return;
}

size_t binject_write(binject_info_t * info, const char * buffer, size_t size) {
  binject_mechanism_t * m = &( info->mecha );
  size_t result = 0;

  if (BINJECT_INTERNAL_ARRAY == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    result = binject_inject_array_write(info, buffer, size);

  if (BINJECT_TAIL_TAG == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    result = binject_inject_tail_tag_write(info, buffer, size);

  if (info->last_error != BINJECT_OK)
    PRINT_MESSAGE(info, "[%d] Can not write the chunk\n", info->last_error);
  return result;
}

void binject_inject_close(binject_info_t * info, char * scr_path, char * out_path){
  binject_mechanism_t * m = &( info->mecha );
  private_info_t * pinfo = private_info(info);

  if (BINJECT_INTERNAL_ARRAY == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_inject_array_close(info, scr_path, out_path);

  if (BINJECT_TAIL_TAG == *m || BINJECT_MECHANISM_UNKNOWN == *m)
    binject_inject_tail_tag_close(info, scr_path, out_path);

  if (pinfo->out) {
    fclose(pinfo->out);
    pinfo->out = 0;
  }

  if (info->last_error != BINJECT_OK)
    PRINT_MESSAGE(info, "[%d] Can not finalize the injection\n", info->last_error);
  return;
}

