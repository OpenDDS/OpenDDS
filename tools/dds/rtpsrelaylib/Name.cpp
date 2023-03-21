#include "Name.h"

#include <ostream>

namespace RtpsRelay {

static void output_character(std::ostream& out, char c) {
  switch (c) {
  case '[':
  case ']':
  case '!':
  case '\\':
  case '?':
  case '*':
    out << '\\';
  }
  out << c;
}

std::ostream& operator<<(std::ostream& out, const Atom& atom)
{
  switch (atom.kind()) {
  case Atom::CHARACTER:
    output_character(out, atom.character());
    break;
  case Atom::CHARACTER_CLASS:
    out << '[';
    for (char c : atom.characters()) {
      output_character(out, c);
    }
    out << ']';
    break;
  case Atom::NEGATED_CHARACTER_CLASS:
    out << "[!";
    for (char c : atom.characters()) {
      output_character(out, c);
    }
    out << ']';
    break;
  case Atom::WILDCARD:
    out << '?';
    break;
  case Atom::GLOB:
    out << '*';
    break;
  }

  return out;
}

void Name::parse(Name& name, const std::string& buffer, size_t& idx)
{
  if (!name.is_valid_ || idx == buffer.size()) {
    return;
  }

  char c = buffer[idx];
  if (c == '?') {
    name.push_back(Atom(Atom::WILDCARD));
    ++idx;
  } else if (c == '*') {
    name.push_back(Atom(Atom::GLOB));
    ++idx;
  } else if (c == '[') {
    name.push_back(parse_character_class(name, buffer, idx));
  } else {
    name.push_back(Atom(parse_character(name, buffer, idx)));
  }

  parse(name, buffer, idx);
}

char Name::parse_character(Name& name, const std::string& buffer, size_t& idx)
{
  char c = buffer[idx++];
  if (c == '\\') {
    if (idx == buffer.size()) {
      name.is_valid_ = false;
      return 0;
    }
    return buffer[idx++];
  } else {
    return c;
  }
}

Atom Name::parse_character_class(Name& name, const std::string& buffer, size_t& idx)
{
  name.is_pattern_ = true;

  // Skip '[']
  ++idx;

  bool negated = false;

  if (idx == buffer.size()) {
    name.is_valid_ = false;
    return Atom(0);
  }

  if (buffer[idx] == '!') {
    negated = true;
    ++idx;
  }

  // One character is required.
  if (idx == buffer.size()) {
    name.is_valid_ = false;
    return Atom(0);
  }

  std::set<char> characters;
  parse_character_or_range(name, buffer, idx, characters);
  parse_character_class_tail(name, buffer, idx, characters);

  return Atom(negated, characters);
}

void Name::parse_character_class_tail(Name& name, const std::string& buffer, size_t& idx, std::set<char>& characters)
{
  if (idx == buffer.size()) {
    name.is_valid_ = false;
    return;
  }

  char c = buffer[idx];
  if (c == ']') {
    ++idx;
    return;
  }

  parse_character_or_range(name, buffer, idx, characters);
  parse_character_class_tail(name, buffer, idx, characters);
}

void Name::parse_character_or_range(Name& name, const std::string& buffer, size_t& idx, std::set<char>& characters)
{
  char first = parse_character(name, buffer, idx);

  if (idx == buffer.size()) {
    name.is_valid_ = false;
    return;
  }

  if (buffer[idx] != '-') {
    characters.insert(first);
    return;
  }

  ++idx;

  if (idx == buffer.size()) {
    name.is_valid_ = false;
    return;
  }

  char last = parse_character(name, buffer, idx);
  if (first > last) {
    name.is_valid_ = false;
    return;
  }

  for (; first <= last; ++first) {
    characters.insert(first);
  }
}

std::ostream& operator<<(std::ostream& out, const Name& name) {
  for (const auto& atom : name) {
    out << atom;
  }
  return out;
}

}
