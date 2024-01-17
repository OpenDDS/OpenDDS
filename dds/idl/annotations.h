/**
 * /file annotations.h
 *
 * Wrappers for accessing data from IDL annotations and registers the
 * annotation by evaluating its definition. Each annotation (@key, @topic,
 * etc.) should have class.
 *
 * To add a new annotation, implement a subclass of Annotation, implementing at
 * least definition() and name(), then add a register_one call for the class to
 * Annotations::register_all() in annotations.cpp.
 *
 * OpenDDS-specific annotations should go in the OpenDDS C++ namespace and the
 * OpenDDS IDL module. Only standardized annotations should be outside those
 * scopes.
 */

#ifndef OPENDDS_IDL_ANNOTATIONS_HEADER
#define OPENDDS_IDL_ANNOTATIONS_HEADER

#include "../Versioned_Namespace.h"
#include "../DCPS/Definitions.h"

#include <ast_expression.h>

#include <ace/Basic_Types.h>

#include <vector>
#include <string>
#include <map>
#include <set>

class AST_Array;
class AST_Sequence;
class AST_Map;
class AST_Decl;
class AST_Union;
class AST_Annotation_Decl;
class AST_Annotation_Appl;
class Annotation;

class Annotations {
public:
  Annotations();
  ~Annotations();

  void register_all();
  Annotation* operator[](const std::string& annotation) const;

  template<typename T>
  void register_one()
  {
    T* annotation = new T;
    map_[annotation->fullname()] = annotation;
    annotation->cache();
  }

private:
  typedef std::map<std::string, Annotation*> MapType;
  MapType map_;
};

/**
 * Wrapper Base Class for Annotations
 */
class Annotation {
public:
  Annotation();
  virtual ~Annotation();

  virtual std::string definition() const = 0;
  virtual std::string name() const = 0;
  virtual std::string module() const;
  virtual std::string fullname() const;

  AST_Annotation_Decl* declaration() const;
  AST_Annotation_Appl* find_on(AST_Decl* node) const;
  void cache();

private:
  AST_Annotation_Decl* declaration_;
};

AST_Expression::AST_ExprValue* get_annotation_member_ev(AST_Annotation_Appl* appl,
                                                        const char* member_name,
                                                        AST_Expression::ExprType type);

bool get_bool_annotation_member_value(AST_Annotation_Appl* appl,
                                      const char* member_name);

ACE_UINT32 get_u32_annotation_member_value(AST_Annotation_Appl* appl,
                                           const char* member_name);

std::string get_str_annotation_member_value(AST_Annotation_Appl* appl,
                                            const char* member_name);

/**
 * Annotation that logically provide a value when absent.
 */
template <typename T>
class AbsentValue {
public:
  explicit AbsentValue(const T& value)
    : absent_value(value)
  {}

  const T absent_value;
};

/**
 * Annotation with a Single Member Named "value"
 */
template <typename T>
class AnnotationWithValue : public Annotation {
public:
  /**
   * If node has the annotation, this sets value to the annotation value and
   * returns true.  Returns false otherwise.
   */
  virtual bool node_value_exists(AST_Decl* node, T& value) const
  {
    AST_Annotation_Appl* appl = find_on(node);
    if (!appl) { return false; }

    value = value_from_appl(appl);
    return true;
  }

protected:
  /* NOTE: Derived classes should either override value_from_appl.  A
     default implementation is provided so template functions can be
     defined. */
  virtual T value_from_appl(AST_Annotation_Appl*) const
  {
    return T();
  }
};

template<>
bool AnnotationWithValue<bool>::value_from_appl(AST_Annotation_Appl* appl) const;

template<>
unsigned AnnotationWithValue<ACE_UINT32>::value_from_appl(AST_Annotation_Appl* appl) const;

template<>
std::string AnnotationWithValue<std::string>::value_from_appl(AST_Annotation_Appl* appl) const;

template <typename T>
class AnnotationWithEnumValue : public AnnotationWithValue<T> {
protected:
  T value_from_appl(AST_Annotation_Appl* appl) const
  {
    return static_cast<T>(get_u32_annotation_member_value(appl, "value"));
  }
};

// @key ======================================================================

class KeyAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  KeyAnnotation()
    : AbsentValue<bool>(false)
  {}

  std::string definition() const;
  std::string name() const;

  bool union_value(AST_Union* node) const;
};

// @topic ====================================================================

struct TopicValue {
  std::string name;
  std::string platform;

  TopicValue()
    : name("")
    , platform("*")
  {}
};

class TopicAnnotation : public AnnotationWithValue<TopicValue> {
public:
  TopicAnnotation();

  std::string definition() const;
  std::string name() const;

private:
  TopicValue value_from_appl(AST_Annotation_Appl* appl) const;
};

// @nested ===================================================================

class NestedAnnotation : public AnnotationWithValue<bool> {
public:
  std::string definition() const;
  std::string name() const;
};

// @default_nested ===========================================================

class DefaultNestedAnnotation : public AnnotationWithValue<bool> {
public:
  std::string definition() const;
  std::string name() const;
};

// @id =======================================================================

class IdAnnotation : public AnnotationWithValue<ACE_UINT32> {
public:
  std::string definition() const;
  std::string name() const;
};

// @autoid ===================================================================

enum AutoidKind {
  autoidkind_sequential,
  autoidkind_hash
};

class AutoidAnnotation : public AnnotationWithEnumValue<AutoidKind>, public AbsentValue<AutoidKind> {
public:
  AutoidAnnotation()
    : AbsentValue<AutoidKind>(autoidkind_sequential)
  {}

  std::string definition() const;
  std::string name() const;
};

// @hashid ===================================================================

class HashidAnnotation : public AnnotationWithValue<std::string> {
public:
  std::string definition() const;
  std::string name() const;
};

// @optional ===================================================================

class OptionalAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  OptionalAnnotation()
    : AbsentValue<bool>(false)
  {}

  std::string definition() const;
  std::string name() const;
};

// @must_understand ============================================================

class MustUnderstandAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  MustUnderstandAnnotation()
    : AbsentValue<bool>(false)
  {}

  std::string definition() const;
  std::string name() const;
};

// @external ===================================================================

class ExternalAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  ExternalAnnotation()
    : AbsentValue<bool>(false)
  {}

  std::string definition() const;
  std::string name() const;
};

// @extensibility ============================================================

enum ExtensibilityKind {
  extensibilitykind_final,
  extensibilitykind_appendable,
  extensibilitykind_mutable
};

class ExtensibilityAnnotation : public AnnotationWithEnumValue<ExtensibilityKind> {
public:
  std::string definition() const;
  std::string name() const;
};

// @final ====================================================================

class FinalAnnotation : public Annotation {
public:
  std::string definition() const;
  std::string name() const;
};

// @appendable ===============================================================

class AppendableAnnotation : public Annotation {
public:
  std::string definition() const;
  std::string name() const;
};

// @mutable ==================================================================

class MutableAnnotation : public Annotation {
public:
  std::string definition() const;
  std::string name() const;
};

// @try_construct ============================================================

enum TryConstructFailAction {
  tryconstructfailaction_discard,
  tryconstructfailaction_use_default,
  tryconstructfailaction_trim,
};

class TryConstructAnnotation
  : public AnnotationWithEnumValue<TryConstructFailAction>
  , public AbsentValue<TryConstructFailAction> {
public:
  TryConstructAnnotation()
    : AbsentValue<TryConstructFailAction>(tryconstructfailaction_discard)
  {}

  std::string definition() const;
  std::string name() const;

  TryConstructFailAction sequence_element_value(AST_Sequence* node) const;
  TryConstructFailAction array_element_value(AST_Array* node) const;
  TryConstructFailAction union_value(AST_Union* node) const;

#if OPENDDS_HAS_IDL_MAP
  TryConstructFailAction map_key(AST_Map* node) const;
  TryConstructFailAction map_value(AST_Map* node) const;
#endif
};

// OpenDDS Specific Annotations
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {

  // @OpenDDS::data_representation ===========================================

  struct DataRepresentation {
    bool xcdr1;
    bool xcdr2;
    bool xml;
    bool unaligned;

    DataRepresentation()
    {
      set_all(false);
    }

    void add(const DataRepresentation& other)
    {
      xcdr1 |= other.xcdr1;
      xcdr2 |= other.xcdr2;
      xml |= other.xml;
      unaligned |= other.unaligned;
    }

    void set_all(bool value)
    {
      xcdr1 = value;
      xcdr2 = value;
      xml = value;
      unaligned = value;
    }

    bool only_xcdr1() const
    {
      return xcdr1 && !xcdr2 && !xml;
    }

    bool not_only_xcdr1() const
    {
      return xcdr1 && (xcdr2 || xml);
    }

    bool only_xcdr2() const
    {
      return !xcdr1 && xcdr2 && !xml;
    }

    bool not_only_xcdr2() const
    {
      return xcdr2 && (xcdr1 || xml);
    }

    bool only_xml() const
    {
      return !xcdr1 && !xcdr2 && xml;
    }

    bool not_only_xml() const
    {
      return xml && (xcdr1 || xcdr2);
    }
  };

  /**
   * Used to specify the allowed RTPS data representations in XTypes.
   * Replacement for @::data_representation which requires bitmask.
   */
  class DataRepresentationAnnotation :
    public AnnotationWithValue<DataRepresentation> {
  public:
    std::string definition() const;
    std::string name() const;
    std::string module() const;

    bool node_value_exists(AST_Decl* node, DataRepresentation& value) const;

  protected:
    DataRepresentation value_from_appl(AST_Annotation_Appl* appl) const;
  };

  namespace internal {
    /**
     * Types with this annotation will not get a DynamicDataAdapterImpl generated
     * for them. Attempting to access struct or union members with this
     * annotation on their type will result in an UNSUPPORTED retcode.
     * get_dynamic_data_adapter for these types will be generated, but will
     * return nullptr.
     */
    struct NoDynamicDataAdapterAnnotation : public Annotation {
      std::string definition() const
      {
        return
          "module OpenDDS {\n"
          "  module internal {\n"
          "    @annotation no_dynamic_data_adapter {\n"
          "    };\n"
          "  };\n"
          "};\n";
      }

      std::string name() const
      {
        return "no_dynamic_data_adapter";
      }

      std::string module() const
      {
        return "::OpenDDS::internal::";
      }
    };

    /**
     * Types with this annotation have a special serialization case in
     * marshal_generator.
     */
    class SpecialSerializationAnnotation : public AnnotationWithValue<std::string> {
    public:
      std::string definition() const
      {
        return
          "module OpenDDS {\n"
          "  module internal {\n"
          "    @annotation special_serialization {\n"
          "      string template_name default \"\";\n"
          "    };\n"
          "  };\n"
          "};\n";
      }

      std::string name() const
      {
        return "special_serialization";
      }

      std::string module() const
      {
        return "::OpenDDS::internal::";
      }

    protected:
      std::string value_from_appl(AST_Annotation_Appl* appl) const
      {
        return get_str_annotation_member_value(appl, "template_name");
      }
    };
  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
