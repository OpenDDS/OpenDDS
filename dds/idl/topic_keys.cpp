#include "idl_defines.h"

#include <string>
#include <list>

#include "be_extern.h"
#include "ast_structure.h"
#include "ast_field.h"
#include "utl_identifier.h"
#include "ast_union.h"
#include "ast_array.h"

#include "topic_keys.h"

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
  : node_(node), message_(message)
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
  : parent_(0),
    pos_(0),
    child_(0),
    current_value_(0),
    root_(0),
    root_type_(InvalidType),
    level_(0),
    recursive_(false)
{
}

TopicKeys::Iterator::Iterator(TopicKeys& parent)
  : parent_(0),
    pos_(0),
    child_(0),
    current_value_(0),
    level_(0),
    recursive_(parent.recursive())
{
  root_ = parent.root();
  root_type_ = parent.root_type();
  ++(*this);
}

TopicKeys::Iterator::Iterator(AST_Type* root, Iterator* parent)
  : parent_(parent),
    pos_(0),
    child_(0),
    current_value_(0),
    level_(parent->level() + 1),
    recursive_(parent->recursive_)
{
  root_type_ = TopicKeys::root_type(root);
  root_ = root;
  ++(*this);
}

TopicKeys::Iterator::Iterator(AST_Field* root, Iterator* parent)
  : parent_(parent),
    pos_(0),
    child_(0),
    current_value_(0),
    level_(parent->level() + 1),
    recursive_(parent->recursive_)
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

TopicKeys::Iterator::Iterator(const TopicKeys::Iterator& other)
  : pos_(0),
    child_(0),
    current_value_(0),
    root_(0),
    root_type_(InvalidType),
    level_(0),
    recursive_(false)
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
  child_ = other.child_ ? new Iterator(*other.child_) : 0;
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
    if (child == Iterator()) {
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
      ACE_CDR::ULong field_count = struct_root->nfields();
      for (; pos_ < field_count; ++pos_) {
        AST_Field** field_ptrptr;
        struct_root->field(field_ptrptr, pos_);
        AST_Field* field = *field_ptrptr;
        if (be_global->is_key(field)) {
          child_ = new Iterator(field, this);
          Iterator& child = *child_;
          if (child == Iterator()) {
            delete child_;
            child_ = 0;
            throw Error(field, "field is marked as key, but does not contain any keys.");
          } else {
            current_value_ = *child;
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
    ACE_CDR::ULong array_dimension_count = array_node->n_dims();
    if (array_dimension_count > 1) {
      throw Error(root_, "using multidimensional arrays as keys is unsupported.");
    }
    unsigned element_count = array_node->dims()[0]->ev()->u.ulval;
    AST_Type* type_node = array_node->base_type();
    AST_Type* unaliased_type_node = type_node->unaliased_type();
    for (; pos_ < element_count; ++pos_) {
      child_ = new Iterator(unaliased_type_node, this);
      Iterator& child = *child_;
      if (child == Iterator()) {
        delete child_;
        child_ = 0;
        throw Error(array_node, "array type is marked as key, but its base type "
          "does not contain any keys.");
      } else {
        current_value_ = *child;
        return *this;
      }
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
          "but it's discriminator isn't");
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
  *this = Iterator();

  return *this;
}

TopicKeys::Iterator TopicKeys::Iterator::operator++(int)
{
  Iterator prev(*this);
  ++(*this);
  return prev;
}

TopicKeys::Iterator::value_type TopicKeys::Iterator::operator*() const
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
  if (root_type_ == StructureType) {
    AST_Structure* struct_root = dynamic_cast<AST_Structure*>(root_);
    AST_Field** field_ptrptr;
    struct_root->field(field_ptrptr, child_ ? pos_ : pos_ - 1);
    AST_Field* field = *field_ptrptr;
    ss << (level_ ? "." : "") << field->local_name()->get_string();
  } else if (root_type_ == UnionType) {
    // Nothing
  } else if (root_type_ == ArrayType) {
    ss << '[' << pos_ << ']';
  } else if (root_type_ != PrimitiveType) {
    throw Error(root_, "Can't get path for invalid topic key iterator!");
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
    return dynamic_cast<AST_Field*>(current_value_)->field_type();
  case ArrayType:
    return dynamic_cast<AST_Type*>(current_value_);
  default:
    return 0;
  }
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
  return Iterator();
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
