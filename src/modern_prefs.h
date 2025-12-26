#ifndef MODERN_PREFS_H
#define MODERN_PREFS_H

#include <string>
#include <vector>

// Modern C++17 preference system that replaces the broken Bison/Flex parser
// Maintains C-compatible API for existing code

#ifdef __cplusplus
extern "C" {
#endif

// Initialize preferences system
void modern_prefs_init();

// Read preferences from file
// Returns 1 on success, 0 on failure
int modern_prefs_read(const char *filename, bool create_if_missing);

// Write preferences to file
// Returns 1 on success, 0 on failure
int modern_prefs_write(const char *filename);

// Get value for a key in a section
// Returns newly allocated string (caller must free) or NULL if not found
char *modern_prefs_get_key(const char *section, const char *key);

// Set value for a key in a section
// Returns 1 on success, 0 on failure
int modern_prefs_set_key(const char *section, const char *key,
                         const char *value);

// Find all sections containing a specific key
// Populates the vector with section names
void modern_prefs_find_sections(const char *key, void *sections_vector);

// Find first section containing key=value
// Returns newly allocated string (caller must free) or NULL if not found
char *modern_prefs_find_section(const char *key, const char *value);

// Get list of all section names
void modern_prefs_get_all_sections(void *sections_vector);

// Add/create a new unique section
// Returns newly allocated section name (caller must free) or NULL on failure
char *modern_prefs_add_section(const char *section_name);

// Free all preferences data
void modern_prefs_free();

// Check if preferences need saving
int modern_prefs_need_save();

// Mark preferences as changed
void modern_prefs_mark_changed();

#ifdef __cplusplus
}
#endif

#endif // MODERN_PREFS_H
