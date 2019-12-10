#include "annotations.h"

#include <ast_annotation_decl.h>
#include <ast_annotation_appl.h>
#include <ast_annotation_member.h>
#include <ast_union.h>

void
BuiltinAnnotations::register_all()
{
  register_one<KeyAnnotation>();
  register_one<TopicAnnotation>();
  register_one<NestedAnnotation>();
  register_one<DefaultNestedAnnotation>();
}

BuiltinAnnotations::BuiltinAnnotations()
{
}

BuiltinAnnotations::~BuiltinAnnotations()
{
  for (MapType::iterator i = map_.begin(); i != map_.end(); ++i) {
    delete i->second;
  }
}

BuiltinAnnotation*
BuiltinAnnotations::operator[](const std::string& annotation)
{
  return map_[annotation];
}

BuiltinAnnotation::BuiltinAnnotation()
: declaration_(0)
{
}

BuiltinAnnotation::~BuiltinAnnotation()
{
}

std::string
BuiltinAnnotation::fullname() const
{
  return std::string("::@") + name();
}

AST_Annotation_Decl*
BuiltinAnnotation::declaration() const
{
  return declaration_;
}

AST_Annotation_Appl*
BuiltinAnnotation::find_on(AST_Decl* node) const
{
  return node->annotations().find(declaration_);
}

void
BuiltinAnnotation::cache()
{
  idl_global->eval(definition().c_str());
  UTL_Scope* root = idl_global->scopes().bottom();
  declaration_ = dynamic_cast<AST_Annotation_Decl*>(
    root->lookup_by_name(fullname().c_str()));
}

BuiltinAnnotationWithBoolValue::BuiltinAnnotationWithBoolValue()
{
}

BuiltinAnnotationWithBoolValue::~BuiltinAnnotationWithBoolValue()
{
}

bool
BuiltinAnnotationWithBoolValue::default_value() const
{
  return false;
}

bool
BuiltinAnnotationWithBoolValue::value_from_appl(AST_Annotation_Appl* appl) const
{
  if (!appl) {
    return default_value();
  }
  AST_Annotation_Member* member =
    dynamic_cast<AST_Annotation_Member*>((*appl)["value"]);
  if (!member || !member->value() || !member->value()->ev()) {
    throw std::string("Error accessing value member");
  }
  return member->value()->ev()->u.bval;
}

bool
BuiltinAnnotationWithBoolValue::node_value(AST_Decl* node) const
{
  return value_from_appl(find_on(node));
}

bool
BuiltinAnnotationWithBoolValue::node_value_exists(AST_Decl* node, bool& value) const
{
  AST_Annotation_Appl* appl = find_on(node);
  value = value_from_appl(appl);
  return appl;
}

// @key ======================================================================

KeyAnnotation::KeyAnnotation()
{
}

KeyAnnotation::~KeyAnnotation()
{
}

std::string
KeyAnnotation::definition() const
{
  return
    "@annotation key {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string
KeyAnnotation::name() const
{
  return "key";
}

bool
KeyAnnotation::union_value(AST_Union* node) const
{
  return value_from_appl(node->disc_annotations().find(declaration()));
}

// @topic ====================================================================

TopicAnnotation::TopicAnnotation()
{
}

TopicAnnotation::~TopicAnnotation()
{
}

std::string
TopicAnnotation::definition() const
{
  return
    "@annotation topic {\n"
    "  string name default \"\";\n"
    "  string platform default \"*\";\n"
    "};\n";
}

std::string
TopicAnnotation::name() const
{
  return "topic";
}

// @nested ===================================================================

NestedAnnotation::NestedAnnotation()
{
}

NestedAnnotation::~NestedAnnotation()
{
}

std::string
NestedAnnotation::definition() const
{
  return
    "@annotation nested {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string
NestedAnnotation::name() const
{
  return "nested";
}

// @default_nested ===========================================================

DefaultNestedAnnotation::DefaultNestedAnnotation()
{
}

DefaultNestedAnnotation::~DefaultNestedAnnotation()
{
}

std::string
DefaultNestedAnnotation::definition() const
{
  return
    "@annotation default_nested {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string
DefaultNestedAnnotation::name() const
{
  return "default_nested";
}
