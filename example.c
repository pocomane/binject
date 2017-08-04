
#include "binject.h"
#include <stdio.h>
#include <string.h>
#include "unistd.h"

script_array_t script_array = BINJECT_STATIC_DATA;

static int print_help(const char * command){
  printf("\nUsage:\n  %s script.txt\n\n", command);
  printf("script.txt.exe executable will be generated or overwritten.\n");
  printf("script.txt.exe will print to the stdout an embedded copy of script.txt.\n\n");
  printf("NOTE: depending on the chosen embedding mechanism, some help information will be\n");
  printf("appended at end of script.txt.exe.\n");
  return 0;
}

void binject_main_app_internal_script_inject(binject_info_t * info, int argc, char **argv){

  if (argc < 2) goto error;
  char * scr_path = argv[1];

  // Open the scipt
  FILE * scr = fopen(scr_path, "rb");
  if (!scr) goto error;

  if (fseek(scr, 0, SEEK_END)) goto error;
  int siz = ftell(scr);
  if (siz < 0) goto error;
  if (fseek(scr, 0, SEEK_SET)) goto error;

  int bufsize = siz;
  { // Scope block to avoid goto and variable length issue
    char buf[bufsize];

    // Read the script
    fread(buf, 1, siz, scr);
    fclose(scr);

    // Copy the script
    binject_write(info, buf, siz);

    info->last_error = BINJECT_OK;
  }

  return;
error:
  info->last_error = BINJECT_ERROR;
  snprintf(info->last_message, sizeof(info->last_message)-1,
    "error reading the script %s\n", scr_path);
  return;
}

static int binject_main_app_internal_script_handle(binject_info_t * info, int argc, char **argv) {

  size_t script_size = binject_read(info, 0,0);
  if (script_size < 0)
    return -1;
  char script[script_size];
  size_t status = binject_read(info, script, script_size);
  if (binject_error(status) || script_size != status)
    return -1; // TODO : proper error handle

  printf("A %d byte script was found (dump:)[", (int)script_size);
  fwrite(script, 1, script_size, stdout);
  printf("]\n");
  return 0;
}

int main(int argc, char **argv) {
  int result = 0;

  // Open the binary
  binject_info_t info = binject_info_init(&script_array, argv[0]);
  if (info.last_error) {
    printf("%s", info.last_message);
    return -1;
  }

  binject_find(&info);

  // Run the proper tool
  if (info.last_error == BINJECT_OK) {
    result = binject_main_app_internal_script_handle(&info, argc, argv);

  } else if (argc < 2 || argv[1][0] == '\0') {
    print_help(argv[0]);
    info.last_error = BINJECT_OK;

  } else {

    // TODO : handle command line arguments !!!

    // Calculate the output path 
    const int pathlen = strlen(argv[1]);
    char OUTPUT_FILE[pathlen+10];
    strncpy(OUTPUT_FILE, argv[1], pathlen);
    strncpy(OUTPUT_FILE + pathlen, ".exe", 4);
    OUTPUT_FILE[pathlen+4] = '\0';

    // Inject !
    binject_inject_start(&info, argv[1], OUTPUT_FILE);
    binject_main_app_internal_script_inject(&info, argc, argv); // TODO : HANDLE ERROR ?!!
    binject_inject_close(&info, argv[1], OUTPUT_FILE);
  }

  if (info.last_error != BINJECT_OK) {
    fprintf(stderr, "Error %s\n", info.last_message);
    return info.last_error;
  } else if (result) {
    fprintf(stderr, "Error [%d] generic\n", -__LINE__);
  }
  return result;
}

