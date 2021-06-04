#ifndef RTPSRELAY_PARTITION_INDEX_H_
#define RTPSRELAY_PARTITION_INDEX_H_

#include "Name.h"
#include "Utility.h"

#include <unordered_map>

namespace RtpsRelay {

class TrieNode {
public:
  typedef std::shared_ptr<TrieNode> NodePtr;

  static void insert(NodePtr node, const Name& name, const OpenDDS::DCPS::GUID_t& guid)
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

    node->guids_.insert(guid);
  }

  static void remove(NodePtr node, const Name& name, const OpenDDS::DCPS::GUID_t& guid)
  {
    remove(node, name.begin(), name.end(), guid);
  }

  bool empty() const
  {
    return guids_.empty() && children_.empty();
  }

  static void lookup(NodePtr node, const Name& name, GuidSet& guids)
  {
    if (name.is_literal()) {
      lookup_literal(node, name.begin(), name.end(), false, guids);
    } else {
      lookup_pattern(node, name.begin(), name.end(), guids);
    }
  }

private:
  typedef std::map<Atom, NodePtr> ChildrenType;
  ChildrenType children_;
  GuidSet guids_;

  struct Transformer {
    OpenDDS::DCPS::GUID_t operator()(const OpenDDS::DCPS::GUID_t& x) const
    {
      return OpenDDS::DCPS::make_part_guid(x);
    }
  };

  static void insert_guids(NodePtr node,
                           GuidSet& guids)
  {
    std::transform(node->guids_.begin(), node->guids_.end(), std::inserter(guids, guids.begin()), Transformer());
  }


  static void lookup_literal(NodePtr node,
                             Name::const_iterator begin,
                             Name::const_iterator end,
                             bool glob_only,
                             GuidSet& guids)
  {
    if (begin == end) {
      insert_guids(node, guids);
      lookup_globs(node, guids);
      return;
    }

    const auto& atom = *begin;

    for (const auto& pos : node->children_) {
      switch (pos.first.kind()) {
      case Atom::CHARACTER:
        if (!glob_only && pos.first == atom) {
          lookup_literal(pos.second, std::next(begin), end, false, guids);
        }
        break;
      case Atom::CHARACTER_CLASS:
        if (!glob_only && pos.first.characters().count(atom.character()) != 0) {
          lookup_literal(pos.second, std::next(begin), end, false, guids);
        }
        break;
      case Atom::NEGATED_CHARACTER_CLASS:
        if (!glob_only && pos.first.characters().count(atom.character()) == 0) {
          lookup_literal(pos.second, std::next(begin), end, false, guids);
        }
        break;
      case Atom::WILDCARD:
        if (!glob_only) {
          lookup_literal(pos.second, std::next(begin), end, false, guids);
        }
        break;
      case Atom::GLOB:
        // Glob consumes character and remains.
        lookup_literal(node, std::next(begin), end, true, guids);
        // Glob matches no characters.
        lookup_literal(pos.second, begin, end, false, guids);
        break;
      }
    }
  }

  static void lookup_globs(NodePtr node,
                           GuidSet& guids)
  {
    for (const auto& pos : node->children_) {
      if (pos.first.kind() == Atom::GLOB) {
        insert_guids(pos.second, guids);
        lookup_globs(pos.second, guids);
      }
    }
  }

  static void lookup_pattern(NodePtr node,
                             Name::const_iterator begin,
                             Name::const_iterator end,
                             GuidSet& guids)
  {
    if (begin == end) {
      insert_guids(node, guids);
      return;
    }

    const auto& atom = *begin;

    switch (atom.kind()) {
    case Atom::CHARACTER:
      {
        const auto pos = node->children_.find(atom);
        if (pos != node->children_.end()) {
          lookup_pattern(pos->second, std::next(begin), end, guids);
        }
      }
      break;
    case Atom::CHARACTER_CLASS:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER && atom.characters().count(p.first.character()) != 0) {
          lookup_pattern(p.second, std::next(begin), end, guids);
        }
      }
      break;
    case Atom::NEGATED_CHARACTER_CLASS:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER && atom.characters().count(p.first.character()) == 0) {
          lookup_pattern(p.second, std::next(begin), end, guids);
        }
      }
      break;
    case Atom::WILDCARD:
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER) {
          lookup_pattern(p.second, std::next(begin), end, guids);
        }
      }
      break;
    case Atom::GLOB:
      // Glob consumes character and remains.
      for (const auto& p : node->children_) {
        if (p.first.kind() == Atom::CHARACTER) {
          lookup_pattern(p.second, begin, end, guids);
        }
      }
      // Glob matches no characters.
      lookup_pattern(node, std::next(begin), end, guids);
      break;
    }
  }

  static void remove(NodePtr node,
                     Name::const_iterator begin,
                     Name::const_iterator end,
                     const OpenDDS::DCPS::GUID_t& guid)
  {
    if (begin == end) {
      node->guids_.erase(guid);
      return;
    }

    const auto& atom = *begin;
    const auto pos = node->children_.find(atom);
    if (pos != node->children_.end()) {
      remove(pos->second, std::next(begin), end, guid);
      if (pos->second->empty()) {
        node->children_.erase(pos);
      }
    }
  }
};

class PartitionIndex {
public:
  PartitionIndex()
    : root_(new TrieNode())
  {}

  void insert(const std::string& name, const OpenDDS::DCPS::GUID_t& guid)
  {
    TrieNode::insert(root_, Name(name), guid);
    cache_.clear();
  }

  void remove(const std::string& name, const OpenDDS::DCPS::GUID_t& guid)
  {
    TrieNode::remove(root_, Name(name), guid);
    cache_.clear();
  }

  void lookup(const std::string& name, GuidSet& guids) const
  {
    const auto pos = cache_.find(name);
    if (pos != cache_.end()) {
      guids = pos->second;
      return;
    }
    TrieNode::lookup(root_, Name(name), guids);
    cache_[name] = guids;
  }

private:
  TrieNode::NodePtr root_;
  typedef std::unordered_map<std::string, GuidSet> Cache;
  mutable Cache cache_;
};

}

#endif // RTPSRELAY_PARTITION_INDEX_H_
