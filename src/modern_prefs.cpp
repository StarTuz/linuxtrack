#include "modern_prefs.h"
#include "mini_ini.h"

#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
// Global state
std::unique_ptr<mINI::INIStructure> g_ini;
std::string g_current_filename;
bool g_needs_save = false;

// Helper to allocate C string (caller must free)
char *strdup_safe(const std::string &str) {
  if (str.empty())
    return nullptr;
  char *result = static_cast<char *>(malloc(str.size() + 1));
  if (result) {
    std::strcpy(result, str.c_str());
  }
  return result;
}

} // namespace

void modern_prefs_init() {
  g_ini = std::make_unique<mINI::INIStructure>();
  g_current_filename.clear();
  g_needs_save = false;
}

int modern_prefs_read(const char *filename, bool create_if_missing) {
  if (!filename || !g_ini)
    return 0;

  g_current_filename = filename;

  // Check if file exists
  if (!std::filesystem::exists(filename)) {
    if (create_if_missing) {
      // Create empty config
      g_ini->clear();
      g_needs_save = true;
      return 1;
    }
    return 0;
  }

  // Parse file
  mINI::INIFile file(filename);
  if (!file.read(*g_ini)) {
    return 0;
  }

  g_needs_save = false;
  return 1;
}

int modern_prefs_write(const char *filename) {
  if (!filename || !g_ini)
    return 0;

  mINI::INIFile file(filename);
  if (!file.write(*g_ini)) {
    return 0;
  }

  g_needs_save = false;
  return 1;
}

char *modern_prefs_get_key(const char *section, const char *key) {
  if (!section || !key || !g_ini)
    return nullptr;

  if (!g_ini->has(section))
    return nullptr;

  auto &sec = (*g_ini)[section];
  if (!sec.has(key))
    return nullptr;

  return strdup_safe(sec[key]);
}

int modern_prefs_set_key(const char *section, const char *key,
                         const char *value) {
  if (!section || !key || !value || !g_ini)
    return 0;

  (*g_ini)[section][key] = value;
  g_needs_save = true;
  return 1;
}

void modern_prefs_find_sections(const char *key, void *sections_vector) {
  if (!key || !sections_vector || !g_ini)
    return;

  auto *vec = static_cast<std::vector<std::string> *>(sections_vector);
  vec->clear();

  // Iterate through all sections
  for (auto const &section : *g_ini) {
    if (section.second.has(key)) {
      vec->push_back(section.first);
    }
  }
}

char *modern_prefs_find_section(const char *key, const char *value) {
  if (!key || !value || !g_ini)
    return nullptr;

  // Iterate through all sections
  for (auto const &section : *g_ini) {
    if (section.second.has(key)) {
      if (section.second.get(key) == value) {
        return strdup_safe(section.first);
      }
    }
  }

  return nullptr;
}

void modern_prefs_get_all_sections(void *sections_vector) {
  if (!sections_vector || !g_ini)
    return;

  auto *vec = static_cast<std::vector<std::string> *>(sections_vector);
  vec->clear();

  for (auto const &section : *g_ini) {
    vec->push_back(section.first);
  }
}

char *modern_prefs_add_section(const char *section_name) {
  if (!section_name || !g_ini)
    return nullptr;

  std::string base_name(section_name);
  std::string unique_name = base_name;

  // Make unique if needed
  int counter = 1;
  while (g_ini->has(unique_name)) {
    unique_name = base_name + " " + std::to_string(counter++);
  }

  // Create empty section
  (*g_ini)[unique_name];
  g_needs_save = true;

  return strdup_safe(unique_name);
}

void modern_prefs_free() {
  g_ini.reset();
  g_current_filename.clear();
  g_needs_save = false;
}

int modern_prefs_need_save() { return g_needs_save ? 1 : 0; }

void modern_prefs_mark_changed() { g_needs_save = true; }
