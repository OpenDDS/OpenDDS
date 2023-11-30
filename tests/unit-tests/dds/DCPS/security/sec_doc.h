#ifndef OPENDDS_UNIT_TESTS_SEC_DOC_H
#define OPENDDS_UNIT_TESTS_SEC_DOC_H

#include <string>
#include <cstdlib>

inline std::string sec_doc_path(const std::string& path)
{
  const char* const source_root = std::getenv("OPENDDS_SOURCE_DIR");
  std::string tests;
  if (source_root && source_root[0]) {
    tests += std::string(source_root) + "/tests/";
  } else {
    tests += "../"; // (relative to tests/unit-tests)
  }
  return tests + "security/" + path;
}

inline std::string sec_doc_prop(const std::string& path)
{
  return std::string("file:") + sec_doc_path(path);
}

#endif
