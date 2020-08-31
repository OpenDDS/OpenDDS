#include "annotations.h"

#include "be_extern.h"

#include <ast_annotation_decl.h>
#include <ast_annotation_appl.h>
#include <ast_annotation_member.h>
#include <ast_union.h>
#include <utl_string.h>

void Annotations::register_all()
{
  register_one<KeyAnnotation>();
  register_one<TopicAnnotation>();
  register_one<NestedAnnotation>();
  register_one<DefaultNestedAnnotation>();
  register_one<IdAnnotation>();
  register_one<AutoidAnnotation>();
  register_one<HashidAnnotation>();
  register_one<OptionalAnnotation>();
  register_one<MustUnderstandAnnotation>();
  register_one<ExternalAnnotation>();
  register_one<ExtensibilityAnnotation>();
  register_one<FinalAnnotation>();
  register_one<AppendableAnnotation>();
  register_one<MutableAnnotation>();
  register_one<TryConstructAnnotation>();
  register_one<OpenDDS::DataRepresentationAnnotation>();
}

Annotations::Annotations()
{
}

Annotations::~Annotations()
{
  for (MapType::iterator i = map_.begin(); i != map_.end(); ++i) {
    delete i->second;
  }
}

Annotation* Annotations::operator[](const std::string& annotation) const
{
  const MapType::const_iterator i = map_.find(annotation);
  if (i == map_.end()) {
    idl_global->err()->misc_error(
      (std::string("No such annotation: ") + annotation).c_str());
    BE_abort();
  }
  return i->second;
}

Annotation::Annotation()
: declaration_(0)
{
}

Annotation::~Annotation()
{
}

std::string Annotation::module() const
{
  return "::";
}

std::string Annotation::fullname() const
{
  return module() + std::string("@") + name();
}

AST_Annotation_Decl* Annotation::declaration() const
{
  return declaration_;
}

AST_Annotation_Appl* Annotation::find_on(AST_Decl* node) const
{
  return node->annotations().find(declaration_);
}

void Annotation::cache()
{
  idl_global->eval(definition().c_str());
  UTL_Scope* root = idl_global->scopes().bottom();
  declaration_ = dynamic_cast<AST_Annotation_Decl*>(
    root->lookup_by_name(fullname().c_str()));
}

AST_Expression::AST_ExprValue* get_annotation_member_ev(
  AST_Annotation_Appl* appl, const char* member_name)
{
  AST_Annotation_Member* member =
    dynamic_cast<AST_Annotation_Member*>((*appl)[member_name]);
  if (!member) {
    return 0;
  }

  AST_Expression* e = member->value();
  if (!e) {
    return 0;
  }

  return e->ev();
}

bool get_bool_annotation_member_value(AST_Annotation_Appl* appl,
                                      const char* member_name,
                                      const bool* default_value)
{
  AST_Expression::AST_ExprValue* ev = get_annotation_member_ev(appl, member_name);
  if (!ev) {
    if (default_value) {
      return *default_value;
    }
    idl_global->err()->misc_error(
      (std::string("Found null pointer while getting value of member \"") +
        member_name + "\" of annotation \"" +
        appl->local_name()->get_string() + "\"").c_str(),
      appl);
    BE_abort();
  }
  return ev->u.bval;
}

ACE_UINT32 get_u32_annotation_member_value(AST_Annotation_Appl* appl,
                                           const char* member_name,
                                           const ACE_UINT32* default_value)
{
  AST_Expression::AST_ExprValue* ev = get_annotation_member_ev(appl, member_name);
  if (!ev) {
    if (default_value) {
      return *default_value;
    }
    idl_global->err()->misc_error(
      (std::string("Found null pointer while getting value of member \"") +
        member_name + "\" of annotation \"" +
        appl->local_name()->get_string() + "\"").c_str(),
      appl);
    BE_abort();
  }
  return ev->u.ulval;
}

std::string get_str_annotation_member_value(AST_Annotation_Appl* appl,
                                            const char* member_name,
                                            const std::string* default_value)
{
  AST_Expression::AST_ExprValue* ev = get_annotation_member_ev(appl, member_name);
  if (!ev) {
    if (default_value) {
      return *default_value;
    }
    idl_global->err()->misc_error(
      (std::string("Found null pointer while getting value of member \"") +
        member_name + "\" of annotation \"" +
        appl->local_name()->get_string() + "\"").c_str(),
      appl);
    BE_abort();
  }
  UTL_String* idlstr = ev->u.strval;
  if (!idlstr) {
    idl_global->err()->misc_error(
      (std::string("Found null pointer while getting string value of member \"") +
        member_name + "\" of annotation \"" +
        appl->local_name()->get_string() + "\"").c_str(),
      appl);
    BE_abort();
  }
  return idlstr->get_string();
}

template<>
bool AnnotationWithValue<bool>::value_from_appl(AST_Annotation_Appl* appl,
                                                const bool* default_value) const
{
  return get_bool_annotation_member_value(appl, "value", default_value);
}

template<>
ACE_UINT32 AnnotationWithValue<ACE_UINT32>::value_from_appl(AST_Annotation_Appl* appl,
                                                            const ACE_UINT32* default_value) const
{
  return get_u32_annotation_member_value(appl, "value", default_value);
}

template<>
std::string AnnotationWithValue<std::string>::value_from_appl(AST_Annotation_Appl* appl,
                                                              const std::string* default_value) const
{
  return get_str_annotation_member_value(appl, "value", default_value);
}

// @key ======================================================================

std::string KeyAnnotation::definition() const
{
  return
    "@annotation key {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string KeyAnnotation::name() const
{
  return "key";
}

bool KeyAnnotation::union_value(AST_Union* node) const
{
  AST_Annotation_Appl* appl = node->disc_annotations().find(declaration());
  if (!appl) { return absent_value; }
  return value_from_appl(appl, default_value());
}

// @topic ====================================================================

TopicAnnotation::TopicAnnotation()
{
}

std::string TopicAnnotation::definition() const
{
  return
    "@annotation topic {\n"
    "  string name default \"\";\n"
    "  string platform default \"*\";\n"
    "};\n";
}

std::string TopicAnnotation::name() const
{
  return "topic";
}

TopicValue TopicAnnotation::value_from_appl(AST_Annotation_Appl* appl, const TopicValue*) const
{
  TopicValue value;
  value.name = get_str_annotation_member_value(appl, "name", &value.name);
  value.platform = get_str_annotation_member_value(appl, "platform", &value.platform);
  return value;
}

// @nested ===================================================================

std::string NestedAnnotation::definition() const
{
  return
    "@annotation nested {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string NestedAnnotation::name() const
{
  return "nested";
}

// @default_nested ===========================================================

std::string DefaultNestedAnnotation::definition() const
{
  return
    "@annotation default_nested {\n"
    "  boolean value default TRUE;\n"
    "};\n";
}

std::string DefaultNestedAnnotation::name() const
{
  return "default_nested";
}

// @id =======================================================================

std::string IdAnnotation::definition() const
{
  return
    "@annotation id {\n"
    "  unsigned long value;\n"
    "};\n";
}

std::string IdAnnotation::name() const
{
  return "id";
}

// @autoid ===================================================================

std::string AutoidAnnotation::definition() const
{
  return
    "@annotation autoid {\n"
    "    enum AutoidKind {\n"
    "      SEQUENTIAL,\n"
    "      HASH\n"
    "    };\n"
    "    AutoidKind value default HASH;\n"
    "};\n";
}

std::string AutoidAnnotation::name() const
{
  return "autoid";
}

// @hashid ===================================================================

std::string HashidAnnotation::definition() const
{
  return
    "@annotation hashid {\n"
    "  string value default \"\";"
    "};\n";
}

std::string HashidAnnotation::name() const
{
  return "hashid";
}

// @optional =================================================================

std::string OptionalAnnotation::definition() const
{
  return
    "@annotation optional {\n"
    "  boolean value default TRUE;"
    "};\n";
}

std::string OptionalAnnotation::name() const
{
  return "optional";
}

// @must_understand =================================================================

std::string MustUnderstandAnnotation::definition() const
{
  return
    "@annotation must_understand {\n"
    "  boolean value default TRUE;"
    "};\n";
}

std::string MustUnderstandAnnotation::name() const
{
  return "must_understand";
}

// @external =================================================================

std::string ExternalAnnotation::definition() const
{
  return
    "@annotation external {\n"
    "  boolean value default TRUE;"
    "};\n";
}

std::string ExternalAnnotation::name() const
{
  return "external";
}

// @extensibility ============================================================

std::string ExtensibilityAnnotation::definition() const
{
  return
    "@annotation extensibility {\n"
    "  enum ExtensibilityKind {\n"
    "    FINAL,\n"
    "    APPENDABLE,\n"
    "    MUTABLE\n"
    "  };\n"
    "  ExtensibilityKind value;\n"
    "};\n";
}

std::string ExtensibilityAnnotation::name() const
{
  return "extensibility";
}

// @final ====================================================================

std::string FinalAnnotation::definition() const
{
  return "@annotation final {};\n";
}

std::string FinalAnnotation::name() const
{
  return "final";
}

// @appendable ===============================================================

std::string AppendableAnnotation::definition() const
{
  return "@annotation appendable {};\n";
}

std::string AppendableAnnotation::name() const
{
  return "appendable";
}

// @mutable ==================================================================

std::string MutableAnnotation::definition() const
{
  return "@annotation mutable {};\n";
}

std::string MutableAnnotation::name() const
{
  return "mutable";
}

// @try_construct ============================================================

std::string TryConstructAnnotation::definition() const
{
  return
    "@annotation try_construct {\n"
    "  enum TryConstructFailAction {\n"
    "    DISCARD,\n"
    "    USE_DEFAULT,\n"
    "    TRIM\n"
    "  };\n"
    "  TryConstructFailAction value default USE_DEFAULT;\n"
    "};\n";
}

std::string TryConstructAnnotation::name() const
{
  return "try_construct";
}

TryConstructFailAction TryConstructAnnotation::union_value(AST_Union* node) const
{
  AST_Annotation_Appl* appl = node->disc_annotations().find(declaration());
  if (!appl) { return absent_value; }
  return value_from_appl(appl, default_value());
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {

  // @OpenDDS::data_representation ===========================================

  std::string DataRepresentationAnnotation::definition() const
  {
    return
      "module OpenDDS {\n"
      "  @annotation data_representation {\n"
      "    enum Kind_t {\n"
      "      XCDR1,\n"
      "      XML,\n"
      "      XCDR2,\n"
      "      UNALIGNED_CDR\n"
      "    };\n"
      "    Kind_t kind;\n"
      "  };\n"
      "};\n";
  }

  std::string DataRepresentationAnnotation::name() const
  {
    return "data_representation";
  }

  bool DataRepresentationAnnotation::node_value_exists(
    AST_Decl* node, DataRepresentation& value) const
  {
    value = DataRepresentation();
    bool found = false;
    if (node) {
      for (AST_Annotation_Appls::iterator i = node->annotations().begin();
          i != node->annotations().end(); ++i) {
        AST_Annotation_Appl* appl = i->get();
        if (appl && appl->annotation_decl() == declaration()) {
          found = true;
          value.add(value_from_appl(appl, 0));
        }
      }
    }
    return found;
  }

  DataRepresentation DataRepresentationAnnotation::value_from_appl(AST_Annotation_Appl* appl,
                                                                   const DataRepresentation* default_value) const
  {
    DataRepresentation value;
    if (appl && appl->annotation_decl() == declaration()) {
      switch (get_u32_annotation_member_value(appl, "kind", reinterpret_cast<const ACE_UINT32*>(default_value))) {
      case 0: // Kind_t::XCDR1
        value.xcdr1 = true;
        break;
      case 1: // Kind_t::XML
        value.xml = true;
        break;
      case 2: // Kind_t::XCDR2
        value.xcdr2 = true;
        break;
      case 3: // Kind_t::UNALIGNED_CDR
        value.unaligned_cdr = true;
        break;
      }
    }
    return value;
  }

  std::string DataRepresentationAnnotation::module() const
  {
    return "::OpenDDS::";
  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
