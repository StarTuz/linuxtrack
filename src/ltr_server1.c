#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
#include "ltr_srv_slave.h"
#include "utils.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Validate that required libraries can be found.
 * Provides clear error messages instead of silent failures.
 */
static void validate_paths(void) {
  char *lib_path = ltr_int_get_lib_path("libltr");

  if (lib_path == NULL) {
    fprintf(stderr, "\n");
    fprintf(
        stderr,
        "=================================================================\n");
    fprintf(stderr, "ERROR: Cannot determine library path!\n");
    fprintf(
        stderr,
        "=================================================================\n");
    fprintf(stderr, "\n");
    fprintf(
        stderr,
        "This usually means the configuration file is missing or invalid.\n");
    fprintf(stderr, "Expected config: ~/.config/linuxtrack/linuxtrack1.conf\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Solutions:\n");
    fprintf(stderr, "  1. Run ltr_gui to create/update configuration\n");
    fprintf(stderr, "  2. Reinstall linuxtrack\n");
    fprintf(stderr, "\n");
    // Don't exit - let it try to continue, the actual load will fail with more
    // info
    return;
  }

  if (access(lib_path, F_OK) != 0) {
    fprintf(stderr, "\n");
    fprintf(
        stderr,
        "=================================================================\n");
    fprintf(stderr, "ERROR: Core library not found!\n");
    fprintf(
        stderr,
        "=================================================================\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Expected: %s\n", lib_path);
    fprintf(stderr, "\n");
    fprintf(stderr,
            "This binary was installed/configured for a different location.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Solutions:\n");
    fprintf(stderr,
            "  1. Run ltr_gui from the correct installation, save settings\n");
    fprintf(stderr, "  2. Reinstall linuxtrack to the expected location\n");
    fprintf(stderr,
            "  3. Set LD_LIBRARY_PATH to include the library directory\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "For debugging, check:\n");
    fprintf(stderr, "  - Config file: ~/.config/linuxtrack/linuxtrack1.conf\n");
    fprintf(stderr, "  - Log file: /tmp/linuxtrack*.log\n");
    fprintf(stderr, "\n");
    // Don't exit - let it try to continue for non-critical paths
  }

  free(lib_path);
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  ltr_int_check_root();

  // Validate paths early to catch configuration issues
  validate_paths();

  // Make sure that broken pipe won't bring us down
  signal(SIGPIPE, SIG_IGN);
  if (argc == 1) {
    ltr_int_master(true);
  } else {
    // Parameter is name of profile
    ltr_int_slave(argv[1], argv[2], argv[3], argv[4], argv[5]);
  }
  return 0;
}
