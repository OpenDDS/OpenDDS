/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "XmlTypeProvider.h"

#if defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

#include "DynamicTypeImpl.h"
#include "TypeLookupService.h"
#include "TypeObject.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/debug.h>
#include <dds/DCPS/unique_ptr.h>

#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_strings.h>
#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

#include <cerrno>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {
namespace {

std::string to_string(const XMLCh* in)
{
  if (!in) {
    return std::string();
  }
  char* raw = XERCES_CPP_NAMESPACE::XMLString::transcode(in);
  const std::string result = raw ? raw : "";
  XERCES_CPP_NAMESPACE::XMLString::release(&raw);
  return result;
}

template <typename T>
std::string xml_type_provider_value_to_string(const T& value)
{
  std::ostringstream out;
  out << value;
  return out.str();
}

class XmlString {
public:
  explicit XmlString(const char* in)
    : value_(XERCES_CPP_NAMESPACE::XMLString::transcode(in))
  {}

  ~XmlString()
  {
    XERCES_CPP_NAMESPACE::XMLString::release(&value_);
  }

  operator const XMLCh*() const
  {
    return value_;
  }

private:
  XMLCh* value_;
};

std::string local_name(const XERCES_CPP_NAMESPACE::DOMNode* node)
{
  const XMLCh* name = node->getLocalName();
  if (!name) {
    name = node->getNodeName();
  }
  return to_string(name);
}

bool has_attr(const XERCES_CPP_NAMESPACE::DOMElement* element, const char* name)
{
  return element->hasAttribute(XmlString(name));
}

std::string attr(const XERCES_CPP_NAMESPACE::DOMElement* element, const char* name)
{
  return to_string(element->getAttribute(XmlString(name)));
}

std::string required_attr(const XERCES_CPP_NAMESPACE::DOMElement* element, const char* name,
                          const std::string& element_name, std::string& error)
{
  if (!has_attr(element, name)) {
    error = "missing required attribute '" + std::string(name) + "' on <" + element_name + ">";
    return std::string();
  }
  return attr(element, name);
}

std::string strip_leading_colons(const std::string& name)
{
  if (name.size() > 1 && name[0] == ':' && name[1] == ':') {
    return name.substr(2);
  }
  return name;
}

std::string join_name(const std::string& scope, const std::string& name)
{
  return scope.empty() ? name : scope + "::" + name;
}

std::string parent_scope(const std::string& scope)
{
  const std::string::size_type pos = scope.rfind("::");
  if (pos == std::string::npos) {
    return std::string();
  }
  return scope.substr(0, pos);
}

bool parse_bool(const std::string& value, bool& out)
{
  if (value == "true" || value == "1" || value == "TRUE") {
    out = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "FALSE") {
    out = false;
    return true;
  }
  return false;
}

bool parse_u32(const std::string& value, ACE_CDR::ULong& out)
{
  if (value.empty() || value[0] == '-') {
    return false;
  }
  errno = 0;
  char* end = 0;
  const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
  if (errno || !end || *end || parsed > std::numeric_limits<ACE_CDR::ULong>::max()) {
    return false;
  }
  out = static_cast<ACE_CDR::ULong>(parsed);
  return true;
}

bool parse_i32(const std::string& value, ACE_CDR::Long& out)
{
  if (value.empty()) {
    return false;
  }
  errno = 0;
  char* end = 0;
  const long parsed = std::strtol(value.c_str(), &end, 0);
  if (errno || !end || *end ||
      parsed < std::numeric_limits<ACE_CDR::Long>::min() ||
      parsed > std::numeric_limits<ACE_CDR::Long>::max()) {
    return false;
  }
  out = static_cast<ACE_CDR::Long>(parsed);
  return true;
}

bool parse_bound(const std::string& value, LBound& out)
{
  if (value == "-1") {
    out = 0;
    return true;
  }
  ACE_CDR::ULong parsed = 0;
  if (!parse_u32(value, parsed)) {
    return false;
  }
  out = parsed;
  return true;
}

bool parse_dimensions(const std::string& value, OPENDDS_VECTOR(LBound)& out)
{
  std::string part;
  std::istringstream is(value);
  while (std::getline(is, part, ',')) {
    while (!part.empty() && (part[0] == ' ' || part[0] == '\t')) {
      part.erase(part.begin());
    }
    while (!part.empty() && (part[part.size() - 1] == ' ' || part[part.size() - 1] == '\t')) {
      part.erase(part.end() - 1);
    }
    LBound bound = 0;
    if (!parse_bound(part, bound) || bound == 0) {
      return false;
    }
    out.push_back(bound);
  }
  return !out.empty();
}

class XercesInitializer {
public:
  XercesInitializer()
    : initialized_(false)
    , failed_(false)
  {}

  ~XercesInitializer()
  {
    if (initialized_) {
      XERCES_CPP_NAMESPACE::XMLPlatformUtils::Terminate();
    }
  }

  bool ensure(std::string& error)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (initialized_) {
      return true;
    }
    if (failed_) {
      error = "Xerces-C initialization previously failed";
      return false;
    }

    try {
      XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
      initialized_ = true;
      return true;
    } catch (const XERCES_CPP_NAMESPACE::XMLException& ex) {
      failed_ = true;
      error = "Xerces-C initialization failed: " + to_string(ex.getMessage());
      return false;
    }
  }

private:
  ACE_Thread_Mutex mutex_;
  bool initialized_;
  bool failed_;
};

bool ensure_xerces(std::string& error)
{
  static XercesInitializer initializer;
  return initializer.ensure(error);
}

// The TypeLookupService below must be process-wide (not per-call) because
// type_identifier_to_dynamic caches DynamicType objects in its gt_map_ by
// TypeIdentifier, and those cached objects are the reference-counting backbone
// of the returned DynamicType tree.  If the TLS were destroyed before all
// DynamicType consumers were done, cascading _var destructions inside gt_map_
// teardown can free member types that are still reachable from parent types.
TypeLookupService& xml_type_lookup_service()
{
  static TypeLookupService service;
  return service;
}

ACE_Thread_Mutex& xml_type_lookup_service_mutex()
{
  static ACE_Thread_Mutex mutex;
  return mutex;
}

enum TypeModelKind {
  TYPE_STRUCT,
  TYPE_UNION,
  TYPE_ENUM,
  TYPE_BITMASK
};

enum TypeSpecKind {
  SPEC_PRIMITIVE,
  SPEC_STRING,
  SPEC_NAMED,
  SPEC_MAP
};

struct TypeSpec {
  TypeSpecKind kind;
  TypeKind primitive_kind;
  bool wide_string;
  LBound string_bound;
  std::string named_type;
  std::string scope;
  bool has_sequence;
  LBound sequence_bound;
  bool has_array;
  OPENDDS_VECTOR(LBound) array_dimensions;
  DCPS::unique_ptr<TypeSpec> map_key;
  DCPS::unique_ptr<TypeSpec> map_value;
  LBound map_bound;

  TypeSpec()
    : kind(SPEC_PRIMITIVE)
    , primitive_kind(TK_NONE)
    , wide_string(false)
    , string_bound(0)
    , has_sequence(false)
    , sequence_bound(0)
    , has_array(false)
    , map_bound(0)
  {}

  TypeSpec(const TypeSpec& other)
    : kind(other.kind)
    , primitive_kind(other.primitive_kind)
    , wide_string(other.wide_string)
    , string_bound(other.string_bound)
    , named_type(other.named_type)
    , scope(other.scope)
    , has_sequence(other.has_sequence)
    , sequence_bound(other.sequence_bound)
    , has_array(other.has_array)
    , array_dimensions(other.array_dimensions)
    , map_key(other.map_key ? new TypeSpec(*other.map_key) : 0)
    , map_value(other.map_value ? new TypeSpec(*other.map_value) : 0)
    , map_bound(other.map_bound)
  {}

  TypeSpec& operator=(const TypeSpec& other)
  {
    if (this != &other) {
      TypeSpec copy(other);
      swap(copy);
    }
    return *this;
  }

  void swap(TypeSpec& other)
  {
    using std::swap;
    swap(kind, other.kind);
    swap(primitive_kind, other.primitive_kind);
    swap(wide_string, other.wide_string);
    swap(string_bound, other.string_bound);
    named_type.swap(other.named_type);
    scope.swap(other.scope);
    swap(has_sequence, other.has_sequence);
    swap(sequence_bound, other.sequence_bound);
    swap(has_array, other.has_array);
    array_dimensions.swap(other.array_dimensions);
    map_key.swap(other.map_key);
    map_value.swap(other.map_value);
    swap(map_bound, other.map_bound);
  }
};

struct MemberModel {
  std::string name;
  TypeSpec type;
  bool has_id;
  MemberId id;
  bool key;
  bool must_understand;
  bool optional;
  bool external;
  std::string try_construct;
  bool has_hashid;
  std::string hashid;

  MemberModel()
    : has_id(false)
    , id(0)
    , key(false)
    , must_understand(false)
    , optional(false)
    , external(false)
    , has_hashid(false)
  {}
};

struct StructModel {
  OPENDDS_VECTOR(MemberModel) members;
  std::string extensibility;
  std::string autoid;
  bool nested;

  StructModel()
    : nested(false)
  {}
};

struct UnionCaseModel {
  OPENDDS_VECTOR(std::string) labels;
  bool default_label;
  MemberModel member;

  UnionCaseModel()
    : default_label(false)
  {}
};

struct UnionModel {
  TypeSpec discriminator;
  bool discriminator_key;
  std::string discriminator_try_construct;
  OPENDDS_VECTOR(UnionCaseModel) cases;
  std::string extensibility;
  std::string autoid;
  bool nested;

  UnionModel()
    : discriminator_key(false)
    , nested(false)
  {}
};

struct EnumLiteralModel {
  std::string name;
  ACE_CDR::Long value;
  bool default_literal;

  EnumLiteralModel()
    : value(0)
    , default_literal(false)
  {}
};

struct EnumModel {
  BitBound bit_bound;
  std::string extensibility;
  OPENDDS_VECTOR(EnumLiteralModel) literals;
  OPENDDS_MAP(OPENDDS_STRING, ACE_CDR::Long) literal_values;

  EnumModel()
    : bit_bound(32)
  {}
};

struct BitmaskFlagModel {
  std::string name;
  ACE_CDR::UShort position;

  BitmaskFlagModel()
    : position(0)
  {}
};

struct BitmaskModel {
  BitBound bit_bound;
  std::string extensibility;
  OPENDDS_VECTOR(BitmaskFlagModel) flags;
  OPENDDS_MAP(OPENDDS_STRING, ACE_CDR::Long) label_values;

  BitmaskModel()
    : bit_bound(32)
  {}
};

struct TypeModel {
  TypeModelKind kind;
  std::string fq_name;
  StructModel struct_model;
  UnionModel union_model;
  EnumModel enum_model;
  BitmaskModel bitmask_model;

  TypeModel()
    : kind(TYPE_STRUCT)
  {}
};

class XmlTypeLoader {
public:
  explicit XmlTypeLoader(const ACE_TString& file)
    : file_(ACE_TEXT_ALWAYS_CHAR(file.c_str()))
  {}

  DDS::ReturnCode_t load(DDS::DynamicType_var& type, const std::string& type_name)
  {
    type = DDS::DynamicType::_nil();

    std::string error;
    if (!parse(error)) {
      report(error, type_name);
      return DDS::RETCODE_ERROR;
    }

    const std::string requested = strip_leading_colons(type_name);
    if (types_.find(requested) == types_.end()) {
      report("type '" + requested + "' was not found", requested);
      return DDS::RETCODE_BAD_PARAMETER;
    }
    TypeIdentifier root_ti;
    if (!build_type(requested, root_ti, error)) {
      report(error, requested);
      return DDS::RETCODE_ERROR;
    }

    TypeMap::const_iterator root = complete_type_map_.find(root_ti);
    if (root == complete_type_map_.end()) {
      report("internal error: requested type was not added to the type map", requested);
      return DDS::RETCODE_ERROR;
    }

    TypeIdentifier root_minimal_ti;
    if (!build_minimal_type_map(root_ti, root_minimal_ti, error)) {
      report(error, requested);
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var loaded;
    {
      ACE_Guard<ACE_Thread_Mutex> guard(xml_type_lookup_service_mutex());
      TypeLookupService& tls = xml_type_lookup_service();
      tls.add(minimal_type_map_.begin(), minimal_type_map_.end());
      tls.add(complete_type_map_.begin(), complete_type_map_.end());
      loaded = tls.complete_to_dynamic(root->second.complete, DCPS::GUID_t());
    }
    if (!loaded) {
      report("CompleteTypeObject to DynamicType conversion failed", requested);
      return DDS::RETCODE_ERROR;
    }

    DynamicTypeImpl* impl = dynamic_cast<DynamicTypeImpl*>(loaded.in());
    if (impl) {
      impl->set_minimal_type_identifier(root_minimal_ti);
      impl->set_minimal_type_map(minimal_type_map_);
      impl->set_complete_type_identifier(root_ti);
      impl->set_complete_type_map(complete_type_map_);
    }

    type = loaded._retn();
    return DDS::RETCODE_OK;
  }

private:
  typedef OPENDDS_MAP(OPENDDS_STRING, TypeModel) TypeModelMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, TypeIdentifier) TypeIdentifierByName;
  typedef OPENDDS_SET(OPENDDS_STRING) NameSet;

  // An EK_COMPLETE TypeIdentifier is ready for minimal conversion when it has
  // already been erased from 'pending' (i.e. successfully converted in a
  // previous iteration).  Non-EK_COMPLETE identifiers (primitives, strings,
  // plain sequences/arrays) are always ready because the converter handles
  // them without needing an entry in the pending set.
  static bool complete_ti_ready(const TypeIdentifier& ti,
                                const OPENDDS_SET(TypeIdentifier)& pending)
  {
    return ti.kind() != EK_COMPLETE || !pending.count(ti);
  }

  // Returns true if every EK_COMPLETE TypeIdentifier directly referenced by
  // 'to' has already been converted (i.e. is absent from 'pending').  Used
  // to avoid calling complete_to_minimal_type_object when dependencies are
  // not yet ready, which would log a spurious error from
  // get_minimal_type_identifier.
  static bool minimal_deps_ready(const TypeObject& to,
                                 const OPENDDS_SET(TypeIdentifier)& pending)
  {
    const CompleteTypeObject& cto = to.complete;
    switch (cto.kind) {
    case TK_STRUCTURE:
      if (!complete_ti_ready(cto.struct_type.header.base_type, pending)) return false;
      for (ACE_CDR::ULong i = 0; i != cto.struct_type.member_seq.length(); ++i) {
        if (!complete_ti_ready(cto.struct_type.member_seq[i].common.member_type_id, pending)) return false;
      }
      return true;
    case TK_UNION:
      if (!complete_ti_ready(cto.union_type.discriminator.common.type_id, pending)) return false;
      for (ACE_CDR::ULong i = 0; i != cto.union_type.member_seq.length(); ++i) {
        if (!complete_ti_ready(cto.union_type.member_seq[i].common.type_id, pending)) return false;
      }
      return true;
    case TK_ALIAS:
      return complete_ti_ready(cto.alias_type.body.common.related_type, pending);
    case TK_SEQUENCE:
      return complete_ti_ready(cto.sequence_type.element.common.type, pending);
    case TK_ARRAY:
      return complete_ti_ready(cto.array_type.element.common.type, pending);
    case TK_MAP:
      return complete_ti_ready(cto.map_type.key.common.type, pending) &&
             complete_ti_ready(cto.map_type.element.common.type, pending);
    default:  // enum, bitmask, annotation: no EK_COMPLETE dependencies
      return true;
    }
  }

  bool build_minimal_type_map(const TypeIdentifier& root_ti, TypeIdentifier& root_minimal_ti, std::string& error)
  {
    TypeLookupService converter;
    converter.add(complete_type_map_.begin(), complete_type_map_.end());

    OPENDDS_SET(TypeIdentifier) pending;
    for (TypeMap::const_iterator pos = complete_type_map_.begin(); pos != complete_type_map_.end(); ++pos) {
      pending.insert(pos->first);
    }

    while (!pending.empty()) {
      bool progress = false;

      for (OPENDDS_SET(TypeIdentifier)::iterator pos = pending.begin(); pos != pending.end(); ) {
        const TypeMap::const_iterator complete = complete_type_map_.find(*pos);
        if (complete == complete_type_map_.end()) {
          error = "internal error: complete TypeObject missing while building minimal TypeObjects";
          return false;
        }

        if (!minimal_deps_ready(complete->second, pending)) {
          ++pos;
          continue;
        }

        TypeObject minimal;
        if (!converter.complete_to_minimal_type_object(complete->second, minimal)) {
          ++pos;
          continue;
        }

        const TypeIdentifier minimal_ti = makeTypeIdentifier(minimal);
        minimal_type_map_[minimal_ti] = minimal;

        TypeIdentifierPairSeq pair;
        pair.length(1);
        pair[0] = TypeIdentifierPair(complete->first, minimal_ti);
        converter.update_type_identifier_map(pair);

        if (complete->first == root_ti) {
          root_minimal_ti = minimal_ti;
        }

        pending.erase(pos++);
        progress = true;
      }

      if (!progress) {
        for (TypeIdentifierByName::const_iterator it = complete_ti_by_name_.begin();
             it != complete_ti_by_name_.end(); ++it) {
          if (pending.count(it->second)) {
            error = "failed to build minimal TypeObject for '" + std::string(it->first.c_str()) + "'";
            return false;
          }
        }
        error = "failed to build minimal TypeObjects for one or more XML type dependencies";
        return false;
      }
    }

    return true;
  }

  bool parse(std::string& error)
  {
    if (!ensure_xerces(error)) {
      return false;
    }

    XERCES_CPP_NAMESPACE::XercesDOMParser parser;
    parser.setDoNamespaces(true);
    parser.setValidationScheme(XERCES_CPP_NAMESPACE::XercesDOMParser::Val_Never);
    parser.setDoSchema(false);
    parser.setLoadExternalDTD(false);

    try {
      parser.parse(file_.c_str());
    } catch (const XERCES_CPP_NAMESPACE::SAXParseException& ex) {
      error = "XML parse error at line " + xml_type_provider_value_to_string(ex.getLineNumber()) +
        ", column " + xml_type_provider_value_to_string(ex.getColumnNumber()) + ": " + to_string(ex.getMessage());
      return false;
    } catch (const XERCES_CPP_NAMESPACE::SAXException& ex) {
      error = "XML parse error: " + to_string(ex.getMessage());
      return false;
    } catch (const XERCES_CPP_NAMESPACE::XMLException& ex) {
      error = "XML parser error: " + to_string(ex.getMessage());
      return false;
    }

    if (parser.getErrorCount()) {
      error = "XML parser reported " + xml_type_provider_value_to_string(parser.getErrorCount()) + " error(s)";
      return false;
    }

    XERCES_CPP_NAMESPACE::DOMDocument* doc = parser.getDocument();
    if (!doc || !doc->getDocumentElement()) {
      error = "XML document is empty";
      return false;
    }

    XERCES_CPP_NAMESPACE::DOMElement* root = doc->getDocumentElement();
    const std::string root_name = local_name(root);
    if (root_name == "dds") {
      for (XERCES_CPP_NAMESPACE::DOMNode* child = root->getFirstChild(); child; child = child->getNextSibling()) {
        if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
          continue;
        }
        XERCES_CPP_NAMESPACE::DOMElement* element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
        const std::string name = local_name(element);
        if (name == "types") {
          if (!parse_scope(element, std::string(), error)) {
            return false;
          }
        } else {
          error = "unsupported <dds> child <" + name + ">";
          return false;
        }
      }
    } else if (root_name == "types") {
      if (!parse_scope(root, std::string(), error)) {
        return false;
      }
    } else {
      error = "expected <dds> or <types> document root, found <" + root_name + ">";
      return false;
    }

    if (types_.empty()) {
      error = "XML document did not contain any type definitions";
      return false;
    }
    return true;
  }

  bool parse_scope(XERCES_CPP_NAMESPACE::DOMElement* scope_element,
                   const std::string& scope,
                   std::string& error)
  {
    for (XERCES_CPP_NAMESPACE::DOMNode* child = scope_element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string name = local_name(element);
      if (name == "module") {
        const std::string module_name = required_attr(element, "name", name, error);
        if (!error.empty()) {
          return false;
        }
        if (module_name.empty()) {
          error = "module name must not be empty";
          return false;
        }
        if (!parse_scope(element, join_name(scope, module_name), error)) {
          return false;
        }
      } else if (name == "struct") {
        if (!parse_struct(element, scope, error)) {
          return false;
        }
      } else if (name == "union") {
        if (!parse_union(element, scope, error)) {
          return false;
        }
      } else if (name == "enum") {
        if (!parse_enum(element, scope, error)) {
          return false;
        }
      } else if (name == "bitmask") {
        if (!parse_bitmask(element, scope, error)) {
          return false;
        }
      } else {
        error = "unsupported type element <" + name + "> in scope '" + scope + "'";
        return false;
      }
    }
    return true;
  }

  bool add_type(const TypeModel& model, std::string& error)
  {
    if (model.fq_name.empty()) {
      error = "type name must not be empty";
      return false;
    }
    if (types_.find(model.fq_name) != types_.end()) {
      error = "duplicate type definition '" + model.fq_name + "'";
      return false;
    }
    types_[model.fq_name.c_str()] = model;
    return true;
  }

  bool parse_struct(XERCES_CPP_NAMESPACE::DOMElement* element,
                    const std::string& scope,
                    std::string& error)
  {
    TypeModel model;
    model.kind = TYPE_STRUCT;
    model.fq_name = join_name(scope, required_attr(element, "name", "struct", error));
    if (!error.empty()) {
      return false;
    }
    model.struct_model.extensibility = has_attr(element, "extensibility") ? attr(element, "extensibility") : "";
    model.struct_model.autoid = has_attr(element, "autoid") ? attr(element, "autoid") : "";
    if (has_attr(element, "nested") &&
        !parse_bool(attr(element, "nested"), model.struct_model.nested)) {
      error = "invalid nested value on struct '" + model.fq_name + "'";
      return false;
    }

    for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* member_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string child_name = local_name(member_element);
      if (child_name != "member") {
        error = "unsupported <struct> child <" + child_name + "> in type '" + model.fq_name + "'";
        return false;
      }
      MemberModel member;
      if (!parse_member(member_element, scope, "member", member, error)) {
        return false;
      }
      model.struct_model.members.push_back(member);
    }

    return add_type(model, error);
  }

  bool parse_union(XERCES_CPP_NAMESPACE::DOMElement* element,
                   const std::string& scope,
                   std::string& error)
  {
    TypeModel model;
    model.kind = TYPE_UNION;
    model.fq_name = join_name(scope, required_attr(element, "name", "union", error));
    if (!error.empty()) {
      return false;
    }
    model.union_model.extensibility = has_attr(element, "extensibility") ? attr(element, "extensibility") : "";
    model.union_model.autoid = has_attr(element, "autoid") ? attr(element, "autoid") : "";
    if (has_attr(element, "nested") &&
        !parse_bool(attr(element, "nested"), model.union_model.nested)) {
      error = "invalid nested value on union '" + model.fq_name + "'";
      return false;
    }

    bool have_discriminator = false;
    for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* child_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string child_name = local_name(child_element);
      if (child_name == "discriminator") {
        if (have_discriminator) {
          error = "union '" + model.fq_name + "' has multiple discriminators";
          return false;
        }
        if (!parse_type_spec(child_element, scope, model.union_model.discriminator, error)) {
          return false;
        }
        if (has_attr(child_element, "key")) {
          if (!parse_bool(attr(child_element, "key"), model.union_model.discriminator_key)) {
            error = "invalid boolean value for discriminator key in union '" + model.fq_name + "'";
            return false;
          }
        }
        model.union_model.discriminator_try_construct =
          has_attr(child_element, "tryConstruct") ? attr(child_element, "tryConstruct") : "";
        have_discriminator = true;
      } else if (child_name == "case") {
        UnionCaseModel case_model;
        if (!parse_union_case(child_element, scope, model.fq_name, case_model, error)) {
          return false;
        }
        model.union_model.cases.push_back(case_model);
      } else {
        error = "unsupported <union> child <" + child_name + "> in type '" + model.fq_name + "'";
        return false;
      }
    }

    if (!have_discriminator) {
      error = "union '" + model.fq_name + "' is missing a discriminator";
      return false;
    }
    return add_type(model, error);
  }

  bool parse_union_case(XERCES_CPP_NAMESPACE::DOMElement* element,
                        const std::string& scope,
                        const std::string& union_name,
                        UnionCaseModel& case_model,
                        std::string& error)
  {
    bool have_member = false;
    for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* child_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string child_name = local_name(child_element);
      if (child_name == "caseDiscriminator") {
        const std::string value = required_attr(child_element, "value", child_name, error);
        if (!error.empty()) {
          return false;
        }
        if (value == "default") {
          case_model.default_label = true;
        } else {
          case_model.labels.push_back(value);
        }
      } else if (child_name == "member") {
        if (have_member) {
          error = "union '" + union_name + "' has a case with multiple members";
          return false;
        }
        if (!parse_member(child_element, scope, "member", case_model.member, error)) {
          return false;
        }
        have_member = true;
      } else {
        error = "unsupported <case> child <" + child_name + "> in union '" + union_name + "'";
        return false;
      }
    }

    if (!have_member) {
      error = "union '" + union_name + "' has a case without a member";
      return false;
    }
    if (!case_model.default_label && case_model.labels.empty()) {
      error = "union '" + union_name + "' has a case without a discriminator value";
      return false;
    }
    return true;
  }

  bool parse_enum(XERCES_CPP_NAMESPACE::DOMElement* element,
                  const std::string& scope,
                  std::string& error)
  {
    TypeModel model;
    model.kind = TYPE_ENUM;
    model.fq_name = join_name(scope, required_attr(element, "name", "enum", error));
    if (!error.empty()) {
      return false;
    }
    model.enum_model.extensibility = has_attr(element, "extensibility") ? attr(element, "extensibility") : "";
    if (has_attr(element, "bitBound")) {
      ACE_CDR::ULong bit_bound = 0;
      if (!parse_u32(attr(element, "bitBound"), bit_bound) || bit_bound > 32) {
        error = "invalid bitBound on enum '" + model.fq_name + "'";
        return false;
      }
      model.enum_model.bit_bound = static_cast<BitBound>(bit_bound);
    }

    bool have_default_literal = false;
    for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* literal_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string child_name = local_name(literal_element);
      if (child_name != "enumerator") {
        error = "unsupported <enum> child <" + child_name + "> in type '" + model.fq_name + "'";
        return false;
      }
      EnumLiteralModel literal;
      literal.name = required_attr(literal_element, "name", child_name, error);
      if (!error.empty()) {
        return false;
      }
      if (has_attr(literal_element, "value")) {
        if (!parse_i32(attr(literal_element, "value"), literal.value)) {
          error = "invalid enumerator value in enum '" + model.fq_name + "'";
          return false;
        }
      } else {
        literal.value = static_cast<ACE_CDR::Long>(model.enum_model.literals.size());
      }
      if (has_attr(literal_element, "defaultLiteral")) {
        if (!parse_bool(attr(literal_element, "defaultLiteral"), literal.default_literal)) {
          error = "invalid defaultLiteral value in enum '" + model.fq_name + "'";
          return false;
        }
        if (literal.default_literal) {
          if (have_default_literal) {
            error = "enum '" + model.fq_name + "' has multiple default literals";
            return false;
          }
          have_default_literal = true;
        }
      }
      model.enum_model.literal_values[literal.name.c_str()] = literal.value;
      model.enum_model.literals.push_back(literal);
    }
    if (model.enum_model.literals.empty()) {
      error = "enum '" + model.fq_name + "' does not define any enumerators";
      return false;
    }
    if (!have_default_literal) {
      model.enum_model.literals[0].default_literal = true;
    }

    return add_type(model, error);
  }

  bool parse_bitmask(XERCES_CPP_NAMESPACE::DOMElement* element,
                     const std::string& scope,
                     std::string& error)
  {
    TypeModel model;
    model.kind = TYPE_BITMASK;
    model.fq_name = join_name(scope, required_attr(element, "name", "bitmask", error));
    if (!error.empty()) {
      return false;
    }
    if (has_attr(element, "bitBound")) {
      ACE_CDR::ULong bit_bound = 0;
      if (!parse_u32(attr(element, "bitBound"), bit_bound) || bit_bound > 64) {
        error = "invalid bitBound on bitmask '" + model.fq_name + "'";
        return false;
      }
      model.bitmask_model.bit_bound = static_cast<BitBound>(bit_bound);
    }
    model.bitmask_model.extensibility = attr(element, "extensibility");

    for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child; child = child->getNextSibling()) {
      if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
        continue;
      }
      XERCES_CPP_NAMESPACE::DOMElement* flag_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
      const std::string child_name = local_name(flag_element);
      if (child_name != "flag") {
        error = "unsupported <bitmask> child <" + child_name + "> in type '" + model.fq_name + "'";
        return false;
      }
      BitmaskFlagModel flag;
      flag.name = required_attr(flag_element, "name", child_name, error);
      if (!error.empty()) {
        return false;
      }
      ACE_CDR::ULong value = 0;
      if (!parse_u32(required_attr(flag_element, "value", child_name, error), value) || value > 63) {
        error = "invalid flag value in bitmask '" + model.fq_name + "'";
        return false;
      }
      flag.position = static_cast<ACE_CDR::UShort>(value);
      if (value < 32) {
        model.bitmask_model.label_values[flag.name.c_str()] =
          static_cast<ACE_CDR::Long>(ACE_CDR::ULong(1) << value);
      }
      model.bitmask_model.flags.push_back(flag);
    }

    return add_type(model, error);
  }

  bool parse_member(XERCES_CPP_NAMESPACE::DOMElement* element,
                    const std::string& scope,
                    const std::string& element_name,
                    MemberModel& member,
                    std::string& error)
  {
    member.name = required_attr(element, "name", element_name, error);
    if (!error.empty()) {
      return false;
    }
    if (!parse_type_spec(element, scope, member.type, error)) {
      return false;
    }

    if (has_attr(element, "id")) {
      ACE_CDR::ULong id = 0;
      if (!parse_u32(attr(element, "id"), id)) {
        error = "invalid member id on member '" + member.name + "'";
        return false;
      }
      member.has_id = true;
      member.id = id;
    }
    if (has_attr(element, "key") && !parse_bool(attr(element, "key"), member.key)) {
      error = "invalid key value on member '" + member.name + "'";
      return false;
    }
    if (has_attr(element, "mustUnderstand") &&
        !parse_bool(attr(element, "mustUnderstand"), member.must_understand)) {
      error = "invalid mustUnderstand value on member '" + member.name + "'";
      return false;
    }
    if (has_attr(element, "optional") && !parse_bool(attr(element, "optional"), member.optional)) {
      error = "invalid optional value on member '" + member.name + "'";
      return false;
    }
    if (has_attr(element, "external") && !parse_bool(attr(element, "external"), member.external)) {
      error = "invalid external value on member '" + member.name + "'";
      return false;
    }
    member.try_construct = has_attr(element, "tryConstruct") ? attr(element, "tryConstruct") : "";
    if (has_attr(element, "hashid")) {
      member.has_hashid = true;
      member.hashid = attr(element, "hashid");
    }
    return true;
  }

  bool parse_type_spec(XERCES_CPP_NAMESPACE::DOMElement* element,
                       const std::string& scope,
                       TypeSpec& spec,
                       std::string& error)
  {
    const std::string type = required_attr(element, "type", local_name(element), error);
    if (!error.empty()) {
      return false;
    }
    spec.scope = scope;

    if (type == "nonBasic") {
      spec.kind = SPEC_NAMED;
      spec.named_type = required_attr(element, "nonBasicTypeName", local_name(element), error);
      if (!error.empty()) {
        return false;
      }
    } else if (type == "string" || type == "wstring") {
      spec.kind = SPEC_STRING;
      spec.wide_string = type == "wstring";
      if (has_attr(element, "stringMaxLength")) {
        if (!parse_bound(attr(element, "stringMaxLength"), spec.string_bound)) {
          error = "invalid stringMaxLength on " + local_name(element);
          return false;
        }
      }
    } else if (primitive_kind(type, spec.primitive_kind)) {
      spec.kind = SPEC_PRIMITIVE;
    } else if (type == "map") {
      spec.kind = SPEC_MAP;
      if (has_attr(element, "mapMaxLength")) {
        if (!parse_bound(attr(element, "mapMaxLength"), spec.map_bound)) {
          error = "invalid mapMaxLength on " + local_name(element);
          return false;
        }
      }
      for (XERCES_CPP_NAMESPACE::DOMNode* child = element->getFirstChild(); child;
           child = child->getNextSibling()) {
        if (child->getNodeType() != XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE) {
          continue;
        }
        XERCES_CPP_NAMESPACE::DOMElement* child_element = static_cast<XERCES_CPP_NAMESPACE::DOMElement*>(child);
        const std::string child_name = local_name(child_element);
        DCPS::unique_ptr<TypeSpec>* child_spec = 0;
        if (child_name == "key") {
          child_spec = &spec.map_key;
        } else if (child_name == "value") {
          child_spec = &spec.map_value;
        } else {
          error = "unsupported <map> child <" + child_name + "> on " + local_name(element);
          return false;
        }
        if (child_spec->get()) {
          error = "duplicate <" + child_name + "> child on map " + local_name(element);
          return false;
        }
        child_spec->reset(new TypeSpec);
        if (!parse_type_spec(child_element, scope, **child_spec, error)) {
          error = "map " + child_name + ": " + error;
          return false;
        }
      }
      if (!spec.map_key || !spec.map_value) {
        error = "map on " + local_name(element) + " requires one <key> and one <value>";
        return false;
      }
      if (has_attr(element, "sequenceMaxLength") || has_attr(element, "arrayDimensions")) {
        error = "combining map with sequenceMaxLength or arrayDimensions is not supported";
        return false;
      }
    } else {
      error = "unsupported XML type reference '" + type + "'";
      return false;
    }

    if (has_attr(element, "sequenceMaxLength")) {
      spec.has_sequence = true;
      if (!parse_bound(attr(element, "sequenceMaxLength"), spec.sequence_bound)) {
        error = "invalid sequenceMaxLength on " + local_name(element);
        return false;
      }
    }
    if (has_attr(element, "arrayDimensions")) {
      spec.has_array = true;
      if (!parse_dimensions(attr(element, "arrayDimensions"), spec.array_dimensions)) {
        error = "invalid arrayDimensions on " + local_name(element);
        return false;
      }
    }
    if (spec.has_sequence && spec.has_array) {
      error = "combining sequenceMaxLength and arrayDimensions is not supported";
      return false;
    }
    return true;
  }

  bool primitive_kind(const std::string& name, TypeKind& kind) const
  {
    if (name == "boolean") kind = TK_BOOLEAN;
    else if (name == "byte") kind = TK_BYTE;
    else if (name == "int8") kind = TK_INT8;
    else if (name == "int16") kind = TK_INT16;
    else if (name == "int32") kind = TK_INT32;
    else if (name == "int64") kind = TK_INT64;
    else if (name == "uint8") kind = TK_UINT8;
    else if (name == "uint16") kind = TK_UINT16;
    else if (name == "uint32") kind = TK_UINT32;
    else if (name == "uint64") kind = TK_UINT64;
    else if (name == "float32") kind = TK_FLOAT32;
    else if (name == "float64") kind = TK_FLOAT64;
    else if (name == "float128") kind = TK_FLOAT128;
    else if (name == "char8") kind = TK_CHAR8;
    else if (name == "char16") kind = TK_CHAR16;
    else return false;
    return true;
  }

  bool build_type(const std::string& fq_name, TypeIdentifier& ti, std::string& error)
  {
    const TypeIdentifierByName::const_iterator existing = complete_ti_by_name_.find(fq_name.c_str());
    if (existing != complete_ti_by_name_.end()) {
      ti = existing->second;
      return true;
    }
    if (building_.find(fq_name.c_str()) != building_.end()) {
      error = "cyclic XML type dependencies are not supported yet; cycle includes '" + fq_name + "'";
      return false;
    }

    const TypeModelMap::const_iterator pos = types_.find(fq_name.c_str());
    if (pos == types_.end()) {
      error = "unknown type '" + fq_name + "'";
      return false;
    }

    building_.insert(fq_name.c_str());
    CompleteTypeObject cto;
    bool ok = false;
    switch (pos->second.kind) {
    case TYPE_STRUCT:
      ok = build_struct(pos->second, cto, error);
      break;
    case TYPE_UNION:
      ok = build_union(pos->second, cto, error);
      break;
    case TYPE_ENUM:
      ok = build_enum(pos->second, cto, error);
      break;
    case TYPE_BITMASK:
      ok = build_bitmask(pos->second, cto, error);
      break;
    }
    building_.erase(fq_name.c_str());
    if (!ok) {
      return false;
    }

    const TypeObject to(cto);
    ti = makeTypeIdentifier(to);
    complete_ti_by_name_[fq_name.c_str()] = ti;
    complete_type_map_[ti] = to;
    return true;
  }

  bool build_struct(const TypeModel& model, CompleteTypeObject& cto, std::string& error)
  {
    CompleteStructType st;
    if (!type_flags(model.struct_model.extensibility, true, st.struct_flags, error)) {
      error = "struct '" + model.fq_name + "': " + error;
      return false;
    }
    if (model.struct_model.nested) {
      st.struct_flags = static_cast<StructTypeFlag>(st.struct_flags | IS_NESTED);
    }
    if (!model.struct_model.autoid.empty()) {
      if (model.struct_model.autoid == "hash") {
        st.struct_flags = static_cast<StructTypeFlag>(st.struct_flags | IS_AUTOID_HASH);
      } else if (model.struct_model.autoid != "sequential") {
        error = "struct '" + model.fq_name + "': unsupported autoid '" + model.struct_model.autoid + "'";
        return false;
      }
    }
    st.header.base_type = TypeIdentifier(TK_NONE);
    st.header.detail.type_name = model.fq_name.c_str();

    MemberId next_id = 0;
    for (size_t i = 0; i != model.struct_model.members.size(); ++i) {
      const MemberModel& member = model.struct_model.members[i];
      CompleteStructMember csm;
      if (!member_id(model.struct_model.autoid, member, next_id, csm.common.member_id, error)) {
        error = "struct '" + model.fq_name + "' member '" + member.name + "': " + error;
        return false;
      }
      if (!member_flags(member, csm.common.member_flags, error)) {
        error = "struct '" + model.fq_name + "' member '" + member.name + "': " + error;
        return false;
      }
      if (!type_identifier(member.type, csm.common.member_type_id, error)) {
        error = "struct '" + model.fq_name + "' member '" + member.name + "': " + error;
        return false;
      }
      csm.detail = member_detail(member);
      st.member_seq.append(csm);
    }

    cto = CompleteTypeObject(st);
    return true;
  }

  bool build_union(const TypeModel& model, CompleteTypeObject& cto, std::string& error)
  {
    CompleteUnionType ut;
    if (!type_flags(model.union_model.extensibility, true, ut.union_flags, error)) {
      error = "union '" + model.fq_name + "': " + error;
      return false;
    }
    if (model.union_model.nested) {
      ut.union_flags = static_cast<UnionTypeFlag>(ut.union_flags | IS_NESTED);
    }
    if (!model.union_model.autoid.empty()) {
      if (model.union_model.autoid == "hash") {
        ut.union_flags = static_cast<UnionTypeFlag>(ut.union_flags | IS_AUTOID_HASH);
      } else if (model.union_model.autoid != "sequential") {
        error = "union '" + model.fq_name + "': unsupported autoid '" + model.union_model.autoid + "'";
        return false;
      }
    }
    ut.header.detail.type_name = model.fq_name.c_str();
    if (!type_identifier(model.union_model.discriminator, ut.discriminator.common.type_id, error)) {
      error = "union '" + model.fq_name + "' discriminator: " + error;
      return false;
    }
    if (!try_construct_flags(model.union_model.discriminator_try_construct,
                             ut.discriminator.common.member_flags, error)) {
      error = "union '" + model.fq_name + "' discriminator: " + error;
      return false;
    }
    if (model.union_model.discriminator_key) {
      ut.discriminator.common.member_flags =
        static_cast<UnionDiscriminatorFlag>(ut.discriminator.common.member_flags | IS_KEY);
    }

    MemberId next_id = 0;
    for (size_t i = 0; i != model.union_model.cases.size(); ++i) {
      const UnionCaseModel& case_model = model.union_model.cases[i];
      CompleteUnionMember cum;
      if (!member_id(model.union_model.autoid, case_model.member, next_id, cum.common.member_id, error)) {
        error = "union '" + model.fq_name + "' member '" + case_model.member.name + "': " + error;
        return false;
      }
      StructMemberFlag parsed_member_flags = 0;
      if (!member_flags(case_model.member, parsed_member_flags, error)) {
        error = "union '" + model.fq_name + "' member '" + case_model.member.name + "': " + error;
        return false;
      }
      cum.common.member_flags = static_cast<UnionMemberFlag>(parsed_member_flags & (TRY_CONSTRUCT1 | TRY_CONSTRUCT2 | IS_EXTERNAL));
      if (case_model.default_label) {
        cum.common.member_flags = static_cast<UnionMemberFlag>(cum.common.member_flags | IS_DEFAULT);
      }
      if (!type_identifier(case_model.member.type, cum.common.type_id, error)) {
        error = "union '" + model.fq_name + "' member '" + case_model.member.name + "': " + error;
        return false;
      }
      for (size_t j = 0; j != case_model.labels.size(); ++j) {
        ACE_CDR::Long label = 0;
        if (!case_label(model.union_model.discriminator, case_model.labels[j], label, error)) {
          error = "union '" + model.fq_name + "' case label '" + case_model.labels[j] + "': " + error;
          return false;
        }
        cum.common.label_seq.append(label);
      }
      cum.detail = member_detail(case_model.member);
      ut.member_seq.append(cum);
    }

    cto = CompleteTypeObject(ut);
    return true;
  }

  bool build_enum(const TypeModel& model, CompleteTypeObject& cto, std::string& error)
  {
    CompleteEnumeratedType et;
    if (!type_flags(model.enum_model.extensibility, false, et.enum_flags, error)) {
      error = "enum '" + model.fq_name + "': " + error;
      return false;
    }
    et.header.common.bit_bound = model.enum_model.bit_bound;
    et.header.detail.type_name = model.fq_name.c_str();
    for (size_t i = 0; i != model.enum_model.literals.size(); ++i) {
      const EnumLiteralModel& literal = model.enum_model.literals[i];
      CompleteEnumeratedLiteral cel;
      cel.common.value = literal.value;
      cel.common.flags = literal.default_literal ? IS_DEFAULT : 0;
      cel.detail.name = literal.name.c_str();
      et.literal_seq.append(cel);
    }
    cto = CompleteTypeObject(et);
    return true;
  }

  bool build_bitmask(const TypeModel& model, CompleteTypeObject& cto, std::string& error)
  {
    CompleteBitmaskType bt;
    if (model.bitmask_model.extensibility.empty()) {
      bt.bitmask_flags = IS_FINAL;
    } else if (!type_flags(model.bitmask_model.extensibility, false, bt.bitmask_flags, error)) {
      return false;
    }
    bt.header.common.bit_bound = model.bitmask_model.bit_bound;
    bt.header.detail.type_name = model.fq_name.c_str();
    for (size_t i = 0; i != model.bitmask_model.flags.size(); ++i) {
      const BitmaskFlagModel& flag = model.bitmask_model.flags[i];
      CompleteBitflag cbf;
      cbf.common.position = flag.position;
      cbf.detail.name = flag.name.c_str();
      bt.flag_seq.append(cbf);
    }
    cto = CompleteTypeObject(bt);
    return true;
  }

  bool type_flags(const std::string& value, bool default_appendable, TypeFlag& flags, std::string& error) const
  {
    if (value.empty()) {
      flags = default_appendable ? IS_APPENDABLE : 0;
      return true;
    }
    if (value == "final") {
      flags = IS_FINAL;
    } else if (value == "appendable") {
      flags = IS_APPENDABLE;
    } else if (value == "mutable") {
      flags = IS_MUTABLE;
    } else {
      error = "unsupported extensibility '" + value + "'";
      return false;
    }
    return true;
  }

  bool try_construct_flags(const std::string& value, MemberFlag& flags, std::string& error) const
  {
    if (value.empty() || value == "discard") {
      flags = TRY_CONSTRUCT1;
    } else if (value == "use_default") {
      flags = TRY_CONSTRUCT2;
    } else if (value == "trim") {
      flags = TRY_CONSTRUCT1 | TRY_CONSTRUCT2;
    } else {
      error = "unsupported tryConstruct '" + value + "'";
      return false;
    }
    return true;
  }

  bool member_flags(const MemberModel& member, StructMemberFlag& flags, std::string& error) const
  {
    MemberFlag parsed = 0;
    if (!try_construct_flags(member.try_construct, parsed, error)) {
      return false;
    }
    flags = static_cast<StructMemberFlag>(parsed);
    if (member.key) {
      flags = static_cast<StructMemberFlag>(flags | IS_KEY);
    }
    if (member.must_understand) {
      flags = static_cast<StructMemberFlag>(flags | IS_MUST_UNDERSTAND);
    }
    if (member.optional) {
      flags = static_cast<StructMemberFlag>(flags | IS_OPTIONAL);
    }
    if (member.external) {
      flags = static_cast<StructMemberFlag>(flags | IS_EXTERNAL);
    }
    return true;
  }

  bool member_id(const std::string& autoid, const MemberModel& member, MemberId& next_id,
                 MemberId& id, std::string& error) const
  {
    if (member.has_id) {
      id = member.id;
      next_id = id + 1;
      return true;
    }
    if (member.has_hashid) {
      id = hash_member_name_to_id(member.hashid.empty() ? member.name.c_str() : member.hashid.c_str());
      return true;
    }
    if (autoid == "hash") {
      id = hash_member_name_to_id(member.name.c_str());
      return true;
    }
    if (!autoid.empty() && autoid != "sequential") {
      error = "unsupported autoid '" + autoid + "'";
      return false;
    }
    id = next_id++;
    return true;
  }

  CompleteMemberDetail member_detail(const MemberModel& member) const
  {
    OPENDDS_OPTIONAL_NS::optional<AppliedBuiltinMemberAnnotations> ann_builtin;
    if (member.has_hashid) {
      OPENDDS_OPTIONAL_NS::optional<DCPS::String> unit;
      OPENDDS_OPTIONAL_NS::optional<AnnotationParameterValue> min;
      OPENDDS_OPTIONAL_NS::optional<AnnotationParameterValue> max;
      OPENDDS_OPTIONAL_NS::optional<DCPS::String> hash_id(DCPS::String(member.hashid.c_str()));
      ann_builtin = AppliedBuiltinMemberAnnotations(unit, min, max, hash_id);
    }
    OPENDDS_OPTIONAL_NS::optional<AppliedAnnotationSeq> ann_custom;
    return CompleteMemberDetail(member.name.c_str(), ann_builtin, ann_custom);
  }

  TypeIdentifier string_type_identifier(bool wide, LBound bound) const
  {
    if (bound <= std::numeric_limits<SBound>::max()) {
      return TypeIdentifier(wide ? TI_STRING16_SMALL : TI_STRING8_SMALL,
                            StringSTypeDefn(static_cast<SBound>(bound)));
    }
    return TypeIdentifier(wide ? TI_STRING16_LARGE : TI_STRING8_LARGE,
                          StringLTypeDefn(bound));
  }

  TypeIdentifier sequence_type_identifier(const TypeIdentifier& element, LBound bound) const
  {
    const PlainCollectionHeader header(is_fully_descriptive(element) ? EK_BOTH : EK_COMPLETE, TRY_CONSTRUCT1);
    if (bound <= std::numeric_limits<SBound>::max()) {
      return TypeIdentifier(TI_PLAIN_SEQUENCE_SMALL,
                            PlainSequenceSElemDefn(header, static_cast<SBound>(bound), element));
    }
    return TypeIdentifier(TI_PLAIN_SEQUENCE_LARGE,
                          PlainSequenceLElemDefn(header, bound, element));
  }

  TypeIdentifier array_type_identifier(const TypeIdentifier& element,
                                       const OPENDDS_VECTOR(LBound)& dimensions) const
  {
    const PlainCollectionHeader header(is_fully_descriptive(element) ? EK_BOTH : EK_COMPLETE, TRY_CONSTRUCT1);
    bool small = true;
    for (size_t i = 0; i != dimensions.size(); ++i) {
      if (dimensions[i] > std::numeric_limits<SBound>::max()) {
        small = false;
        break;
      }
    }

    if (small) {
      SBoundSeq bounds;
      bounds.length(static_cast<ACE_CDR::ULong>(dimensions.size()));
      for (ACE_CDR::ULong i = 0; i != bounds.length(); ++i) {
        bounds[i] = static_cast<SBound>(dimensions[i]);
      }
      return TypeIdentifier(TI_PLAIN_ARRAY_SMALL,
                            PlainArraySElemDefn(header, bounds, element));
    }

    LBoundSeq bounds;
    bounds.length(static_cast<ACE_CDR::ULong>(dimensions.size()));
    for (ACE_CDR::ULong i = 0; i != bounds.length(); ++i) {
      bounds[i] = dimensions[i];
    }
    return TypeIdentifier(TI_PLAIN_ARRAY_LARGE,
                          PlainArrayLElemDefn(header, bounds, element));
  }

  TypeIdentifier map_type_identifier(const TypeIdentifier& element,
                                     const TypeIdentifier& key,
                                     LBound bound) const
  {
    const PlainCollectionHeader header(
      is_fully_descriptive(element) && is_fully_descriptive(key) ? EK_BOTH : EK_COMPLETE,
      TRY_CONSTRUCT1);
    const CollectionElementFlag key_flags = 0;
    if (bound <= std::numeric_limits<SBound>::max()) {
      return TypeIdentifier(TI_PLAIN_MAP_SMALL,
                            PlainMapSTypeDefn(header, static_cast<SBound>(bound),
                                              element, key_flags, key));
    }
    return TypeIdentifier(TI_PLAIN_MAP_LARGE,
                          PlainMapLTypeDefn(header, bound, element, key_flags, key));
  }

  bool type_identifier(const TypeSpec& spec, TypeIdentifier& ti, std::string& error)
  {
    switch (spec.kind) {
    case SPEC_PRIMITIVE:
      ti = TypeIdentifier(spec.primitive_kind);
      break;
    case SPEC_STRING:
      ti = string_type_identifier(spec.wide_string, spec.string_bound);
      break;
    case SPEC_NAMED: {
      const std::string resolved = resolve_name(spec.named_type, spec.scope);
      if (resolved.empty()) {
        error = "unknown nonBasic type '" + spec.named_type + "'";
        return false;
      }
      if (!build_type(resolved, ti, error)) {
        return false;
      }
      break;
    }
    case SPEC_MAP: {
      TypeIdentifier key_ti;
      if (!type_identifier(*spec.map_key, key_ti, error)) {
        error = "map key: " + error;
        return false;
      }
      TypeIdentifier element_ti;
      if (!type_identifier(*spec.map_value, element_ti, error)) {
        error = "map value: " + error;
        return false;
      }
      ti = map_type_identifier(element_ti, key_ti, spec.map_bound);
      break;
    }
    }

    if (spec.has_sequence) {
      ti = sequence_type_identifier(ti, spec.sequence_bound);
    } else if (spec.has_array) {
      ti = array_type_identifier(ti, spec.array_dimensions);
    }
    return true;
  }

  std::string resolve_name(const std::string& name, const std::string& scope) const
  {
    const std::string stripped = strip_leading_colons(name);
    if (stripped.find("::") != std::string::npos) {
      return types_.find(stripped.c_str()) == types_.end() ? std::string() : stripped;
    }

    std::string current = scope;
    while (true) {
      const std::string candidate = join_name(current, stripped);
      if (types_.find(candidate.c_str()) != types_.end()) {
        return candidate;
      }
      if (current.empty()) {
        break;
      }
      current = parent_scope(current);
    }
    return std::string();
  }

  bool case_label(const TypeSpec& discriminator, const std::string& value,
                  ACE_CDR::Long& label, std::string& error) const
  {
    if (parse_i32(value, label)) {
      return true;
    }

    if (discriminator.kind == SPEC_NAMED) {
      const std::string resolved = resolve_name(discriminator.named_type, discriminator.scope);
      const TypeModelMap::const_iterator type = types_.find(resolved.c_str());
      if (type != types_.end()) {
        if (type->second.kind == TYPE_ENUM) {
          const OPENDDS_MAP(OPENDDS_STRING, ACE_CDR::Long)::const_iterator pos =
            type->second.enum_model.literal_values.find(value.c_str());
          if (pos != type->second.enum_model.literal_values.end()) {
            label = pos->second;
            return true;
          }
        } else if (type->second.kind == TYPE_BITMASK) {
          const OPENDDS_MAP(OPENDDS_STRING, ACE_CDR::Long)::const_iterator pos =
            type->second.bitmask_model.label_values.find(value.c_str());
          if (pos != type->second.bitmask_model.label_values.end()) {
            label = pos->second;
            return true;
          }
        }
      }
    }

    error = "label is neither an integer nor a known discriminator literal";
    return false;
  }

  void report(const std::string& error, const std::string& type_name) const
  {
    if (DCPS::log_level >= DCPS::LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: OpenDDS::XTypes::load_xml_type(%C, %C): %C\n"),
        file_.c_str(), type_name.c_str(), error.c_str()));
    }
  }

  std::string file_;
  TypeModelMap types_;
  TypeIdentifierByName complete_ti_by_name_;
  NameSet building_;
  TypeMap complete_type_map_;
  TypeMap minimal_type_map_;
};

} // namespace

DDS::ReturnCode_t load_xml_type(DDS::DynamicType_var& type,
                                const ACE_TString& xml_file,
                                const std::string& fully_qualified_type_name)
{
  XmlTypeLoader loader(xml_file);
  return loader.load(type, fully_qualified_type_name);
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_XERCES3 && !OPENDDS_SAFETY_PROFILE
