#include "itl/itl.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace rapidjson;

// FUZZ: disable check_for_improper_main_declaration

int main(int argc, char** argv) {
  std::cout << "parsing " << argv[1] << std::endl;
  // 1. Parse a JSON string into DOM.
  std::ifstream ss(argv[1]);

  itl::Dictionary dict;
  dict.fromJson(ss);

  // Document d;
  // d.Parse(json);

  // 2. Modify it by DOM.
  // Value& s = d["stars"];
  // s.SetInt(s.GetInt() + 1);

  // 3. Stringify the DOM
  // StringBuffer buffer;
  // Writer<StringBuffer> writer(buffer);
  // d.Accept(writer);

  // // Output {"project":"rapidjson","stars":11}
  // std::cout << buffer.GetString() << std::endl;
  return 0;
}
