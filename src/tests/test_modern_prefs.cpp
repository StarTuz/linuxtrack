// Unit tests for modern_prefs module
// Uses Catch2 v3 testing framework

#include "../modern_prefs.h"
#include "catch2/catch_amalgamated.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h> // For write(), close(), mkstemp()
#include <vector>

// Helper to create a temporary test file
static std::string createTestFile(const std::string &content) {
  char tmpname[] = "/tmp/linuxtrack_test_XXXXXX";
  int fd = mkstemp(tmpname);
  if (fd < 0)
    return "";

  write(fd, content.c_str(), content.size());
  close(fd);
  return tmpname;
}

// Helper to cleanup test file
static void removeTestFile(const std::string &path) {
  if (!path.empty()) {
    std::remove(path.c_str());
  }
}

TEST_CASE("modern_prefs initialization", "[prefs]") {
  modern_prefs_init();
  modern_prefs_free();
  REQUIRE(true); // If we get here without crash, init/free works
}

TEST_CASE("modern_prefs read basic INI file", "[prefs]") {
  std::string content = "[General]\n"
                        "name=Test\n"
                        "version=1.0\n"
                        "\n"
                        "[Device]\n"
                        "type=TrackIR\n"
                        "id=12345\n";

  std::string tmpfile = createTestFile(content);
  REQUIRE(!tmpfile.empty());

  modern_prefs_init();
  REQUIRE(modern_prefs_read(tmpfile.c_str(), false) == 1);

  // Test getting values
  char *name = modern_prefs_get_key("General", "name");
  REQUIRE(name != nullptr);
  REQUIRE(std::string(name) == "Test");
  free(name);

  char *version = modern_prefs_get_key("General", "version");
  REQUIRE(version != nullptr);
  REQUIRE(std::string(version) == "1.0");
  free(version);

  char *devType = modern_prefs_get_key("Device", "type");
  REQUIRE(devType != nullptr);
  REQUIRE(std::string(devType) == "TrackIR");
  free(devType);

  // Test non-existent key
  char *nonExistent = modern_prefs_get_key("General", "nonexistent");
  REQUIRE(nonExistent == nullptr);

  modern_prefs_free();
  removeTestFile(tmpfile);
}

TEST_CASE("modern_prefs set and get keys", "[prefs]") {
  modern_prefs_init();

  // Set new values
  REQUIRE(modern_prefs_set_key("TestSection", "key1", "value1") == 1);
  REQUIRE(modern_prefs_set_key("TestSection", "key2", "value2") == 1);
  REQUIRE(modern_prefs_set_key("AnotherSection", "foo", "bar") == 1);

  // Get values back
  char *v1 = modern_prefs_get_key("TestSection", "key1");
  REQUIRE(v1 != nullptr);
  REQUIRE(std::string(v1) == "value1");
  free(v1);

  char *v2 = modern_prefs_get_key("TestSection", "key2");
  REQUIRE(v2 != nullptr);
  REQUIRE(std::string(v2) == "value2");
  free(v2);

  char *foo = modern_prefs_get_key("AnotherSection", "foo");
  REQUIRE(foo != nullptr);
  REQUIRE(std::string(foo) == "bar");
  free(foo);

  // Update existing value
  REQUIRE(modern_prefs_set_key("TestSection", "key1", "updated") == 1);
  char *updated = modern_prefs_get_key("TestSection", "key1");
  REQUIRE(updated != nullptr);
  REQUIRE(std::string(updated) == "updated");
  free(updated);

  modern_prefs_free();
}

TEST_CASE("modern_prefs write and read back", "[prefs]") {
  std::string tmpfile = "/tmp/linuxtrack_test_write.ini";

  modern_prefs_init();

  // Set some values
  modern_prefs_set_key("WriteTest", "alpha", "hello");
  modern_prefs_set_key("WriteTest", "beta", "world");
  modern_prefs_set_key("Numbers", "count", "42");

  // Write to file
  REQUIRE(modern_prefs_write(tmpfile.c_str()) == 1);

  // Clear and read back
  modern_prefs_free();
  modern_prefs_init();
  REQUIRE(modern_prefs_read(tmpfile.c_str(), false) == 1);

  // Verify values
  char *alpha = modern_prefs_get_key("WriteTest", "alpha");
  REQUIRE(alpha != nullptr);
  REQUIRE(std::string(alpha) == "hello");
  free(alpha);

  char *count = modern_prefs_get_key("Numbers", "count");
  REQUIRE(count != nullptr);
  REQUIRE(std::string(count) == "42");
  free(count);

  modern_prefs_free();
  removeTestFile(tmpfile);
}

TEST_CASE("modern_prefs get all sections", "[prefs]") {
  modern_prefs_init();

  modern_prefs_set_key("Section1", "key", "val");
  modern_prefs_set_key("Section2", "key", "val");
  modern_prefs_set_key("Section3", "key", "val");

  std::vector<std::string> sections;
  modern_prefs_get_all_sections(&sections);

  REQUIRE(sections.size() >= 3);

  bool found1 = false, found2 = false, found3 = false;
  for (const auto &s : sections) {
    if (s == "Section1")
      found1 = true;
    if (s == "Section2")
      found2 = true;
    if (s == "Section3")
      found3 = true;
  }
  REQUIRE(found1);
  REQUIRE(found2);
  REQUIRE(found3);

  modern_prefs_free();
}

TEST_CASE("modern_prefs find section by key value", "[prefs]") {
  modern_prefs_init();

  modern_prefs_set_key("Device1", "type", "TrackIR");
  modern_prefs_set_key("Device2", "type", "Webcam");
  modern_prefs_set_key("Device3", "type", "TrackIR");

  char *found = modern_prefs_find_section("type", "Webcam");
  REQUIRE(found != nullptr);
  REQUIRE(std::string(found) == "Device2");
  free(found);

  char *notFound = modern_prefs_find_section("type", "NonExistent");
  REQUIRE(notFound == nullptr);

  modern_prefs_free();
}

TEST_CASE("modern_prefs change tracking", "[prefs]") {
  modern_prefs_init();

  // Initially no changes
  REQUIRE(modern_prefs_need_save() == 0);

  // Mark as changed
  modern_prefs_mark_changed();
  REQUIRE(modern_prefs_need_save() == 1);

  // Write should clear the flag (if file write succeeds)
  std::string tmpfile = "/tmp/linuxtrack_test_change.ini";
  modern_prefs_set_key("Test", "key", "value");
  modern_prefs_write(tmpfile.c_str());

  modern_prefs_free();
  removeTestFile(tmpfile);
}
