#ifndef RTPSRELAY_QOS_INDEX_H_
#define RTPSRELAY_QOS_INDEX_H_

#include "Name.h"
#include "RelayTypeSupportImpl.h"

namespace RtpsRelay {

class NoIndex;

struct RtpsRelayLib_Export Writer {
  Writer(const RtpsRelay::WriterEntry& we, bool local) : writer_entry(we), local_(local) {}
  RtpsRelay::WriterEntry writer_entry;
  std::set<NoIndex*> indexes;

  bool local() const { return local_; }
  bool remote() const { return !local_; }

private:
  const bool local_;
};

typedef std::shared_ptr<Writer> WriterPtr;
typedef std::set<WriterPtr> WriterSet;

struct RtpsRelayLib_Export Reader {
  Reader(const RtpsRelay::ReaderEntry& re, bool local) : reader_entry(re), local_(local) {}
  RtpsRelay::ReaderEntry reader_entry;
  std::set<NoIndex*> indexes;

  bool local() const { return local_; }
  bool remote() const { return !local_; }

private:
  const bool local_;
};

typedef std::shared_ptr<Reader> ReaderPtr;
typedef std::set<ReaderPtr> ReaderSet;

class RtpsRelayLib_Export NoIndex {
public:
  ~NoIndex()
  {
    for (auto& p : writers_) {
      p.first->indexes.erase(this);
    }
    for (auto& p : readers_) {
      p.first->indexes.erase(this);
    }
  }

  void insert(WriterPtr writer)
  {
    const auto p = writers_.insert(std::make_pair(writer, ReaderSet()));
    writer->indexes.insert(this);
    match(p.first);
  }

  void reinsert(WriterPtr writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    writer->writer_entry = writer_entry;
    match(writers_.find(writer));
  }

  void erase(WriterPtr writer)
  {
    const auto pos = writers_.find(writer);
    for (const auto reader : pos->second) {
      readers_[reader].erase(writer);
    }
    writers_.erase(writer);
    writer->indexes.erase(this);
  }

  void insert(ReaderPtr reader)
  {
    const auto p = readers_.insert(std::make_pair(reader, WriterSet()));
    reader->indexes.insert(this);
    match(p.first);
  }

  void reinsert(ReaderPtr reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    reader->reader_entry = reader_entry;
    match(readers_.find(reader));
  }

  void erase(ReaderPtr reader)
  {
    const auto pos = readers_.find(reader);
    for (const auto writer : pos->second) {
      writers_[writer].erase(reader);
    }
    readers_.erase(reader);
    reader->indexes.erase(this);
  }

  bool empty() const
  {
    return writers_.empty() && readers_.empty();
  }

  void get_readers(WriterPtr writer, ReaderSet& readers) const
  {
    const auto pos = writers_.find(writer);
    if (pos != writers_.end()) {
      readers.insert(pos->second.begin(), pos->second.end());
    }
  }

  void get_writers(ReaderPtr reader, WriterSet& writers) const
  {
    const auto pos = readers_.find(reader);
    if (pos != readers_.end()) {
      writers.insert(pos->second.begin(), pos->second.end());
    }
  }

private:
  typedef std::map<WriterPtr, ReaderSet> Writers;
  typedef std::map<ReaderPtr, WriterSet> Readers;

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
  typedef std::shared_ptr<Literal> LiteralPtr;
  typedef std::shared_ptr<Pattern<SubIndex> > PatternPtr;

  Literal() : literal_count_(0) {}

  static void remove_from_patterns(LiteralPtr literal)
  {
    for (PatternPtr pattern : literal->patterns_) {
      pattern->erase(literal);
    }
    literal->patterns_.clear();
  }

  void insert(PatternPtr pattern)
  {
    patterns_.insert(pattern);
  }

  void erase(PatternPtr pattern)
  {
    patterns_.erase(pattern);
  }

  void insert_literal(WriterPtr writer)
  {
    index_.insert(writer);
    ++literal_count_;
  }

  void insert_pattern(WriterPtr writer)
  {
    index_.insert(writer);
  }

  void reinsert(WriterPtr writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    index_.reinsert(writer, writer_entry);
  }

  void erase_literal(WriterPtr writer)
  {
    index_.erase(writer);
    --literal_count_;
  }

  void erase_pattern(WriterPtr writer)
  {
    index_.erase(writer);
  }

  void insert_literal(ReaderPtr reader)
  {
    index_.insert(reader);
    ++literal_count_;
  }

  void insert_pattern(ReaderPtr reader)
  {
    index_.insert(reader);
  }

  void reinsert(ReaderPtr reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    index_.reinsert(reader, reader_entry);
  }

  void erase_literal(ReaderPtr reader)
  {
    index_.erase(reader);
    --literal_count_;
  }

  void erase_pattern(ReaderPtr reader)
  {
    index_.erase(reader);
  }

  bool empty() const
  {
    return literal_count_ == 0;
  }

  void get_readers(WriterPtr writer, ReaderSet& readers) const
  {
    index_.get_readers(writer, readers);
  }

  void get_writers(ReaderPtr reader, WriterSet& writers) const
  {
    index_.get_writers(reader, writers);
  }

private:
  SubIndex index_;
  size_t literal_count_;
  std::set<PatternPtr> patterns_;
};

template <typename SubIndex>
class Pattern {
public:
  typedef std::shared_ptr<Pattern> PatternPtr;
  typedef Literal<SubIndex> LiteralType;
  typedef std::shared_ptr<LiteralType> LiteralPtr;

  static void remove_from_literals(PatternPtr pattern)
  {
    for (const auto literal : pattern->literals_) {
      literal->erase(pattern);
    }
    pattern->literals_.clear();
  }

  static void insert(PatternPtr pattern, LiteralPtr literal)
  {
    pattern->literals_.insert(literal);
    literal->insert(pattern);
    for (const auto writer : pattern->writers_) {
      literal->insert_pattern(writer);
    }
    for (const auto reader : pattern->readers_) {
      literal->insert_pattern(reader);
    }
  }

  void erase(LiteralPtr literal)
  {
    literals_.erase(literal);
  }

  void insert(WriterPtr writer)
  {
    writers_.insert(writer);
    for (const auto literal : literals_) {
      literal->insert_pattern(writer);
    }
  }

  void reinsert(WriterPtr writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    for (const auto literal : literals_) {
      literal->reinsert(writer, writer_entry);
    }
  }

  void erase(WriterPtr writer)
  {
    for (const auto literal : literals_) {
      literal->erase_pattern(writer);
    }
    writers_.erase(writer);
  }

  void insert(ReaderPtr reader)
  {
    readers_.insert(reader);
    for (const auto literal : literals_) {
      literal->insert_pattern(reader);
    }
  }

  void reinsert(ReaderPtr reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    for (const auto literal : literals_) {
      literal->reinsert(reader, reader_entry);
    }
  }

  void erase(ReaderPtr reader)
  {
    for (const auto literal : literals_) {
      literal->erase_pattern(reader);
    }
    readers_.erase(reader);
  }

  bool empty() const
  {
    return writers_.empty() && readers_.empty();
  }

  void get_readers(WriterPtr writer, ReaderSet& readers) const
  {
    for (const auto literal : literals_) {
      literal->get_readers(writer, readers);
    }
  }

  void get_writers(ReaderPtr reader, WriterSet& writers) const
  {
    for (const auto literal : literals_) {
      literal->get_writers(reader, writers);
    }
  }

private:
  std::set<LiteralPtr> literals_;
  WriterSet writers_;
  ReaderSet readers_;
};

template <typename SubIndex>
class TrieNode {
public:
  typedef Literal<SubIndex> LiteralType;
  typedef std::shared_ptr<LiteralType> LiteralPtr;
  typedef Pattern<SubIndex> PatternType;
  typedef std::shared_ptr<PatternType> PatternPtr;
  typedef std::shared_ptr<TrieNode> NodePtr;

  TrieNode() : pattern_(nullptr) {}

  ~TrieNode()
  {
    if (literal_) {
      LiteralType::remove_from_patterns(literal_);
    }
    if (pattern_) {
      PatternType::remove_from_literals(pattern_);
    }
  }

  static void insert(NodePtr node, const Name& name, LiteralPtr literal)
  {
    for (const auto& atom : name) {
      const auto iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        NodePtr child(new TrieNode());
        node->children_[atom] = child;
        node = child;
      } else {
        node = iter->second;
      }
    }

    node->literal_ = literal;
  }

  static LiteralPtr find_literal(NodePtr node, const Name& name)
  {
    if (!name.is_literal()) {
      return nullptr;
    }

    for (const auto& atom : name) {
      const auto iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        return nullptr;
      } else {
        node = iter->second;
      }
    }

    return node->literal_;
  }

  static std::set<LiteralPtr> find_literals(NodePtr node, const Name& name)
  {
    std::set<LiteralPtr> literals;

    if (!name.is_pattern()) {
      return literals;
    }

    find_literals(literals, node, name.begin(), name.end());

    return literals;
  }

  static void insert(NodePtr node, const Name& name, PatternPtr pattern)
  {
    for (const auto& atom : name) {
      const auto iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        NodePtr child(new TrieNode());
        node->children_[atom] = child;
        node = child;
      } else {
        node = iter->second;
      }
    }

    node->pattern_ = pattern;
  }

  static PatternPtr find_pattern(NodePtr node, const Name& name)
  {
    if (!name.is_pattern()) {
      return nullptr;
    }

    for (const auto& atom : name) {
      const auto iter = node->children_.find(atom);
      if (iter == node->children_.end()) {
        return nullptr;
      } else {
        node = iter->second;
      }
    }

    return node->pattern_;
  }

  static std::set<PatternPtr> find_patterns(NodePtr node, const Name& name)
  {
    std::set<PatternPtr> patterns;

    if (!name.is_literal()) {
      return patterns;
    }

    find_patterns(patterns, node, name.begin(), name.end());

    return patterns;
  }

  static void remove(NodePtr node, const Name& name)
  {
    remove(node, name.begin(), name.end());
  }

  bool empty() const
  {
    return literal_ == nullptr && pattern_ == nullptr && children_.empty();
  }

private:
  typedef std::map<Atom, NodePtr> ChildrenType;
  ChildrenType children_;
  LiteralPtr literal_;
  PatternPtr pattern_;

  static void find_literals(std::set<LiteralPtr>& literals, NodePtr node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      if (node->literal_) {
        literals.insert(node->literal_);
      }
      return;
    }

    const auto& pattern_atom = *begin;

    switch (pattern_atom.kind()) {
    case Atom::CHARACTER:
      {
        const auto pos = node->children_.find(pattern_atom);
        if (pos != node->children_.end()) {
          find_literals(literals, pos->second, std::next(begin), end);
        }
      }
      break;
    case Atom::CHARACTER_CLASS:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER && pattern_atom.characters().count(p.first.character()) != 0) {
          find_literals(literals, p.second, std::next(begin), end);
        }
      }
      break;
    case Atom::NEGATED_CHARACTER_CLASS:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER && pattern_atom.characters().count(p.first.character()) == 0) {
          find_literals(literals, p.second, std::next(begin), end);
        }
      }
      break;
    case Atom::WILDCARD:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER) {
          find_literals(literals, p.second, std::next(begin), end);
        }
      }
      break;
    case Atom::GLOB:
      // Glob consumes character and remains.
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER) {
          find_literals(literals, p.second, begin, end);
          // Handle special case where glob matches the end.
          if (std::next(begin) == end && p.second->literal_) {
            literals.insert(p.second->literal_);
          }
        }
      }
      // Glob matches no characters.
      find_literals(literals, node, std::next(begin), end);
      break;
    }
  }

  static void find_patterns(std::set<PatternPtr>& patterns, NodePtr node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      if (node->pattern_) {
        patterns.insert(node->pattern_);
      }
      return;
    }

    const auto& literal_atom = *begin;

    for (const auto p : node->children_) {
      const auto& pattern_atom = p.first;
      const auto next_node = p.second;

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

  static void remove(NodePtr node, Name::const_iterator begin, Name::const_iterator end)
  {
    if (begin == end) {
      node->literal_ = nullptr;
      node->pattern_ = nullptr;
      return;
    }

    const auto& atom = *begin;
    const auto pos = node->children_.find(atom);
    if (pos != node->children_.end()) {
      remove(pos->second, std::next(begin), end);
      if (pos->second->empty()) {
        node->children_.erase(pos);
      }
    }
  }

};

template <typename SubIndex>
class PartitionIndex {
public:
  typedef Literal<SubIndex> LiteralType;
  typedef std::shared_ptr<LiteralType> LiteralPtr;
  typedef Pattern<SubIndex> PatternType;
  typedef std::shared_ptr<PatternType> PatternPtr;
  typedef TrieNode<SubIndex> NodeType;

  PartitionIndex() : node_(new NodeType())
  {
    empty_qos_.name.length(1);
    empty_qos_.name[0] = "";
  }

  void insert(WriterPtr writer)
  {
    const auto qos = choose_qos(&writer->writer_entry._publisher_qos.partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        auto literal = NodeType::find_literal(node_, name);
        if (literal == nullptr) {
          literal = LiteralPtr(new LiteralType());
          NodeType::insert(node_, name, literal);
          for (const auto pattern : NodeType::find_patterns(node_, name)) {
            PatternType::insert(pattern, literal);
          }
        }
        literal->insert_literal(writer);
      } else {
        auto pattern = NodeType::find_pattern(node_, name);
        if (pattern == nullptr) {
          pattern = PatternPtr(new PatternType());
          NodeType::insert(node_, name, pattern);
          for (const auto literal : NodeType::find_literals(node_, name)) {
            PatternType::insert(pattern, literal);
          }
        }
        pattern->insert(writer);
      }
    }
  }

  void reinsert(WriterPtr writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    const auto old_qos = choose_qos(&writer->writer_entry._publisher_qos.partition);
    const auto new_qos = choose_qos(&writer_entry._publisher_qos.partition);

    std::set<Name> old_names;
    for (int idx = 0, limit = old_qos->name.length(); idx != limit; ++idx) {
      Name name(old_qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      old_names.insert(name);
    }

    std::set<Name> new_names;
    for (int idx = 0, limit = new_qos->name.length(); idx != limit; ++idx) {
      Name name(new_qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      new_names.insert(name);
    }

    for (Name name : old_names) {
      if (new_names.count(name) == 0) {
        if (name.is_literal()) {
          const auto literal = NodeType::find_literal(node_, name);
          literal->erase_literal(writer);
          if (literal->empty()) {
            NodeType::remove(node_, name);
            LiteralType::remove_from_patterns(literal);
          }
        } else {
          const auto pattern = NodeType::find_pattern(node_, name);
          pattern->erase(writer);
          if (pattern->empty()) {
            NodeType::remove(node_, name);
            PatternType::remove_from_literals(pattern);
          }
        }
      } else {
        if (name.is_literal()) {
          const auto literal = NodeType::find_literal(node_, name);
          literal->reinsert(writer, writer_entry);
        } else {
          const auto pattern = NodeType::find_pattern(node_, name);
          pattern->reinsert(writer, writer_entry);
        }
      }
    }

    writer->writer_entry = writer_entry;

    for (Name name : new_names) {
      if (old_names.count(name) == 0) {
        if (name.is_literal()) {
          auto literal = NodeType::find_literal(node_, name);
          if (literal == nullptr) {
            literal = LiteralPtr(new LiteralType());
            NodeType::insert(node_, name, literal);
            for (const auto pattern : NodeType::find_patterns(node_, name)) {
              PatternType::insert(pattern, literal);
            }
          }
          literal->insert_literal(writer);
        } else {
          auto pattern = NodeType::find_pattern(node_, name);
          if (pattern == nullptr) {
            pattern = PatternPtr(new PatternType());
            NodeType::insert(node_, name, pattern);
            for (const auto literal : NodeType::find_literals(node_, name)) {
              PatternType::insert(pattern, literal);
            }
          }
          pattern->insert(writer);
        }
      }
    }
  }

  void erase(WriterPtr writer)
  {
    const auto qos = choose_qos(&writer->writer_entry._publisher_qos.partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        const auto literal = NodeType::find_literal(node_, name);
        literal->erase_literal(writer);
        if (literal->empty()) {
          NodeType::remove(node_, name);
          LiteralType::remove_from_patterns(literal);
        }
      } else {
        const auto pattern = NodeType::find_pattern(node_, name);
        pattern->erase(writer);
        if (pattern->empty()) {
          NodeType::remove(node_, name);
          PatternType::remove_from_literals(pattern);
        }
      }
    }
  }

  void insert(ReaderPtr reader)
  {
    const auto qos = choose_qos(&reader->reader_entry._subscriber_qos.partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }

      if (name.is_literal()) {
        auto literal = NodeType::find_literal(node_, name);
        if (literal == nullptr) {
          literal = LiteralPtr(new LiteralType());
          NodeType::insert(node_, name, literal);
          for (const auto pattern : NodeType::find_patterns(node_, name)) {
            PatternType::insert(pattern, literal);
          }
        }
        literal->insert_literal(reader);
      } else {
        auto pattern = NodeType::find_pattern(node_, name);
        if (pattern == nullptr) {
          pattern = PatternPtr(new PatternType());
          NodeType::insert(node_, name, pattern);
          for (const auto literal : NodeType::find_literals(node_, name)) {
            PatternType::insert(pattern, literal);
          }
        }
        pattern->insert(reader);
      }
    }
  }

  void reinsert(ReaderPtr reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    const auto old_qos = choose_qos(&reader->reader_entry._subscriber_qos.partition);
    const auto new_qos = choose_qos(&reader_entry._subscriber_qos.partition);

    // Changed partitions.
    std::set<Name> old_names;
    for (int idx = 0, limit = old_qos->name.length(); idx != limit; ++idx) {
      Name name(old_qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      old_names.insert(name);
    }

    std::set<Name> new_names;
    for (int idx = 0, limit = new_qos->name.length(); idx != limit; ++idx) {
      Name name(new_qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      new_names.insert(name);
    }

    for (const Name& name : old_names) {
      if (new_names.count(name) == 0) {
        if (name.is_literal()) {
          const auto literal = NodeType::find_literal(node_, name);
          literal->erase_literal(reader);
          if (literal->empty()) {
            NodeType::remove(node_, name);
            LiteralType::remove_from_patterns(literal);
          }
        } else {
          const auto pattern = NodeType::find_pattern(node_, name);
          pattern->erase(reader);
          if (pattern->empty()) {
            NodeType::remove(node_, name);
            PatternType::remove_from_literals(pattern);
          }
        }
      } else {
        if (name.is_literal()) {
          const auto literal = NodeType::find_literal(node_, name);
          literal->reinsert(reader, reader_entry);
        } else {
          const auto pattern = NodeType::find_pattern(node_, name);
          pattern->reinsert(reader, reader_entry);
        }
      }
    }

    reader->reader_entry = reader_entry;

    for (const Name& name : new_names) {
      if (old_names.count(name) == 0) {
        if (name.is_literal()) {
          auto literal = NodeType::find_literal(node_, name);
          if (literal == nullptr) {
            literal = LiteralPtr(new LiteralType());
            NodeType::insert(node_, name, literal);
            for (const auto pattern : NodeType::find_patterns(node_, name)) {
              PatternType::insert(pattern, literal);
            }
          }
          literal->insert_literal(reader);
        } else {
          auto pattern = NodeType::find_pattern(node_, name);
          if (pattern == nullptr) {
            pattern = PatternPtr(new PatternType());
            NodeType::insert(node_, name, pattern);
            for (const auto literal : NodeType::find_literals(node_, name)) {
              PatternType::insert(pattern, literal);
            }
          }
          pattern->insert(reader);
        }
      }
    }
  }

  void erase(ReaderPtr reader)
  {
    const auto qos = choose_qos(&reader->reader_entry._subscriber_qos.partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        const auto literal = NodeType::find_literal(node_, name);
        literal->erase_literal(reader);
        if (literal->empty()) {
          NodeType::remove(node_, name);
          LiteralType::remove_from_patterns(literal);
        }
      } else {
        const auto pattern = NodeType::find_pattern(node_, name);
        pattern->erase(reader);
        if (pattern->empty()) {
          NodeType::remove(node_, name);
          PatternType::remove_from_literals(pattern);
        }
      }
    }
  }

  bool empty() const { return node_->empty(); }

  void get_readers(WriterPtr writer, ReaderSet& readers) const
  {
    const auto qos = choose_qos(&writer->writer_entry.publisher_qos().partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        const auto literal = NodeType::find_literal(node_, name);
        if (literal) {
          literal->get_readers(writer, readers);
        }
      } else {
        const auto pattern = NodeType::find_pattern(node_, name);
        if (pattern) {
          pattern->get_readers(writer, readers);
        }
      }
    }
  }

  void get_writers(ReaderPtr reader, WriterSet& writers) const
  {
    const auto qos = choose_qos(&reader->reader_entry.subscriber_qos().partition);

    for (int idx = 0, limit = qos->name.length(); idx != limit; ++idx) {
      Name name(qos->name[idx].in());
      if (!name.is_valid()) {
        continue;
      }
      if (name.is_literal()) {
        const auto literal = NodeType::find_literal(node_, name);
        if (literal) {
          literal->get_writers(reader, writers);
        }
      } else {
        const auto pattern = NodeType::find_pattern(node_, name);
        if (pattern) {
          pattern->get_writers(reader, writers);
        }
      }
    }
  }

private:
  const DDS::PartitionQosPolicy* choose_qos(const DDS::PartitionQosPolicy* qos) const
  {
    if (qos->name.length() == 0) {
      return &empty_qos_;
    } else {
      return qos;
    }
  }

  typename NodeType::NodePtr node_;
  DDS::PartitionQosPolicy empty_qos_;
};

template <typename SubIndex>
class TopicIndex {
public:
  typedef std::pair<std::string, std::string> KeyType;

  void insert(WriterPtr writer)
  {
    const auto key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    auto p = entities_.insert(std::make_pair(key, SubIndex()));
    p.first->second.insert(writer);
  }

  void reinsert(WriterPtr writer, const RtpsRelay::WriterEntry& writer_entry)
  {
    const auto old_key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    const auto new_key = std::make_pair(writer_entry.topic_name(), writer_entry.type_name());
    const auto pos = entities_.find(old_key);
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

  void erase(WriterPtr writer)
  {
    const auto key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    const auto pos = entities_.find(key);
    pos->second.erase(writer);
    if (pos->second.empty()) {
      entities_.erase(pos);
    }
  }

  void insert(ReaderPtr reader)
  {
    const auto key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    const auto p = entities_.insert(std::make_pair(key, SubIndex()));
    p.first->second.insert(reader);
  }

  void reinsert(ReaderPtr reader, const RtpsRelay::ReaderEntry& reader_entry)
  {
    const auto old_key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    const auto new_key = std::make_pair(reader_entry.topic_name(), reader_entry.type_name());
    const auto pos = entities_.find(old_key);
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

  void erase(ReaderPtr reader)
  {
    const auto key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    const auto pos = entities_.find(key);
    pos->second.erase(reader);
    if (pos->second.empty()) {
      entities_.erase(pos);
    }
  }

  void get_readers(WriterPtr writer, ReaderSet& readers) const
  {
    const auto key = std::make_pair(writer->writer_entry.topic_name(), writer->writer_entry.type_name());
    const auto pos = entities_.find(key);
    if (pos != entities_.end()) {
      pos->second.get_readers(writer, readers);
    }
  }

  void get_writers(ReaderPtr reader, WriterSet& writers) const
  {
    const auto key = std::make_pair(reader->reader_entry.topic_name(), reader->reader_entry.type_name());
    const auto pos = entities_.find(key);
    if (pos != entities_.end()) {
      pos->second.get_writers(reader, writers);
    }
  }

private:
  typedef std::map<std::pair<std::string, std::string>, SubIndex> Entities;
  Entities entities_;
};

}

#endif // RTPSRELAY_QOS_INDEX_H_
