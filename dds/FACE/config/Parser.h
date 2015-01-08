#ifndef FACE_CONFIG_PARSER_H
#define FACE_CONFIG_PARSER_H

#include "FACE/OpenDDS_FACE_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export Parser {
public:
  // Returns non-zero on failure
  int parse(const char* filename);

private:
  int foo;
};

} } }

#endif
