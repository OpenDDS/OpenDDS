#include "idl_defines.h"
#ifdef TAO_IDL_HAS_ANNOTATIONS

#include <string>
#include <sstream>
#include <list>

#include "be_extern.h"
#include "ast_structure.h"
#include "ast_field.h"
#include "utl_identifier.h"
#include "ast_union.h"

#include "sample_keys.h"

SampleKeys::Error::Error()
{
}

SampleKeys::Error::Error(const SampleKeys::Error& error)
  : message_(error.message_)
{
}

SampleKeys::Error::Error(const std::string& message)
  : message_(message)
{
}

SampleKeys::Error::Error(AST_Decl* node, const std::string& message)
{
  std::stringstream ss;
  if (node) {
    ss << "On line " << node->line() << " in " << node->file_name() << ":";
  }
  ss << message;
  message_ = ss.str();
}

SampleKeys::Error& SampleKeys::Error::operator=(const SampleKeys::Error& error)
{
  message_ = error.message_;
  return *this;
}

const char* SampleKeys::Error::what() const noexcept
{
  return message_.c_str();
}

SampleKeys::Iterator::Iterator()
  : pos_(0),
    child_(0),
    current_value_(0),
    root_sample_(0)
{
}

SampleKeys::Iterator::Iterator(SampleKeys& parent)
  : pos_(0),
    child_(0),
    current_value_(0)
{
  root_sample_ = parent.root_sample();
  (*this)++;
}

SampleKeys::Iterator::Iterator(AST_Structure* root_sample)
  : pos_(0),
    child_(0),
    current_value_(0),
    root_sample_(root_sample)
{
  (*this)++;
}

SampleKeys::Iterator::Iterator(const SampleKeys::Iterator& other)
  : pos_(0),
    child_(0),
    current_value_(0),
    root_sample_(0)
{
  *this = other;
}

SampleKeys::Iterator::~Iterator()
{
  cleanup();
}

SampleKeys::Iterator& SampleKeys::Iterator::operator=(const SampleKeys::Iterator& other)
{
  cleanup();
  pos_ = other.pos_;
  current_value_ = other.current_value_;
  root_sample_ = other.root_sample_;
  child_ = other.child_ ? new Iterator(*other.child_) : 0;
  return *this;
}

SampleKeys::Iterator& SampleKeys::Iterator::operator++()
{
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

  // Go on to next field
  if (root_sample_) {
    size_t field_count = root_sample_->nfields();
    for (; pos_ < field_count; ++pos_) {
      AST_Field** field_ptrptr;
      root_sample_->field(field_ptrptr, pos_);
      AST_Field* field = *field_ptrptr;
      if (be_global->is_key(field)) {
        bool valid_key = false;
        AST_Type* field_type = field->field_type()->unaliased_type();
        // If the field is a struct, transverse that
        if (field_type->node_type() == AST_Decl::NT_struct) {
          AST_Structure* struct_node = dynamic_cast<AST_Structure*>(field_type);
          child_ = new Iterator(struct_node);
          Iterator& child = *child_;
          if (child == Iterator()) {
            delete child_;
            child_ = 0;
            throw Error(field, "struct type field is marked as key, but "
              "none if its fields are marked as a key.");
          } else {
            current_value_ = *child;
            return *this;
          }
        // If it is a Union, check if the discriminator is a key
        } else if (field_type->node_type() == AST_Decl::NT_union) {
          if (be_global->has_key(dynamic_cast<AST_Union*>(field_type))) {
            valid_key = true;
          } else {
            throw Error(field, "union type field is marked as key, but the "
              "union discriminator isn't marked as a key.");
          }
        // Else we found a valid key
        } else {
          valid_key = true;
        }

        // We have reached a valid key
        if (valid_key) {
          current_value_ = dynamic_cast<AST_Decl*>(field);
          pos_++;
          return *this;
        }
      }
    }
  }

  // Nothing left to do, set this to null
  *this = Iterator();

  return *this;
}

SampleKeys::Iterator SampleKeys::Iterator::operator++(int)
{
  Iterator prev(*this);
  ++(*this);
  return prev;
}

SampleKeys::Iterator::value_type SampleKeys::Iterator::operator*() const
{
  return current_value_;
}

bool SampleKeys::Iterator::operator==(const SampleKeys::Iterator& other) const
{
  return
    root_sample_ == other.root_sample_ &&
    pos_ == other.pos_ &&
    current_value_ == other.current_value_ &&
    (
      (child_ && other.child_) ? *child_ == *other.child_ : child_ == other.child_
    );
}

bool SampleKeys::Iterator::operator!=(const SampleKeys::Iterator& other) const
{
  return !(*this == other);
}

std::string SampleKeys::Iterator::path()
{
  std::list<std::string> name_stack;
  path_i(name_stack);
  std::list<std::string>::iterator
    i = name_stack.begin(),
    finished = name_stack.end();
  std::stringstream ss;
  if (i != finished) {
    ss << *i;
    ++i;
  }
  for (; i != finished; ++i) {
    ss << "." << *i;
  }
  return ss.str();
}

void SampleKeys::Iterator::path_i(std::list<std::string>& name_stack)
{
  AST_Field** field_ptrptr;
  root_sample_->field(field_ptrptr, pos_-1);
  AST_Field* field = *field_ptrptr;
  name_stack.push_back(field->local_name()->get_string());
  if (child_) {
    child_->path_i(name_stack);
  }
}

void SampleKeys::Iterator::cleanup()
{
  delete child_;
}

SampleKeys::SampleKeys(AST_Structure* root_sample)
  : root_sample_ (root_sample),
    counted_ (false)
{
  root_sample_ = root_sample;
}

SampleKeys::~SampleKeys()
{
}

SampleKeys::Iterator SampleKeys::begin()
{
  return Iterator(*this);
}

SampleKeys::Iterator SampleKeys::end()
{
  return Iterator();
}

AST_Structure* SampleKeys::root_sample() const
{
  return root_sample_;
}

size_t SampleKeys::count()
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
#endif
