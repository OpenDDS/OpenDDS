#ifndef RTPSRELAY_QOS_INDEX_H_
#define RTPSRELAY_QOS_INDEX_H_

#include "Name.h"
#include "RelayTypeSupportImpl.h"

class NoIndex;

struct RtpsRelayLib_Export Writer {
  explicit Writer(const RtpsRelay::WriterEntry& we, bool local) : writer_entry(we), local_(local) {}
  RtpsRelay::WriterEntry writer_entry;
  std::set<NoIndex*> indexes;

  bool local() const { return local_; }
  bool remote() const { return !local_; }

private:
  bool const local_;
};

struct RtpsRelayLib_Export Reader {
  explicit Reader(const RtpsRelay::ReaderEntry& re, bool local) : reader_entry(re), local_(local) {}
  RtpsRelay::ReaderEntry reader_entry;
  std::set<NoIndex*> indexes;

  bool local() const { return local_; }
  bool remote() const { return !local_; }

private:
  bool const local_;
};

typedef std::set<Writer*> WriterSet;
typedef std::set<Reader*> ReaderSet;

class RtpsRelayLib_Export NoIndex {
public:
  ~NoIndex() {
    for (Writers::const_iterator pos = writers_.begin(), limit = writers_.end(); pos != limit; ++pos) {
      pos->first->indexes.erase(this);
    }
    for (Readers::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
      pos->first->indexes.erase(this);
    }
  }

  void insert(Writer* writer)
  {
    auto p = writers_.insert(std::make_pair(writer, ReaderSet()));
    writer->indexes.insert(this);
    match(p.first);
  }

  void reinsert(Writer* writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    writer->writer_entry = writer_entry;
    match(writers_.find(writer));
  }

  void erase(Writer* writer)
  {
    Writers::iterator pos = writers_.find(writer);
    for (Reader* reader : pos->second) {
      readers_[reader].erase(writer);
    }
    writers_.erase(writer);
    writer->indexes.erase(this);
  }

  void insert(Reader* reader)
  {
    auto p = readers_.insert(std::make_pair(reader, WriterSet()));
    reader->indexes.insert(this);
    match(p.first);
  }

  void reinsert(Reader* reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    reader->reader_entry = reader_entry;
    match(readers_.find(reader));
  }

  void erase(Reader* reader)
  {
    Readers::iterator pos = readers_.find(reader);
    for (Writer* writer : pos->second) {
      writers_[writer].erase(reader);
    }
    readers_.erase(reader);
    reader->indexes.erase(this);
  }

  bool empty() const
  {
    return writers_.empty() && readers_.empty();
  }

  void get_readers(Writer* writer, ReaderSet& readers) const
  {
    Writers::const_iterator pos = writers_.find(writer);
    if (pos != writers_.end()) {
      readers.insert(pos->second.begin(), pos->second.end());
    }
  }

  void get_writers(Reader* reader, WriterSet& writers) const
  {
    Readers::const_iterator pos = readers_.find(reader);
    if (pos != readers_.end()) {
      writers.insert(pos->second.begin(), pos->second.end());
    }
  }

private:
  typedef std::map<Writer*, ReaderSet > Writers;
  typedef std::map<Reader*, WriterSet > Readers;

  void match(Writers::iterator pos);
  void match(Readers::iterator pos);

  Writers writers_;
  Readers readers_;
};

template <typename SubIndex>
class Pattern;

template <typename SubIndex>
class Literal {
public:
  typedef Pattern<SubIndex> PatternType;

  Literal() : literal_count_(0) {}

  ~Literal()
  {
    for (PatternType* pattern : patterns_) {
      pattern->erase(this);
    }
  }

  void insert(PatternType* pattern)
  {
    patterns_.insert(pattern);
  }

  void erase(PatternType* pattern)
  {
    patterns_.erase(pattern);
  }

  void insert_literal(Writer* writer)
  {
    index_.insert(writer);
    ++literal_count_;
  }

  void insert_pattern(Writer* writer)
  {
    index_.insert(writer);
  }

  void reinsert(Writer* writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    index_.reinsert(writer, writer_entry);
  }

  void erase_literal(Writer* writer)
  {
    index_.erase(writer);
    --literal_count_;
  }

  void erase_pattern(Writer* writer)
  {
    index_.erase(writer);
  }

  void insert_literal(Reader* reader)
  {
    index_.insert(reader);
    ++literal_count_;
  }

  void insert_pattern(Reader* reader)
  {
    index_.insert(reader);
  }

  void reinsert(Reader* reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    index_.reinsert(reader, reader_entry);
  }

  void erase_literal(Reader* reader)
  {
    index_.erase(reader);
    --literal_count_;
  }

  void erase_pattern(Reader* reader)
  {
    index_.erase(reader);
  }

  bool empty() const
  {
    return literal_count_ == 0;
  }

  void get_readers(Writer* writer, ReaderSet& readers) const
  {
    index_.get_readers(writer, readers);
  }

  void get_writers(Reader* reader, WriterSet& writers) const
  {
    index_.get_writers(reader, writers);
  }

private:
  SubIndex index_;
  size_t literal_count_;
  std::set<PatternType*> patterns_;
};

template <typename SubIndex>
class Pattern {
public:
  typedef Literal<SubIndex> LiteralType;

  ~Pattern() {
    for (LiteralType* literal : literals_) {
      literal->erase(this);
    }
  }

  void insert(LiteralType* literal)
  {
    literals_.insert(literal);
    literal->insert(this);
    for (Writer* writer : writers_) {
      literal->insert_pattern(writer);
    }
    for (Reader* reader : readers_) {
      literal->insert_pattern(reader);
    }
  }

  void erase(LiteralType* literal)
  {
    literals_.erase(literal);
  }

  void insert(Writer* writer)
  {
    writers_.insert(writer);
    for (LiteralType* literal : literals_) {
      literal->insert_pattern(writer);
    }
  }

  void reinsert(Writer* writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    for (LiteralType* literal : literals_) {
      literal->reinsert(writer, writer_entry);
    }
  }

  void erase(Writer* writer)
  {
    for (LiteralType* literal : literals_) {
      literal->erase_pattern(writer);
    }
    writers_.erase(writer);
  }

  void insert(Reader* reader)
  {
    readers_.insert(reader);
    for (LiteralType* literal : literals_) {
      literal->insert_pattern(reader);
    }
  }

  void reinsert(Reader* reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    for (LiteralType* literal : literals_) {
      literal->reinsert(reader, reader_entry);
    }
  }

  void erase(Reader* reader)
  {
    for (LiteralType* literal : literals_) {
      literal->erase_pattern(reader);
    }
    readers_.erase(reader);
  }

  bool empty() const
  {
    return writers_.empty() && readers_.empty();
  }

  void get_readers(Writer* writer, ReaderSet& readers) const
  {
    for (LiteralType* literal : literals_) {
      literal->get_readers(writer, readers);
    }
  }

  void get_writers(Reader* reader, WriterSet& writers) const
  {
    for (LiteralType* literal : literals_) {
      literal->get_writers(reader, writers);
    }
  }

private:
  std::set<LiteralType*> literals_;
  std::set<Writer*> writers_;
  std::set<Reader*> readers_;
};

template <typename SubIndex>
class TrieNode {
public:
  typedef Literal<SubIndex> LiteralType;
  typedef Pattern<SubIndex> PatternType;

  TrieNode() : literal_(nullptr), pattern_(nullptr) {}

  ~TrieNode() {
    if (literal_) {
      delete literal_;
    }
    if (pattern_) {
      delete pattern_;
    }
    for (auto p : children_) {
      delete p.second;
    }
  }

  void insert(const Name& name, LiteralType* literal)
  {
    TrieNode* node = this;

    for (Name::const_iterator pos = name.begin(), limit = name.end(); pos != limit; ++pos) {
      const Atom& atom = *pos;
      typename ChildrenType::const_iterator iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        TrieNode* child = new TrieNode();
        node->children_[atom] = child;
        node = child;
      } else {
        node = iter->second;
      }
    }

    node->literal_ = literal;
  }

  LiteralType* find_literal(const Name& name) const
  {
    if (!name.is_literal()) {
      return nullptr;
    }

    const TrieNode* node = this;

    for (Name::const_iterator pos = name.begin(), limit = name.end(); pos != limit; ++pos) {
      const Atom& atom = *pos;
      typename ChildrenType::const_iterator iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        return nullptr;
      } else {
        node = iter->second;
      }
    }

    return node->literal_;
  }

  std::set<LiteralType*> find_literals(const Name& name) const
  {
    std::set<LiteralType*> literals;

    if (!name.is_pattern()) {
      return literals;
    }

    find_literals(literals, this, name.begin(), name.end());

    return literals;
  }

  void insert(const Name& name, PatternType* pattern)
  {
    TrieNode* node = this;

    for (Name::const_iterator pos = name.begin(), limit = name.end(); pos != limit; ++pos) {
      const Atom& atom = *pos;
      typename ChildrenType::const_iterator iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        TrieNode* child = new TrieNode();
        node->children_[atom] = child;
        node = child;
      } else {
        node = iter->second;
      }
    }

    node->pattern_ = pattern;
  }

  PatternType* find_pattern(const Name& name) const
  {
    if (!name.is_pattern()) {
      return nullptr;
    }

    const TrieNode* node = this;

    for (Name::const_iterator pos = name.begin(), limit = name.end(); pos != limit; ++pos) {
      const Atom& atom = *pos;
      typename ChildrenType::const_iterator iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        return nullptr;
      } else {
        node = iter->second;
      }
    }

    return node->pattern_;
  }

  std::set<PatternType*> find_patterns(const Name& name) const
  {
    std::set<PatternType*> patterns;

    if (!name.is_literal()) {
      return patterns;
    }

    find_patterns(patterns, this, name.begin(), name.end());

    return patterns;
  }

  void remove(const Name& name)
  {
    remove(this, name.begin(), name.end());
  }

  bool empty() const
  {
    return literal_ == nullptr && pattern_ == nullptr && children_.empty();
  }

private:
  typedef std::map<Atom, TrieNode*> ChildrenType;
  ChildrenType children_;
  LiteralType* literal_;
  PatternType* pattern_;

  static void find_literals(std::set<LiteralType*>& literals, const TrieNode* node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      if (node->literal_) {
        literals.insert(node->literal_);
      }
      return;
    }

    const Atom& pattern_atom = *begin;

    switch (pattern_atom.kind()) {
    case Atom::CHARACTER:
      {
        typename ChildrenType::const_iterator pos = node->children_.find(pattern_atom);
        if (pos != node->children_.end()) {
          find_literals(literals, pos->second, std::next(begin), end);
        }
      }
      break;
    case Atom::CHARACTER_CLASS:
      for (typename ChildrenType::const_iterator pos = node->children_.begin(), limit = node->children_.end(); pos != limit; ++pos) {
        if (pos->first.kind() == Atom::CHARACTER && pattern_atom.characters().count(pos->first.character()) != 0) {
          find_literals(literals, pos->second, std::next(begin), end);
        }
      }
      break;
    case Atom::NEGATED_CHARACTER_CLASS:
      for (typename ChildrenType::const_iterator pos = node->children_.begin(), limit = node->children_.end(); pos != limit; ++pos) {
        if (pos->first.kind() == Atom::CHARACTER && pattern_atom.characters().count(pos->first.character()) == 0) {
          find_literals(literals, pos->second, std::next(begin), end);
        }
      }
      break;
    case Atom::WILDCARD:
      for (typename ChildrenType::const_iterator pos = node->children_.begin(), limit = node->children_.end(); pos != limit; ++pos) {
        if (pos->first.kind() == Atom::CHARACTER) {
          find_literals(literals, pos->second, std::next(begin), end);
        }
      }
      break;
    case Atom::GLOB:
      // Glob consumes character and remains.
      for (typename ChildrenType::const_iterator pos = node->children_.begin(), limit = node->children_.end(); pos != limit; ++pos) {
        if (pos->first.kind() == Atom::CHARACTER) {
          find_literals(literals, pos->second, begin, end);
          // Handle special case where glob matches the end.
          if (std::next(begin) == end && pos->second->literal_) {
            literals.insert(pos->second->literal_);
          }
        }
      }
      // Glob matches no characters.
      find_literals(literals, node, std::next(begin), end);
      break;
    }
  }

  static void find_patterns(std::set<PatternType*>& patterns, const TrieNode* node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      if (node->pattern_) {
        patterns.insert(node->pattern_);
      }
      return;
    }

    const Atom& literal_atom = *begin;

    for (typename ChildrenType::const_iterator pos = node->children_.begin(), limit = node->children_.end(); pos != limit; ++pos) {
      const Atom& pattern_atom = pos->first;
      const TrieNode* next_node = pos->second;

      switch (pattern_atom.kind()) {
      case Atom::CHARACTER:
        if (literal_atom.character() == pattern_atom.character()) {
          find_patterns(patterns, next_node, std::next(begin), end);
        }
        break;
      case Atom::CHARACTER_CLASS:
        if (pattern_atom.characters().count(literal_atom.character()) == 1) {
          find_patterns(patterns, next_node, std::next(begin), end);
        }
        break;
      case Atom::NEGATED_CHARACTER_CLASS:
        if (pattern_atom.characters().count(literal_atom.character()) == 0) {
          find_patterns(patterns, next_node, std::next(begin), end);
        }
        break;
      case Atom::WILDCARD:
        find_patterns(patterns, next_node, std::next(begin), end);
        break;
      case Atom::GLOB:
        // Glob consumes no characters.
        find_patterns(patterns, next_node, begin, end);
        // Glob consumes a character and remains.
        find_patterns(patterns, node, std::next(begin), end);
        // Handle special case where glob matches the end.
        if (std::next(begin) == end && next_node->pattern_) {
          patterns.insert(next_node->pattern_);
        }
        break;
      }
    }
  }

  static void remove(TrieNode* node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      node->literal_ = nullptr;
      node->pattern_ = nullptr;
      return;
    }

    const Atom& atom = *begin;
    typename ChildrenType::iterator pos = node->children_.find(atom);
    if (pos != node->children_.end()) {
      remove(pos->second, std::next(begin), end);
      if (pos->second->empty()) {
        delete pos->second;
        node->children_.erase(pos);
      }
    }
  }

};

template <typename SubIndex>
class PartitionIndex {
public:
  typedef Literal<SubIndex> LiteralType;
  typedef Pattern<SubIndex> PatternType;

  void insert(Writer* writer)
  {
    const DDS::PartitionQosPolicy& qos = writer->writer_entry._publisher_qos.partition;
    if (qos.name.length() == 0) {
      index_.insert(writer);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        if (literal == nullptr) {
          literal = new LiteralType();
          node_.insert(name, literal);
          for (PatternType* pattern : node_.find_patterns(name)) {
            pattern->insert(literal);
          }
        }
        literal->insert_literal(writer);
      } else {
        PatternType* pattern = node_.find_pattern(name);
        if (pattern == nullptr) {
          pattern = new PatternType();
          node_.insert(name, pattern);
          for (LiteralType* literal : node_.find_literals(name)) {
            pattern->insert(literal);
          }
        }
        pattern->insert(writer);
      }
    }
  }

  void reinsert(Writer* writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    const DDS::PartitionQosPolicy& old_qos = writer->writer_entry._publisher_qos.partition;
    const DDS::PartitionQosPolicy& new_qos = writer_entry._publisher_qos.partition;

    if (old_qos.name.length() == 0 && new_qos.name.length() == 0) {
      index_.reinsert(writer, writer_entry);
      return;
    }

    if (old_qos.name.length() != 0 && new_qos.name.length() == 0) {
      // Removed partitions.
      for (int idx = 0, limit = old_qos.name.length(); idx != limit; ++idx) {
        Name name(old_qos.name[idx].in());
        if (!name.is_valid()) {
          continue;
        }

        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->erase_literal(writer);
          if (literal->empty()) {
            node_.remove(name);
            delete literal;
          }
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->erase(writer);
          if (pattern->empty()) {
            node_.remove(name);
            delete pattern;
          }
        }
      }

      writer->writer_entry = writer_entry;
      index_.insert(writer);

      return;
    }

    if (old_qos.name.length() == 0 && new_qos.name.length() != 0) {
      // Added partitions.
      index_.erase(writer);
      writer->writer_entry = writer_entry;
      insert(writer);
      return;
    }

    // Changed partitions.
    std::set<Name> old_names;
    for (int idx = 0, limit = old_qos.name.length(); idx != limit; ++idx) {
      Name name(old_qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      old_names.insert(name);
    }

    std::set<Name> new_names;
    for (int idx = 0, limit = new_qos.name.length(); idx != limit; ++idx) {
      Name name(new_qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      new_names.insert(name);
    }

    for (Name name : old_names) {
      if (new_names.count(name) == 0) {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->erase_literal(writer);
          if (literal->empty()) {
            node_.remove(name);
            delete literal;
          }
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->erase(writer);
          if (pattern->empty()) {
            node_.remove(name);
            delete pattern;
          }
        }
      } else {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->reinsert(writer, writer_entry);
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->reinsert(writer, writer_entry);
        }
      }
    }

    writer->writer_entry = writer_entry;

    for (Name name : new_names) {
      if (old_names.count(name) == 0) {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          if (literal == nullptr) {
            literal = new LiteralType();
            node_.insert(name, literal);
            for (PatternType* pattern : node_.find_patterns(name)) {
              pattern->insert(literal);
            }
          }
          literal->insert_literal(writer);
        } else {
          PatternType* pattern = node_.find_pattern(name);
          if (pattern == nullptr) {
            pattern = new PatternType();
            node_.insert(name, pattern);
            for (LiteralType* literal : node_.find_literals(name)) {
              pattern->insert(literal);
            }
          }
          pattern->insert(writer);
        }
      }
    }
  }

  void erase(Writer* writer)
  {
    const DDS::PartitionQosPolicy& qos = writer->writer_entry._publisher_qos.partition;
    if (qos.name.length() == 0) {
      index_.erase(writer);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        literal->erase_literal(writer);
        if (literal->empty()) {
          node_.remove(name);
          delete literal;
        }
      } else {
        PatternType* pattern = node_.find_pattern(name);
        pattern->erase(writer);
        if (pattern->empty()) {
          node_.remove(name);
          delete pattern;
        }
      }
    }
  }

  void insert(Reader* reader)
  {
    const DDS::PartitionQosPolicy& qos = reader->reader_entry._subscriber_qos.partition;
    if (qos.name.length() == 0) {
      index_.insert(reader);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }

      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        if (literal == nullptr) {
          literal = new LiteralType();
          node_.insert(name, literal);
          for (PatternType* pattern : node_.find_patterns(name)) {
            pattern->insert(literal);
          }
        }
        literal->insert_literal(reader);
      } else {
        PatternType* pattern = node_.find_pattern(name);
        if (pattern == nullptr) {
          pattern = new PatternType();
          node_.insert(name, pattern);
          for (LiteralType* literal : node_.find_literals(name)) {
            pattern->insert(literal);
          }
        }
        pattern->insert(reader);
      }
    }
  }

  void reinsert(Reader* reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    const DDS::PartitionQosPolicy& old_qos = reader->reader_entry._subscriber_qos.partition;
    const DDS::PartitionQosPolicy& new_qos = reader_entry._subscriber_qos.partition;

    if (old_qos.name.length() == 0 && new_qos.name.length() == 0) {
      index_.reinsert(reader, reader_entry);
      return;
    }

    if (old_qos.name.length() != 0 && new_qos.name.length() == 0) {
      // Removed partitions.
      for (int idx = 0, limit = old_qos.name.length(); idx != limit; ++idx) {
        Name name(old_qos.name[idx].in());
        if (!name.is_valid()) {
          continue;
        }

        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->erase_literal(reader);
          if (literal->empty()) {
            node_.remove(name);
            delete literal;
          }
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->erase(reader);
          if (pattern->empty()) {
            node_.remove(name);
            delete pattern;
          }
        }
      }

      reader->reader_entry = reader_entry;
      index_.insert(reader);

      return;
    }

    if (old_qos.name.length() == 0 && new_qos.name.length() != 0) {
      // Added partitions.
      index_.erase(reader);
      reader->reader_entry = reader_entry;
      insert(reader);
      return;
    }

    // Changed partitions.
    std::set<Name> old_names;
    for (int idx = 0, limit = old_qos.name.length(); idx != limit; ++idx) {
      Name name(old_qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      old_names.insert(name);
    }

    std::set<Name> new_names;
    for (int idx = 0, limit = new_qos.name.length(); idx != limit; ++idx) {
      Name name(new_qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      new_names.insert(name);
    }

    for (const Name& name : old_names) {
      if (new_names.count(name) == 0) {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->erase_literal(reader);
          if (literal->empty()) {
            node_.remove(name);
            delete literal;
          }
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->erase(reader);
          if (pattern->empty()) {
            node_.remove(name);
            delete pattern;
          }
        }
      } else {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          literal->reinsert(reader, reader_entry);
        } else {
          PatternType* pattern = node_.find_pattern(name);
          pattern->reinsert(reader, reader_entry);
        }
      }
    }

    reader->reader_entry = reader_entry;

    for (const Name& name : new_names) {
      if (old_names.count(name) == 0) {
        if (name.is_literal()) {
          LiteralType* literal = node_.find_literal(name);
          if (literal == nullptr) {
            literal = new LiteralType();
            node_.insert(name, literal);
            for (PatternType* pattern : node_.find_patterns(name)) {
              pattern->insert(literal);
            }
          }
          literal->insert_literal(reader);
        } else {
          PatternType* pattern = node_.find_pattern(name);
          if (pattern == nullptr) {
            pattern = new PatternType();
            node_.insert(name, pattern);
            for (LiteralType* literal : node_.find_literals(name)) {
              pattern->insert(literal);
            }
          }
          pattern->insert(reader);
        }
      }
    }
  }

  void erase(Reader* reader)
  {
    const DDS::PartitionQosPolicy& qos = reader->reader_entry._subscriber_qos.partition;
    if (qos.name.length() == 0) {
      index_.erase(reader);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        literal->erase_literal(reader);
        if (literal->empty()) {
          node_.remove(name);
          delete literal;
        }
      } else {
        PatternType* pattern = node_.find_pattern(name);
        pattern->erase(reader);
        if (pattern->empty()) {
          node_.remove(name);
          delete pattern;
        }
      }
    }
  }

  bool empty() const { return node_.empty() && index_.empty(); }

  void get_readers(Writer* writer, ReaderSet& readers) const
  {
    const DDS::PartitionQosPolicy& qos = writer->writer_entry.publisher_qos().partition;
    if (qos.name.length() == 0) {
      index_.get_readers(writer, readers);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        if (literal) {
          literal->get_readers(writer, readers);
        }
      } else {
        PatternType* pattern = node_.find_pattern(name);
        if (pattern) {
          pattern->get_readers(writer, readers);
        }
      }
    }
  }

  void get_writers(Reader* reader, WriterSet& writers) const
  {
    const DDS::PartitionQosPolicy& qos = reader->reader_entry.subscriber_qos().partition;
    if (qos.name.length() == 0) {
      index_.get_writers(reader, writers);
      return;
    }

    for (int idx = 0, limit = qos.name.length(); idx != limit; ++idx) {
      Name name(qos.name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        LiteralType* literal = node_.find_literal(name);
        if (literal) {
          literal->get_writers(reader, writers);
        }
      } else {
        PatternType* pattern = node_.find_pattern(name);
        if (pattern) {
          pattern->get_writers(reader, writers);
        }
      }
    }
  }

private:
  TrieNode<SubIndex> node_;
  SubIndex index_; // For no partitions.
};

template <typename SubIndex>
class TopicIndex {
public:
  typedef std::pair<std::string, std::string> KeyType;

  void insert(Writer* writer)
  {
    const KeyType key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    auto p = entities_.insert(std::make_pair(key, SubIndex()));
    p.first->second.insert(writer);
  }

  void reinsert(Writer* writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    const KeyType old_key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    const KeyType new_key = std::make_pair(writer_entry.topic_name(), writer_entry.type_name());
    typename Entities::iterator pos = entities_.find(old_key);
    if (old_key == new_key) {
      pos->second.reinsert(writer, writer_entry);
    } else {
      pos->second.erase(writer);
      if (pos->second.empty()) {
        entities_.erase(pos);
      }
      writer->writer_entry = writer_entry;
      insert(writer);
    }
  }

  void erase(Writer* writer)
  {
    const KeyType key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    typename Entities::iterator pos = entities_.find(key);
    pos->second.erase(writer);
    if (pos->second.empty()) {
      entities_.erase(pos);
    }
  }

  void insert(Reader* reader)
  {
    const KeyType key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    auto p = entities_.insert(std::make_pair(key, SubIndex()));
    p.first->second.insert(reader);
  }

  void reinsert(Reader* reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    const KeyType old_key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    const KeyType new_key = std::make_pair(reader_entry.topic_name(), reader_entry.type_name());
    typename Entities::iterator pos = entities_.find(old_key);
    if (old_key == new_key) {
      pos->second.reinsert(reader, reader_entry);
    } else {
      pos->second.erase(reader);
      if (pos->second.empty()) {
        entities_.erase(pos);
      }
      reader->reader_entry = reader_entry;
      insert(reader);
    }
  }

  void erase(Reader* reader)
  {
    const KeyType key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    typename Entities::iterator pos = entities_.find(key);
    pos->second.erase(reader);
    if (pos->second.empty()) {
      entities_.erase(pos);
    }
  }

  void get_readers(Writer* writer, ReaderSet& readers) const
  {
    const KeyType key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    typename Entities::const_iterator pos = entities_.find(key);
    if (pos != entities_.end()) {
      pos->second.get_readers(writer, readers);
    }
  }

  void get_writers(Reader* reader, WriterSet& writers) const
  {
    const KeyType key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    typename Entities::const_iterator pos = entities_.find(key);
    if (pos != entities_.end()) {
      pos->second.get_writers(reader, writers);
    }
  }

private:
  typedef std::map<std::pair<std::string, std::string>, SubIndex> Entities;
  Entities entities_;
};

#endif // RTPSRELAY_QOS_INDEX_H_
