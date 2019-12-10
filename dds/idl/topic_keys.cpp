#include "topic_keys.h"

#include "be_extern.h"
#include "dds_generator.h"

#include <idl_defines.h>
#include <ast_structure.h>
#include <ast_field.h>
#include <utl_identifier.h>
#include <ast_union.h>
#include <ast_array.h>

#include <string>

TopicKeys::RootType TopicKeys::root_type(AST_Type* type)
{
  if (!type) {
    return InvalidType;
  }
  switch (type->unaliased_type()->node_type()) {
  case AST_Decl::NT_pre_defined:
  case AST_Decl::NT_string:
  case AST_Decl::NT_wstring:
  case AST_Decl::NT_enum:
    return PrimitiveType;
  case AST_Decl::NT_struct:
    return StructureType;
  case AST_Decl::NT_union:
    return UnionType;
  case AST_Decl::NT_array:
    return ArrayType;
  default:
    return InvalidType;
  }
}

TopicKeys::Error::Error()
  : node_(0)
{
}

TopicKeys::Error::Error(AST_Decl* node, const std::string& message)
  : node_(node)
  , message_(message)
{
}

TopicKeys::Error::~Error() throw ()
{
}

TopicKeys::Error& TopicKeys::Error::operator=(const TopicKeys::Error& error)
{
  node_ = error.node_;
  message_ = error.message_;
  return *this;
}

const char* TopicKeys::Error::what() const throw()
{
  return message_.c_str();
}

AST_Decl* TopicKeys::Error::node() {
  return node_;
}

TopicKeys::Iterator::Iterator()
  : parent_(0)
  , pos_(0)
  , child_(0)
  , current_value_(0)
  , root_(0)
  , root_type_(InvalidType)
  , level_(0)
  , recursive_(false)
  , element_count_(0)
  , implied_keys_(false)
{
}

TopicKeys::Iterator::Iterator(TopicKeys& parent)
  : parent_(0)
  , pos_(0)
  , child_(0)
  , current_value_(0)
  , level_(0)
  , recursive_(parent.recursive())
  , element_count_(0)
  , implied_keys_(false)
{
  root_ = parent.root();
  root_type_ = parent.root_type();
  ++*this;
}

TopicKeys::Iterator::Iterator(AST_Type* root, Iterator* parent)
  : parent_(parent)
  , pos_(0)
  , child_(0)
  , current_value_(0)
  , level_(parent->level() + 1)
  , recursive_(parent->recursive_)
  , element_count_(0)
  , implied_keys_(false)
{
  root_type_ = TopicKeys::root_type(root);
  root_ = root;
  ++(*this);
}

TopicKeys::Iterator::Iterator(AST_Field* root, Iterator* parent)
  : parent_(parent)
  , pos_(0)
  , child_(0)
  , current_value_(0)
  , level_(parent->level() + 1)
  , recursive_(parent->recursive_)
  , element_count_(0)
  , implied_keys_(false)
{
  AST_Type* type = root->field_type()->unaliased_type();
  root_type_ = TopicKeys::root_type(type);
  if (root_type_ == PrimitiveType) {
    root_ = root;
  } else {
    root_ = type;
  }
  ++(*this);
}

TopicKeys::Iterator::Iterator(const Iterator& other)
  : pos_(0)
  , child_(0)
  , current_value_(0)
  , root_(0)
  , root_type_(InvalidType)
  , level_(0)
  , recursive_(false)
  , element_count_(0)
  , implied_keys_(false)
{
  *this = other;
}

TopicKeys::Iterator::~Iterator()
{
  cleanup();
}

TopicKeys::Iterator& TopicKeys::Iterator::operator=(const TopicKeys::Iterator& other)
{
  cleanup();
  parent_ = other.parent_;
  pos_ = other.pos_;
  current_value_ = other.current_value_;
  root_ = other.root_;
  root_type_ = other.root_type_;
  level_ = other.level_;
  recursive_ = other.recursive_;
  dimensions_ = other.dimensions_;
  child_ = other.child_ ? new Iterator(*other.child_) : 0;
  element_count_ = other.element_count_;
  return *this;
}

TopicKeys::Iterator& TopicKeys::Iterator::operator++()
{
  // Nop if we are a invalid iterator of any type
  if (!root_ || root_type_ == InvalidType) {
    return *this;
  }

  // If we have a child iterator, ask it for the next value
  if (child_) {
    Iterator& child = *child_;
    ++child;
    if (child == end_value()) {
      delete child_;
      child_ = 0;
      pos_++;
    } else {
      current_value_ = *child;
      return *this;
    }
  }

  if (root_type_ == StructureType) {
    // If we are recursive and at a structure, look for key fields
    if (recursive_ || level_ == 0) {
      AST_Structure* struct_root = dynamic_cast<AST_Structure*>(root_);
      if (!struct_root) {
        throw Error(root_, "Invalid Key Iterator");
      }
      const Fields fields(struct_root);
      const Fields::Iterator fields_end = fields.end();

      // If a nested struct marked as a key has no keys, all the fields are
      // implied to be keys (expect those marked with @key(FALSE)).
      if (pos_ == 0) {
        if (level_ > 0) {
          implied_keys_ = true;
          for (Fields::Iterator i = fields.begin(); i != fields_end && implied_keys_; ++i) {
            bool key_annotation_value;
            const bool has_key_annotation = be_global->check_key(*i, key_annotation_value);
            if (has_key_annotation && key_annotation_value) {
              implied_keys_ = false;
            }
          }
        } else {
          implied_keys_ = false;
        }
      }

      for (Fields::Iterator i = fields[pos_]; i != fields_end; ++i) {
        bool key_annotation_value;
        const bool has_key_annotation = be_global->check_key(*i, key_annotation_value);
        const bool implied_key = implied_keys_ && !(has_key_annotation && !key_annotation_value);
        if (key_annotation_value || implied_key) {
          child_ = new Iterator(*i, this);
          Iterator& child = *child_;
          if (child == end_value()) {
            delete child_;
            child_ = 0;
            throw Error(*i, std::string("field is ") + (implied_key ? "implicitly" : "explicitly") +
              " marked as key, but does not contain any keys.");
          } else {
            current_value_ = *child;
            pos_ = i.pos();
            return *this;
          }
        }
      }
    } else if (pos_ == 0) { // Else return "this" once
      pos_ = 1;
      current_value_ = root_;
      return *this;
    }

  // If we are an array, use the base type and repeat for every element
  } else if (root_type_ == ArrayType) {
    AST_Array* array_node = dynamic_cast<AST_Array*>(root_);
    if (!array_node) {
      throw Error(root_, "Invalid Key Iterator");
    }
    if (element_count_ == 0) {
      element_count_ = 1;
      ACE_CDR::ULong array_dimension_count = array_node->n_dims();
      for (unsigned i = 0; i < array_dimension_count; i++) {
        ACE_CDR::ULong dimension = array_node->dims()[i]->ev()->u.ulval;
        dimensions_.push_back(dimension);
        element_count_ *= dimension;
      }
    }
    AST_Type* type_node = array_node->base_type();
    AST_Type* unaliased_type_node = type_node->unaliased_type();
    if (pos_ < element_count_) {
      child_ = new Iterator(unaliased_type_node, this);
      Iterator& child = *child_;
      if (child == Iterator()) {
        delete child_;
        child_ = 0;
        throw Error(array_node, "array type is marked as key, but its base type "
          "does not contain any keys.");
      }
      current_value_ = *child;
      return *this;
    }

  // If we are a union, use self if we have a key
  } else if (root_type_ == UnionType) {
    if (pos_ == 0) { // Only Allow One Iteration
      pos_ = 1;
      AST_Union* union_node = dynamic_cast<AST_Union*>(root_);
      if (be_global->has_key(union_node)) {
        current_value_ = root_;
        return *this;
      } else {
        throw Error(union_node, "union type is marked as key, "
          "but its discriminator isn't");
      }
    }

  // If we are a primitive type, use self
  } else if (root_type_ == PrimitiveType) {
    if (pos_ == 0) { // Only Allow One Iteration
      pos_ = 1;
      current_value_ = root_;
      return *this;
    }
  }

  // Nothing left to do, set this to null
  *this = end_value();

  return *this;
}

TopicKeys::Iterator TopicKeys::Iterator::operator++(int)
{
  Iterator prev(*this);
  ++(*this);
  return prev;
}

AST_Decl* TopicKeys::Iterator::operator*() const
{
  return current_value_;
}

bool TopicKeys::Iterator::operator==(const TopicKeys::Iterator& other) const
{
  return
    parent_ == other.parent_ &&
    root_ == other.root_ &&
    root_type_ == other.root_type_ &&
    pos_ == other.pos_ &&
    current_value_ == other.current_value_ &&
    level_ == other.level_ &&
    recursive_ == other.recursive_ &&
    dimensions_ == other.dimensions_ &&
    element_count_ == other.element_count_ &&
    (
      (child_ && other.child_) ? *child_ == *other.child_ : child_ == other.child_
    );
}

bool TopicKeys::Iterator::operator!=(const TopicKeys::Iterator& other) const
{
  return !(*this == other);
}

std::string TopicKeys::Iterator::path()
{
  std::stringstream ss;
  path_i(ss);
  return ss.str();
}

void TopicKeys::Iterator::path_i(std::stringstream& ss)
{
  const char* error_msg = "Can't get path for invalid topic key iterator!";
  if (root_type_ == StructureType) {
    AST_Structure* struct_root = dynamic_cast<AST_Structure*>(root_);
    if (!struct_root) {
      throw Error(root_, error_msg);
    }
    AST_Field* field = *Fields(struct_root)[child_ ? pos_ : pos_ - 1];
    ss << (level_ ? "." : "") << field->local_name()->get_string();
  } else if (root_type_ == UnionType) {
    // Nothing
  } else if (root_type_ == ArrayType) {
    // Figure out what the vector version of the scalar pos_ is
    std::vector<size_t>::reverse_iterator di, dfinished = dimensions_.rend();
    size_t acc = pos_;
    size_t div = 1;
    std::vector<size_t> results;
    for (di = dimensions_.rbegin(); di != dfinished; ++di) {
      acc /= div;
      results.push_back(acc % *di);
      div = *di;
    }

    std::vector<size_t>::reverse_iterator
      ri = results.rbegin(),
      rfinished = results.rend();
    for (; ri != rfinished; ++ri) {
      ss << '[' << *ri << ']';
    }
  } else if (root_type_ != PrimitiveType) {
    throw Error(root_, error_msg);
  }
  if (child_ && recursive_) {
    child_->path_i(ss);
  }
}

void TopicKeys::Iterator::cleanup()
{
  delete child_;
}

TopicKeys::RootType TopicKeys::Iterator::root_type() const
{
  return child_ ? child_->root_type() : root_type_;
}

TopicKeys::RootType TopicKeys::Iterator::parents_root_type() const
{
  return child_ ? child_->parents_root_type() : (parent_ ? parent_->root_type_ : InvalidType);
}

size_t TopicKeys::Iterator::level() const
{
  return child_ ? child_->level() : level_;
}

AST_Type* TopicKeys::Iterator::get_ast_type() const
{
  AST_Field* field = 0;
  switch (root_type()) {
  case UnionType:
    return dynamic_cast<AST_Type*>(current_value_);
  case StructureType:
    if (level_ > 0) {
      if (!recursive_) {
        return dynamic_cast<AST_Type*>(current_value_);
      }
    } else {
      return child_->get_ast_type();
    }
    break;
  default:
    break;
  }
  switch (parents_root_type()) {
  case StructureType:
    field = dynamic_cast<AST_Field*>(current_value_);
    if (field) {
      return field->field_type();
    }
    break;
  case ArrayType:
    return dynamic_cast<AST_Type*>(current_value_);
  default:
    break;
  }
  return 0;
}

TopicKeys::TopicKeys()
  : root_(0),
    root_type_(InvalidType),
    counted_(false),
    count_(0),
    recursive_(false)
{
}

TopicKeys::TopicKeys(const TopicKeys& other)
{
  *this = other;
}

TopicKeys::TopicKeys(AST_Structure* root, bool recursive)
  : root_(root),
    root_type_(StructureType),
    counted_(false),
    count_(0),
    recursive_(recursive)
{
  root_ = root;
}

TopicKeys::TopicKeys(AST_Union* root)
  : root_(root),
    root_type_(UnionType),
    counted_(false),
    count_(0),
    recursive_(false)
{
  root_ = root;
}

TopicKeys::~TopicKeys()
{
}

TopicKeys& TopicKeys::operator=(const TopicKeys& other)
{
  root_ = other.root_;
  root_type_ = other.root_type_;
  counted_ = other.counted_;
  count_ = other.count_;
  recursive_ = other.recursive_;
  return *this;
}

TopicKeys::Iterator TopicKeys::begin()
{
  return Iterator(*this);
}

TopicKeys::Iterator TopicKeys::end()
{
  return Iterator::end_value();
}

AST_Decl* TopicKeys::root() const
{
  return root_;
}

TopicKeys::RootType TopicKeys::root_type() const
{
  return root_type_;
}

size_t TopicKeys::count()
{
  if (!counted_) {
    count_ = 0;
    Iterator finished = end();
    for (Iterator i = begin(); i != finished; ++i) {
      count_++;
    }
    counted_ = true;
  }
  return count_;
}

bool TopicKeys::recursive() const
{
  return recursive_;
}

TopicKeys::Iterator TopicKeys::Iterator::end_value()
{
  static Iterator end;
  return end;
}
