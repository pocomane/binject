
#include "binject.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "unistd.h"

// Size of the data for the INTERNAL ARRAY mechanism. It should be
// a positive integer
#ifndef BINJECT_ARRAY_SIZE
#define BINJECT_ARRAY_SIZE (9216)
#endif // BINJECT_ARRAY_SIZE

static void error_report(int internal_error) {
  if (internal_error != NO_ERROR)
    fprintf(stderr, "Error %d\n", internal_error);
  if (0 != errno)
    fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
}

BINJECT_STATIC_STRING("```replace_data```", BINJECT_ARRAY_SIZE, static_data);

static int print_help(const char * command){
  printf("\nUsage:\n  %s script.txt\n\n", command);
  printf("script.txt.exe executable will be generated or overwritten.\n");
  printf("script.txt.exe will print to the stdout an embedded copy of script.txt.\n\n");
  printf("NOTE: depending on the chosen embedding mechanism, some help information will be\n");
  printf("appended at end of script.txt.exe.\n");
  return 0;
}

int binject_main_copy(const char * src, const char * dst){
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
  error_report(0);
  fclose(fd);
  fclose(fs);
  if (r != w) return ACCESS_ERROR;
  return NO_ERROR;
}

int binject_main_app_internal_script_inject(binject_static_t * info, const char * scr_path, const char* bin_path, const char * outpath){
  int result = ACCESS_ERROR;

  // Open the scipt
  FILE * scr = fopen(scr_path, "rb");
  if (!scr) goto end;

  // Get the original binary size
  if (fseek(scr, 0, SEEK_END)) goto end;
  int siz = ftell(scr);
  if (siz < 0) goto end;
  if (fseek(scr, 0, SEEK_SET)) goto end;

  int bufsize = siz;
  { // Scope block to avoid goto and variable length issue
    char buf[bufsize];

    // Copy the binary
    result = binject_main_copy(bin_path, outpath);
    if (NO_ERROR != result) goto end;

    // Inject the script into the new binary
    if (0> fread(buf, 1, siz, scr)) goto end;
    result = binject_step(info, outpath, buf, siz);
    if (NO_ERROR != result) goto end;
  }

  // Finalize by writing static info into the binary
  result = binject_done(static_data, outpath);

end:
  error_report(0);
  if (scr) fclose(scr);
  return result;
}

static int binject_main_app_internal_script_handle(binject_static_t * info, int argc, char **argv) {
  unsigned int size = 0;
  FILE * f = 0;

  // Get information from static section
  char * script = binject_info(static_data, &size);

  if (script) {
    // Script found in the static section

    // Script echo
    printf("A %d byte script was found (dump:)[", (int)size);
    int w = fwrite(script, 1, size, stdout);
    if (w != size) return ACCESS_ERROR;
    printf("]\n");

  } else {
    // Script should be at end of the binary

    // Open the binary
    f = fopen(argv[0], "rb");
    if (!f) return ACCESS_ERROR;

    // Go at start of the script
    fseek(f, 0, SEEK_END);
    unsigned int script_size = ftell(f) - size;
    if (0 == script_size) return INVALID_RESOURCE_ERROR;
    char buf[script_size];
    script = buf;
    if (0 != fseek(f, size, SEEK_SET)) return ACCESS_ERROR;

    // Script echo
    if (0> fread(buf, 1, size, f)) return ACCESS_ERROR;
    printf("A %d byte script was found (dump:)[", (int)script_size);
    if (fwrite(script, 1, script_size, stdout) != script_size) return ACCESS_ERROR;
    printf("]\n");

    fclose(f);
  }

  return NO_ERROR;
}

int main(int argc, char **argv) {
  int result = GENERIC_ERROR;

  // Get information from static section
  unsigned int size = 0;
  binject_info(static_data, &size);

  // Run the proper tool
  if (size > 0) {
    // No script found: inject
    result = binject_main_app_internal_script_handle(static_data, argc, argv);

  } else if (argc < 2 || argv[1][0] == '\0') {
    // No arguments: print help
    print_help(argv[0]);
    result = NO_ERROR;

  } else {
    // Script found: handle it
    if (argc < 2) { print_help(argv[0]); goto end; }
    result = binject_main_app_internal_script_inject(static_data, argv[1], argv[0], "injed.exe");
  }

end:
  if (result != NO_ERROR) fprintf(stderr, "Error %d\n", result);
  error_report(0);
  return result;
}

