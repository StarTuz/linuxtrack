/*
 * ltr_hotkeyd - Global hotkey daemon for Linuxtrack
 *
 * Provides global hotkeys for controlling Linuxtrack from native Linux games
 * without needing to alt-tab.
 *
 * Default hotkeys (configurable via ltr_hotkey_gui):
 *   F12      - Recenter tracking
 *   Pause    - Toggle pause/resume tracking
 *   Ctrl+F12 - Quit daemon
 *
 * Usage: ltr_hotkeyd [--help] [--verbose]
 *
 * Configuration is read from ~/.config/linuxtrack/ltr_hotkey_gui.conf
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Linuxtrack client API */
#include "linuxtrack.h"

/* State */
static volatile bool running = true;
static bool verbose = false;
static bool tracking_paused = false;
static Display *display = NULL;
static Window root_window;

/* Hotkey definitions */
typedef struct {
  KeySym keysym;
  unsigned int modifiers;
  const char *name;
  void (*action)(void);
} hotkey_t;

/* Forward declarations */
static void action_recenter(void);
static void action_toggle_pause(void);
static void action_quit(void);

/* Default hotkeys - will be overridden by config */
static hotkey_t hotkeys[4] = {
    {XK_F12, 0, "Recenter", action_recenter},
    {XK_Pause, 0, "Toggle Pause", action_toggle_pause},
    {XK_F12, ControlMask, "Quit Daemon", action_quit},
    {0, 0, NULL, NULL} /* Sentinel */
};

static void log_msg(const char *fmt, ...) {
  if (!verbose)
    return;
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[ltr_hotkeyd] ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

/* Parse Qt key sequence string to X11 keysym and modifiers */
static bool parse_key_sequence(const char *seq, KeySym *keysym,
                               unsigned int *modifiers) {
  if (!seq || !*seq) {
    return false;
  }

  *modifiers = 0;
  *keysym = 0;

  char buf[256];
  strncpy(buf, seq, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  /* Parse modifiers and key from Qt format: "Ctrl+Shift+F12" */
  char *token = strtok(buf, "+");
  char *last_token = NULL;

  while (token) {
    /* Trim whitespace */
    while (*token && isspace(*token))
      token++;
    char *end = token + strlen(token) - 1;
    while (end > token && isspace(*end))
      *end-- = '\0';

    last_token = token;

    if (strcasecmp(token, "Ctrl") == 0 || strcasecmp(token, "Control") == 0) {
      *modifiers |= ControlMask;
    } else if (strcasecmp(token, "Shift") == 0) {
      *modifiers |= ShiftMask;
    } else if (strcasecmp(token, "Alt") == 0) {
      *modifiers |= Mod1Mask;
    } else if (strcasecmp(token, "Meta") == 0 ||
               strcasecmp(token, "Super") == 0) {
      *modifiers |= Mod4Mask;
    }
    /* Otherwise it's the key itself - will be parsed after loop */

    token = strtok(NULL, "+");
  }

  if (!last_token) {
    return false;
  }

  /* Parse the actual key */
  /* Common key names */
  if (strcasecmp(last_token, "F1") == 0)
    *keysym = XK_F1;
  else if (strcasecmp(last_token, "F2") == 0)
    *keysym = XK_F2;
  else if (strcasecmp(last_token, "F3") == 0)
    *keysym = XK_F3;
  else if (strcasecmp(last_token, "F4") == 0)
    *keysym = XK_F4;
  else if (strcasecmp(last_token, "F5") == 0)
    *keysym = XK_F5;
  else if (strcasecmp(last_token, "F6") == 0)
    *keysym = XK_F6;
  else if (strcasecmp(last_token, "F7") == 0)
    *keysym = XK_F7;
  else if (strcasecmp(last_token, "F8") == 0)
    *keysym = XK_F8;
  else if (strcasecmp(last_token, "F9") == 0)
    *keysym = XK_F9;
  else if (strcasecmp(last_token, "F10") == 0)
    *keysym = XK_F10;
  else if (strcasecmp(last_token, "F11") == 0)
    *keysym = XK_F11;
  else if (strcasecmp(last_token, "F12") == 0)
    *keysym = XK_F12;
  else if (strcasecmp(last_token, "Pause") == 0)
    *keysym = XK_Pause;
  else if (strcasecmp(last_token, "Scroll_Lock") == 0 ||
           strcasecmp(last_token, "ScrollLock") == 0)
    *keysym = XK_Scroll_Lock;
  else if (strcasecmp(last_token, "Print") == 0 ||
           strcasecmp(last_token, "SysReq") == 0)
    *keysym = XK_Print;
  else if (strcasecmp(last_token, "Insert") == 0 ||
           strcasecmp(last_token, "Ins") == 0)
    *keysym = XK_Insert;
  else if (strcasecmp(last_token, "Delete") == 0 ||
           strcasecmp(last_token, "Del") == 0)
    *keysym = XK_Delete;
  else if (strcasecmp(last_token, "Home") == 0)
    *keysym = XK_Home;
  else if (strcasecmp(last_token, "End") == 0)
    *keysym = XK_End;
  else if (strcasecmp(last_token, "Page_Up") == 0 ||
           strcasecmp(last_token, "PgUp") == 0)
    *keysym = XK_Page_Up;
  else if (strcasecmp(last_token, "Page_Down") == 0 ||
           strcasecmp(last_token, "PgDown") == 0)
    *keysym = XK_Page_Down;
  else if (strcasecmp(last_token, "Space") == 0)
    *keysym = XK_space;
  else if (strcasecmp(last_token, "Escape") == 0 ||
           strcasecmp(last_token, "Esc") == 0)
    *keysym = XK_Escape;
  else if (strcasecmp(last_token, "Return") == 0 ||
           strcasecmp(last_token, "Enter") == 0)
    *keysym = XK_Return;
  else if (strcasecmp(last_token, "Tab") == 0)
    *keysym = XK_Tab;
  else if (strcasecmp(last_token, "Backspace") == 0)
    *keysym = XK_BackSpace;
  else if (strlen(last_token) == 1) {
    /* Single character key */
    *keysym = XStringToKeysym(last_token);
    if (*keysym == NoSymbol) {
      /* Try lowercase */
      char lower[2] = {(char)tolower(last_token[0]), 0};
      *keysym = XStringToKeysym(lower);
    }
  } else {
    /* Try XStringToKeysym directly */
    *keysym = XStringToKeysym(last_token);
  }

  return *keysym != NoSymbol && *keysym != 0;
}

/* Read config from QSettings file */
static void load_config(void) {
  char config_path[512];
  const char *home = getenv("HOME");
  if (!home) {
    log_msg("HOME not set, using defaults");
    return;
  }

  snprintf(config_path, sizeof(config_path),
           "%s/.config/linuxtrack/ltr_hotkey_gui.conf", home);

  FILE *fp = fopen(config_path, "r");
  if (!fp) {
    log_msg("Config file not found, using defaults: %s", config_path);
    return;
  }

  log_msg("Loading config from: %s", config_path);

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    /* Remove newline */
    line[strcspn(line, "\r\n")] = 0;

    /* Parse key=value */
    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    char *key = line;
    char *value = eq + 1;

    /* Trim whitespace */
    while (*key && isspace(*key))
      key++;
    while (*value && isspace(*value))
      value++;

    KeySym keysym;
    unsigned int modifiers;

    if (strcmp(key, "recenter_key") == 0) {
      if (parse_key_sequence(value, &keysym, &modifiers)) {
        hotkeys[0].keysym = keysym;
        hotkeys[0].modifiers = modifiers;
        log_msg("Recenter key: %s (keysym=0x%lx, mods=0x%x)", value, keysym,
                modifiers);
      }
    } else if (strcmp(key, "pause_key") == 0) {
      if (parse_key_sequence(value, &keysym, &modifiers)) {
        hotkeys[1].keysym = keysym;
        hotkeys[1].modifiers = modifiers;
        log_msg("Pause key: %s (keysym=0x%lx, mods=0x%x)", value, keysym,
                modifiers);
      }
    } else if (strcmp(key, "quit_key") == 0) {
      if (parse_key_sequence(value, &keysym, &modifiers)) {
        hotkeys[2].keysym = keysym;
        hotkeys[2].modifiers = modifiers;
        log_msg("Quit key: %s (keysym=0x%lx, mods=0x%x)", value, keysym,
                modifiers);
      }
    }
  }

  fclose(fp);
}

static void action_recenter(void) {
  log_msg("Recentering tracking...");
  linuxtrack_recenter();
}

static void action_toggle_pause(void) {
  if (tracking_paused) {
    log_msg("Resuming tracking...");
    linuxtrack_wakeup();
    tracking_paused = false;
  } else {
    log_msg("Pausing tracking...");
    linuxtrack_suspend();
    tracking_paused = true;
  }
}

static void action_quit(void) {
  log_msg("Quit requested.");
  running = false;
}

static void signal_handler(int sig) {
  (void)sig;
  running = false;
}

static bool grab_hotkeys(void) {
  for (int i = 0; hotkeys[i].keysym != 0; i++) {
    KeyCode keycode = XKeysymToKeycode(display, hotkeys[i].keysym);
    if (keycode == 0) {
      fprintf(stderr, "Warning: No keycode for keysym %s\n", hotkeys[i].name);
      continue;
    }

    /* Grab with and without NumLock/CapsLock modifiers */
    unsigned int mods[] = {0, Mod2Mask, LockMask, Mod2Mask | LockMask};
    for (unsigned int m = 0; m < 4; m++) {
      int result = XGrabKey(display, keycode, hotkeys[i].modifiers | mods[m],
                            root_window, True, GrabModeAsync, GrabModeAsync);
      if (result != 0) {
        log_msg("Grabbed %s (mod variant %d)", hotkeys[i].name, m);
      }
    }
  }
  return true;
}

static void ungrab_hotkeys(void) {
  for (int i = 0; hotkeys[i].keysym != 0; i++) {
    KeyCode keycode = XKeysymToKeycode(display, hotkeys[i].keysym);
    if (keycode != 0) {
      XUngrabKey(display, keycode, AnyModifier, root_window);
    }
  }
}

static void process_key_event(XKeyEvent *ke) {
  KeySym keysym = XLookupKeysym(ke, 0);

  /* Remove NumLock/CapsLock from consideration */
  unsigned int clean_mods = ke->state & ~(Mod2Mask | LockMask);

  for (int i = 0; hotkeys[i].keysym != 0; i++) {
    if (keysym == hotkeys[i].keysym &&
        (clean_mods & ~ShiftMask) == hotkeys[i].modifiers) {
      log_msg("Hotkey pressed: %s", hotkeys[i].name);
      if (hotkeys[i].action) {
        hotkeys[i].action();
      }
      return;
    }
  }
}

static void print_usage(const char *progname) {
  printf("Usage: %s [OPTIONS]\n"
         "\n"
         "Global hotkey daemon for Linuxtrack (X11 only).\n"
         "\n"
         "Options:\n"
         "  --help, -h      Show this help message\n"
         "  --verbose, -v   Print verbose messages\n"
         "\n"
         "Default Hotkeys (configurable via ltr_hotkey_gui):\n"
         "  F12             Recenter tracking\n"
         "  Pause           Toggle pause/resume tracking\n"
         "  Ctrl+F12        Quit this daemon\n"
         "\n"
         "Configuration is read from:\n"
         "  ~/.config/linuxtrack/ltr_hotkey_gui.conf\n"
         "\n"
         "The daemon must be running alongside ltr_gui or ltr_server1.\n",
         progname);
}

int main(int argc, char *argv[]) {
  /* Parse arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "--verbose") == 0 ||
               strcmp(argv[i], "-v") == 0) {
      verbose = true;
    }
  }

  printf("ltr_hotkeyd: Starting global hotkey daemon for Linuxtrack...\n");

  /* Load configuration */
  load_config();

  /* Set up signal handlers */
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Connect to X11 */
  display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Error: Cannot open X display. Are you running X11?\n");
    fprintf(stderr, "Note: Wayland is not supported. Use X11 or XWayland.\n");
    return 1;
  }

  root_window = DefaultRootWindow(display);
  log_msg("Connected to X11 display.");

  /* Initialize Linuxtrack */
  linuxtrack_state_type state = linuxtrack_init(NULL);
  if (state < LINUXTRACK_OK) {
    fprintf(stderr, "Error: Cannot initialize Linuxtrack: %s\n",
            linuxtrack_explain(state));
    fprintf(stderr, "Make sure ltr_gui is running or ltr_server1 is active.\n");
    XCloseDisplay(display);
    return 1;
  }

  /* Wait for tracker to be running */
  int timeout = 0;
  while (timeout < 50) {
    state = linuxtrack_get_tracking_state();
    if (state == RUNNING || state == PAUSED) {
      break;
    }
    usleep(100000);
    timeout++;
  }

  if (state != RUNNING && state != PAUSED) {
    fprintf(stderr, "Warning: Tracker not running (state: %s)\n",
            linuxtrack_explain(state));
    fprintf(stderr, "Hotkeys will work once tracking starts.\n");
  } else {
    log_msg("Linuxtrack connected (state: %s)", linuxtrack_explain(state));
  }

  /* Grab global hotkeys */
  if (!grab_hotkeys()) {
    fprintf(stderr, "Error: Failed to grab hotkeys.\n");
    linuxtrack_shutdown();
    XCloseDisplay(display);
    return 1;
  }

  printf("ltr_hotkeyd: Ready. Hotkeys active.\n");
  printf("  Recenter     = %s%s\n",
         hotkeys[0].modifiers & ControlMask ? "Ctrl+" : "",
         XKeysymToString(hotkeys[0].keysym));
  printf("  Toggle Pause = %s%s\n",
         hotkeys[1].modifiers & ControlMask ? "Ctrl+" : "",
         XKeysymToString(hotkeys[1].keysym));
  printf("  Quit         = %s%s\n",
         hotkeys[2].modifiers & ControlMask ? "Ctrl+" : "",
         XKeysymToString(hotkeys[2].keysym));

  /* Main event loop */
  XEvent event;
  while (running) {
    /* Use XPending + XNextEvent for non-blocking check with timeout */
    if (XPending(display) > 0) {
      XNextEvent(display, &event);
      if (event.type == KeyPress) {
        process_key_event(&event.xkey);
      }
    } else {
      usleep(50000); /* 50ms idle */
    }
  }

  /* Cleanup */
  printf("\nltr_hotkeyd: Shutting down...\n");
  ungrab_hotkeys();
  linuxtrack_shutdown();
  XCloseDisplay(display);

  return 0;
}
