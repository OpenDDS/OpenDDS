#ifndef RTPSRELAY_NAME_H_
#define RTPSRELAY_NAME_H_

#include "export.h"

#include <set>
#include <string>
#include <vector>

namespace RtpsRelay {

class RtpsRelayLib_Export Atom {
public:
  enum Kind {
    CHARACTER,
    CHARACTER_CLASS,
    NEGATED_CHARACTER_CLASS,
    WILDCARD,
    GLOB
  };

  explicit Atom(Kind kind) : kind_(kind), character_(0) {}
  explicit Atom(char c) : kind_(CHARACTER), character_(c) {}
  Atom(bool negated, const std::set<char>& characters)
    : kind_(negated ? NEGATED_CHARACTER_CLASS : CHARACTER_CLASS)
    , character_(0)
    , characters_(characters) {}

  Kind kind() const { return kind_; }

  char character() const { return character_; }

  const std::set<char>& characters() const { return characters_; }

  bool operator<(const Atom& other) const
  {
    if (kind_ != other.kind_) {
      return kind_ < other.kind_;
    }
    switch (kind_) {
    case CHARACTER:
      return character_ < other.character_;
    case CHARACTER_CLASS:
    case NEGATED_CHARACTER_CLASS:
      return characters_ < other.characters_;
    default:
      return false;
    }
  }

  bool operator==(const Atom& other) const
  {
    if (kind_ != other.kind_) {
      return false;
    }
    switch (kind_) {
    case CHARACTER:
      return character_ == other.character_;
    case CHARACTER_CLASS:
    case NEGATED_CHARACTER_CLASS:
      return characters_ == other.characters_;
    default:
      return true;
    }
  }

  bool operator!=(const Atom& other) const
  {
    return !(*this == other);
  }

  bool is_pattern() const
  {
    switch (kind_) {
    case CHARACTER_CLASS:
    case NEGATED_CHARACTER_CLASS:
    case WILDCARD:
    case GLOB:
      return true;
    default:
      return false;
    }
  }

private:
  Kind kind_;
  char character_;            // For CHARACTER.
  std::set<char> characters_; // For CHARACTER_CLASS and NEGATED_CHARACTER_CLASS.
};

RtpsRelayLib_Export std::ostream& operator<<(std::ostream& out, const Atom& atom);

class RtpsRelayLib_Export Name {
public:
  typedef std::vector<Atom> Atoms;
  typedef Atoms::const_iterator const_iterator;

  Name() : is_pattern_(false), is_valid_(true) {}

  explicit Name(const std::string& name) : is_pattern_(false), is_valid_(true)
  {
    size_t idx = 0;
    parse(*this, name, idx);
  }

  bool is_valid() const { return is_valid_; }
  bool is_literal() const { return !is_pattern_; }
  bool is_pattern() const { return is_pattern_; }
  const_iterator begin() const { return atoms_.begin(); }
  const_iterator end() const { return atoms_.end(); }

  bool operator<(const Name& other) const { return atoms_ < other.atoms_; }
  bool operator==(const Name& other) const
  {
    return is_pattern_ == other.is_pattern_ &&
      is_valid_ == other.is_valid_ &&
      atoms_ == other.atoms_;
  }
  bool operator!=(const Name& other) const
  {
    return !(*this == other);
  }

  void push_back(const Atom& atom)
  {
    // Don't allow consecutive globs.
    if (atom.kind() == Atom::GLOB && !atoms_.empty() && atoms_.back().kind() == Atom::GLOB) {
      return;
    }

    is_pattern_ = is_pattern_ || atom.is_pattern();
    atoms_.push_back(atom);
  }

private:
  Atoms atoms_;
  bool is_pattern_;
  bool is_valid_;

  static void parse(Name& name, const std::string& buffer, size_t& idx);
  static Atom::Kind parse_pattern(Name& name, const std::string& buffer, size_t& idx, char expected, Atom::Kind kind);
  static char parse_character(Name& name, const std::string& buffer, size_t& idx);
  static Atom parse_character_class(Name& name, const std::string& buffer, size_t& idx);
  static void parse_character_class_tail(Name& name, const std::string& buffer, size_t& idx, std::set<char>& characters);
  static void parse_character_or_range(Name& name, const std::string& buffer, size_t& idx, std::set<char>& characters);
};

RtpsRelayLib_Export std::ostream& operator<<(std::ostream& out, const Name& name);

}

#endif // RTPSRELAY_NAME_H_
