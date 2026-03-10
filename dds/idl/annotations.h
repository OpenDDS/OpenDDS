/**
 * /file annotations.h
 *
 * Wrappers for accessing data from IDL annotations and registers the
 * annotation by evaluating its definition. Each annotation (@key, @topic,
 * etc.) should have class.
 *
 * To add a new annotation, implement a subclass of Annotation, implementing at
 * least definition() and a static name() method, then add a register_one call
 * for the class to Annotations::register_all() in annotations.cpp.
 *
 * OpenDDS-specific annotations should go in the OpenDDS C++ namespace and the
 * OpenDDS IDL module. Only standardized annotations should be outside those
 * scopes.
 */

#ifndef OPENDDS_IDL_ANNOTATIONS_HEADER
#define OPENDDS_IDL_ANNOTATIONS_HEADER

#include "../Versioned_Namespace.h"

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
class AnnotationBaseDoNotUse;

class Annotations {
public:
  Annotations();
  ~Annotations();

  void register_all();
  AnnotationBaseDoNotUse* operator[](const std::string& annotation) const;

  template<typename T>
  void register_one()
  {
    T* annotation = new T;
    map_[T::static_fullname()] = annotation;
    annotation->cache();
  }

  template<typename T>
  T* get() const
  {
    MapType::const_iterator it = map_.find(T::static_fullname());
    return it != map_.end() ? static_cast<T*>(it->second) : 0;
  }

private:
  typedef std::map<std::string, AnnotationBaseDoNotUse*> MapType;
  MapType map_;
};

/**
 * Non-template base class for polymorphism, shouldn't be used directly.
 */
class AnnotationBaseDoNotUse {
public:
  AnnotationBaseDoNotUse();
  virtual ~AnnotationBaseDoNotUse();

  virtual std::string definition() const = 0;
  virtual std::string fullname() const = 0;

  AST_Annotation_Decl* declaration() const;
  AST_Annotation_Appl* find_on(AST_Decl* node) const;
  void cache();

protected:
  AST_Annotation_Decl* declaration_;
};

/// Templated base with static methods
template <typename Derived>
class Annotation : public AnnotationBaseDoNotUse {
public:
  static const char* module() { return ""; }

  static std::string static_fullname()
  {
    std::string modules("::");
    const std::string module_names = Derived::module();
    if (!module_names.empty()) {
      modules += module_names + "::";
    }
    return modules + "@" + Derived::name();
  }

  /// Virtual override that calls static version
  std::string fullname() const { return static_fullname(); }
};

AST_Expression::AST_ExprValue* get_annotation_member_ev(AST_Annotation_Appl* appl,
                                                        const char* member_name,
                                                        AST_Expression::ExprType type);

bool get_bool_annotation_member_value(AST_Annotation_Appl* appl,
                                      const char* member_name);

ACE_UINT32 get_u32_annotation_member_value(AST_Annotation_Appl* appl,
                                           const char* member_name);

ACE_INT32 get_i32_annotation_member_value(AST_Annotation_Appl* appl,
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

// Dispatch helpers for value_from_appl - use overloading to select correct getter
namespace annotation_detail {
  inline bool call_value_getter(AST_Annotation_Appl* appl, bool*)
  {
    return get_bool_annotation_member_value(appl, "value");
  }

  inline ACE_UINT32 call_value_getter(AST_Annotation_Appl* appl, ACE_UINT32*)
  {
    return get_u32_annotation_member_value(appl, "value");
  }

  inline ACE_INT32 call_value_getter(AST_Annotation_Appl* appl, ACE_INT32*)
  {
    return get_i32_annotation_member_value(appl, "value");
  }

  inline std::string call_value_getter(AST_Annotation_Appl* appl, std::string*)
  {
    return get_str_annotation_member_value(appl, "value");
  }

  // Fallback for types without specific handling (e.g., custom value types)
  template<typename T>
  T call_value_getter(AST_Annotation_Appl*, T*)
  {
    return T();
  }
}

/**
 * Annotation with a Single Member Named "value"
 */
template <typename T, typename Derived>
class AnnotationWithValue : public Annotation<Derived> {
public:
  /**
   * If node has the annotation, this sets value to the annotation value and
   * returns true.  Returns false otherwise.
   */
  virtual bool node_value_exists(AST_Decl* node, T& value) const
  {
    AST_Annotation_Appl* appl = this->find_on(node);
    if (!appl) { return false; }

    value = value_from_appl(appl);
    return true;
  }

protected:
  /* NOTE: Derived classes should either override value_from_appl.  A
     default implementation is provided so template functions can be
     defined. */
  virtual T value_from_appl(AST_Annotation_Appl* appl) const
  {
    return annotation_detail::call_value_getter(appl, static_cast<T*>(0));
  }
};

template <typename T, typename Derived>
class AnnotationWithEnumValue : public AnnotationWithValue<T, Derived> {
protected:
  T value_from_appl(AST_Annotation_Appl* appl) const
  {
    return static_cast<T>(get_u32_annotation_member_value(appl, "value"));
  }
};

// @key ======================================================================

class KeyAnnotation : public AnnotationWithValue<bool, KeyAnnotation>, public AbsentValue<bool> {
public:
  KeyAnnotation()
    : AbsentValue<bool>(false)
  {}

  static const char* name() { return "key"; }
  std::string definition() const;

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

class TopicAnnotation : public AnnotationWithValue<TopicValue, TopicAnnotation> {
public:
  TopicAnnotation();

  static const char* name() { return "topic"; }
  std::string definition() const;

private:
  TopicValue value_from_appl(AST_Annotation_Appl* appl) const;
};

// @nested ===================================================================

class NestedAnnotation : public AnnotationWithValue<bool, NestedAnnotation> {
public:
  static const char* name() { return "nested"; }
  std::string definition() const;
};

// @default_nested ===========================================================

class DefaultNestedAnnotation : public AnnotationWithValue<bool, DefaultNestedAnnotation> {
public:
  static const char* name() { return "default_nested"; }
  std::string definition() const;
};

// @id =======================================================================

class IdAnnotation : public AnnotationWithValue<ACE_UINT32, IdAnnotation> {
public:
  static const char* name() { return "id"; }
  std::string definition() const;
};

// @autoid ===================================================================

enum AutoidKind {
  autoidkind_sequential,
  autoidkind_hash
};

class AutoidAnnotation :
  public AnnotationWithEnumValue<AutoidKind, AutoidAnnotation>, public AbsentValue<AutoidKind> {
public:
  AutoidAnnotation()
    : AbsentValue<AutoidKind>(autoidkind_sequential)
  {}

  static const char* name() { return "autoid"; }
  std::string definition() const;
};

// @hashid ===================================================================

class HashidAnnotation : public AnnotationWithValue<std::string, HashidAnnotation> {
public:
  static const char* name() { return "hashid"; }
  std::string definition() const;
};

// @optional ===================================================================

class OptionalAnnotation :
  public AnnotationWithValue<bool, OptionalAnnotation>, public AbsentValue<bool> {
public:
  OptionalAnnotation()
    : AbsentValue<bool>(false)
  {}

  static const char* name() { return "optional"; }
  std::string definition() const;
};

// @must_understand ============================================================

class MustUnderstandAnnotation :
  public AnnotationWithValue<bool, MustUnderstandAnnotation>, public AbsentValue<bool> {
public:
  MustUnderstandAnnotation()
    : AbsentValue<bool>(false)
  {}

  static const char* name() { return "must_understand"; }
  std::string definition() const;
};

// @external ===================================================================

class ExternalAnnotation :
  public AnnotationWithValue<bool, ExternalAnnotation>, public AbsentValue<bool> {
public:
  ExternalAnnotation()
    : AbsentValue<bool>(false)
  {}

  static const char* name() { return "external"; }
  std::string definition() const;
};

// @extensibility ============================================================

enum ExtensibilityKind {
  extensibilitykind_final,
  extensibilitykind_appendable,
  extensibilitykind_mutable
};

class ExtensibilityAnnotation :
  public AnnotationWithEnumValue<ExtensibilityKind, ExtensibilityAnnotation> {
public:
  static const char* name() { return "extensibility"; }
  std::string definition() const;
};

// @final ====================================================================

class FinalAnnotation : public Annotation<FinalAnnotation> {
public:
  static const char* name() { return "final"; }
  std::string definition() const;
};

// @appendable ===============================================================

class AppendableAnnotation : public Annotation<AppendableAnnotation> {
public:
  static const char* name() { return "appendable"; }
  std::string definition() const;
};

// @mutable ==================================================================

class MutableAnnotation : public Annotation<MutableAnnotation> {
public:
  static const char* name() { return "mutable"; }
  std::string definition() const;
};

// @try_construct ============================================================

enum TryConstructFailAction {
  tryconstructfailaction_discard,
  tryconstructfailaction_use_default,
  tryconstructfailaction_trim,
};

class TryConstructAnnotation
  : public AnnotationWithEnumValue<TryConstructFailAction, TryConstructAnnotation>
  , public AbsentValue<TryConstructFailAction> {
public:
  TryConstructAnnotation()
    : AbsentValue<TryConstructFailAction>(tryconstructfailaction_discard)
  {}

  static const char* name() { return "try_construct"; }
  std::string definition() const;

  TryConstructFailAction sequence_element_value(AST_Sequence* node) const;
  TryConstructFailAction array_element_value(AST_Array* node) const;
  TryConstructFailAction union_value(AST_Union* node) const;

  TryConstructFailAction map_key(AST_Map* node) const;
  TryConstructFailAction map_value(AST_Map* node) const;
};

// @value ====================================================================

class ValueAnnotation : public AnnotationWithValue<ACE_INT32, ValueAnnotation> {
public:
  // @value(long) is supported for enumerators
  // more general @value support as described in IDL4.2 is not yet supported
  static const char* name() { return "value"; }
  std::string definition() const;
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
    public AnnotationWithValue<DataRepresentation, DataRepresentationAnnotation> {
  public:
    static const char* name() { return "data_representation"; }
    static const char* module() { return "OpenDDS"; }
    std::string definition() const;

    bool node_value_exists(AST_Decl* node, DataRepresentation& value) const;

  protected:
    DataRepresentation value_from_appl(AST_Annotation_Appl* appl) const;
  };

  // @OpenDDS::no_init_before_deserialize ========================================

  /**
   * Indicate to the mapping and type support generators that this type should
   * not be initialized before deserialization to optimize reading samples.
   * This only works for primitive type sequences.
   */
  class NoInitBeforeDeserializeAnnotation :
    public AnnotationWithValue<bool, NoInitBeforeDeserializeAnnotation> {
  public:
    static const char* name() { return "no_init_before_deserialize"; }
    static const char* module() { return "OpenDDS"; }

    std::string definition() const
    {
      return
        "module OpenDDS {\n"
        "  @annotation no_init_before_deserialize {\n"
        "  };\n"
        "};\n";
    }
  };

  namespace internal {
    /**
     * Types with this annotation will not get a DynamicDataAdapterImpl generated
     * for them. Attempting to access struct or union members with this
     * annotation on their type will result in an UNSUPPORTED retcode.
     * get_dynamic_data_adapter for these types will be generated, but will
     * return nullptr.
     */
    class NoDynamicDataAdapterAnnotation : public Annotation<NoDynamicDataAdapterAnnotation> {
    public:
      static const char* name() { return "no_dynamic_data_adapter"; }
      static const char* module() { return "OpenDDS::internal"; }

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
    };

    /**
     * Types with this annotation have a special serialization case in
     * marshal_generator.
     */
    class SpecialSerializationAnnotation :
      public AnnotationWithValue<std::string, SpecialSerializationAnnotation> {
    public:
      static const char* name() { return "special_serialization"; }
      static const char* module() { return "OpenDDS::internal"; }

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
