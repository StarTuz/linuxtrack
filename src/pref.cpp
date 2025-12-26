// Modern preference system wrapper for Linuxtrack
// Maintains API compatibility with legacy code while using mINI parser
// Based on the modernization from tuxtracksold

#include "pref.h"
#include "modern_prefs.h"
#include "pref.hpp"
#include "utils.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Static initialization flag
static bool prefs_initialized = false;

static void ensure_init() {
  if (!prefs_initialized) {
    modern_prefs_init();
    prefs_initialized = true;
  }
}

// ----------------------------------------------------------------------------
// C API implementation (pref.h)
// ----------------------------------------------------------------------------

char *ltr_int_get_key(const char *section_name, const char *key_name) {
  ensure_init();
  return modern_prefs_get_key(section_name, key_name);
}

bool ltr_int_get_key_flt(const char *section_name, const char *key_name,
                         float *val) {
  char *str_val = ltr_int_get_key(section_name, key_name);
  if (!str_val || !val)
    return false;
  *val = std::atof(str_val);
  free(str_val);
  return true;
}

bool ltr_int_get_key_int(const char *section_name, const char *key_name,
                         int *val) {
  char *str_val = ltr_int_get_key(section_name, key_name);
  if (!str_val || !val)
    return false;
  *val = std::atoi(str_val);
  free(str_val);
  return true;
}

bool ltr_int_change_key(const char *section_name, const char *key_name,
                        const char *new_value) {
  ensure_init();
  return modern_prefs_set_key(section_name, key_name, new_value) != 0;
}

bool ltr_int_change_key_flt(const char *section_name, const char *key_name,
                            float new_value) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%g", new_value);
  return ltr_int_change_key(section_name, key_name, buffer);
}

bool ltr_int_change_key_int(const char *section_name, const char *key_name,
                            int new_value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%d", new_value);
  return ltr_int_change_key(section_name, key_name, buffer);
}

void ltr_int_free_prefs(void) {
  modern_prefs_free();
  prefs_initialized = false;
}

bool ltr_int_read_prefs(const char *file, bool force_read) {
  ensure_init();
  if (!file) {
    char *pfile = ltr_int_get_default_file_name(NULL);
    if (!pfile)
      return false;
    bool res = modern_prefs_read(pfile, false);
    free(pfile);
    return res;
  }
  return modern_prefs_read(file, force_read) != 0;
}

bool ltr_int_new_prefs(void) {
  ensure_init();
  modern_prefs_mark_changed();
  return true;
}

bool ltr_int_save_prefs(const char *fname) {
  ensure_init();
  if (!fname) {
    char *pfile = ltr_int_get_default_file_name(NULL);
    if (!pfile)
      return false;
    int res = modern_prefs_write(pfile);
    free(pfile);
    return res != 0;
  }
  return modern_prefs_write(fname) != 0;
}

bool ltr_int_dump_prefs(const char *file_name) {
  return ltr_int_save_prefs(file_name);
}

bool ltr_int_need_saving(void) { return modern_prefs_need_save() != 0; }

char *ltr_int_find_section(const char *key_name, const char *value) {
  ensure_init();
  return modern_prefs_find_section(key_name, value);
}

bool ltr_int_find_sections(const char *key_name, void *result) {
  ensure_init();
  modern_prefs_find_sections(key_name, result);
  auto *vec = static_cast<std::vector<std::string> *>(result);
  return !vec->empty();
}

char *ltr_int_add_unique_section(const char *name_template) {
  ensure_init();
  return modern_prefs_add_section(name_template);
}

void ltr_int_prefs_changed(void) { modern_prefs_mark_changed(); }

void ltr_int_get_section_list(void *sections_ptr) {
  ensure_init();
  modern_prefs_get_all_sections(sections_ptr);
}

// ----------------------------------------------------------------------------
// Compatibility Layer for C++ prefs class (pref.hpp)
// ----------------------------------------------------------------------------

// Stub implementation to satisfy linkers if needed, though they shouldn't be
// called by modern code. If called, they bridge to the C API.

prefs *prefs::prf = nullptr;
static pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;

prefs &prefs::getPrefs() {
  pthread_mutex_lock(&sg_mutex);
  if (!prf)
    prf = new prefs();
  pthread_mutex_unlock(&sg_mutex);
  return *prf;
}

void prefs::freePrefs() {
  pthread_mutex_lock(&sg_mutex);
  if (prf) {
    delete prf;
    prf = nullptr;
  }
  pthread_mutex_unlock(&sg_mutex);
}

prefs::prefs() : changed_flag(false) { pthread_rwlock_init(&lock, NULL); }

prefs::~prefs() { pthread_rwlock_destroy(&lock); }

bool prefs::getValue(const std::string &sec, const std::string &key,
                     std::string &result) const {
  char *val = ltr_int_get_key(sec.c_str(), key.c_str());
  if (val) {
    result = val;
    free(val);
    return true;
  }
  return false;
}

bool prefs::getValue(const std::string &sec, const std::string &key,
                     float &result) const {
  return ltr_int_get_key_flt(sec.c_str(), key.c_str(), &result);
}

bool prefs::getValue(const std::string &sec, const std::string &key,
                     int &result) const {
  return ltr_int_get_key_int(sec.c_str(), key.c_str(), &result);
}

void prefs::setValue(const std::string &sec, const std::string &key,
                     const std::string &value) {
  ltr_int_change_key(sec.c_str(), key.c_str(), value.c_str());
}

void prefs::setValue(const std::string &sec, const std::string &key,
                     const float &value) {
  ltr_int_change_key_flt(sec.c_str(), key.c_str(), value);
}

void prefs::setValue(const std::string &sec, const std::string &key,
                     const int &value) {
  ltr_int_change_key_int(sec.c_str(), key.c_str(), value);
}

void prefs::getSectionList(std::vector<std::string> &sections) const {
  ltr_int_get_section_list(&sections);
}

bool prefs::sectionExists(const std::string &sec) const {
  // Rough estimate: if we can find any key in it or get its list
  std::vector<std::string> list;
  ltr_int_get_section_list(&list);
  for (const auto &s : list) {
    if (s == sec)
      return true;
  }
  return false;
}

void prefs::clear() { ltr_int_free_prefs(); }

bool prefs::changed() const { return ltr_int_need_saving(); }

void prefs::setChangeFlag() { ltr_int_prefs_changed(); }

void prefs::resetChangeFlag() {
  // Not directly supported by mINI in a simple way without writing,
  // but we can just say "not changed" after save.
}
