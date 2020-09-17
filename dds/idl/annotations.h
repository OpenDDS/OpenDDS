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

#include <ast_expression.h>

#include <ace/Basic_Types.h>

#include <vector>
#include <string>
#include <map>
#include <set>

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
                                                        const char* member_name);

bool get_bool_annotation_member_value(AST_Annotation_Appl* appl,
                                      const char* member_name,
                                      const bool* default_value);

ACE_UINT32 get_u32_annotation_member_value(AST_Annotation_Appl* appl,
                                           const char* member_name,
                                           const ACE_UINT32* default_value);

std::string get_str_annotation_member_value(AST_Annotation_Appl* appl,
                                            const char* member_name,
                                            const std::string* default_value);

/**
 * Annotation that logically provide a value when absent.
 */
template <typename T>
class AbsentValue {
public:
  AbsentValue(const T& value)
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
   * returns true.
   * If node does not have the annotation, it sets it to a default value and returns false.
   */
  virtual bool node_value_exists(AST_Decl* node, T& value) const
  {
    AST_Annotation_Appl* appl = find_on(node);
    if (!appl) { return false; }

    value = value_from_appl(appl, default_value());
    return true;
  }

protected:
  /* NOTE: Derived classes should either override value_from_appl or default_value. */

  virtual T value_from_appl(AST_Annotation_Appl*,
                            const T* default_value) const
  {
    return *default_value;
  }

  virtual const T* default_value() const
  {
    return 0;
  }
};

template<>
bool AnnotationWithValue<bool>::value_from_appl(AST_Annotation_Appl* appl,
                                                const bool* default_value) const;

template<>
unsigned AnnotationWithValue<ACE_UINT32>::value_from_appl(AST_Annotation_Appl* appl,
                                                          const ACE_UINT32* default_value) const;

template<>
std::string AnnotationWithValue<std::string>::value_from_appl(AST_Annotation_Appl* appl,
                                                              const std::string* default_value) const;

template <typename T>
class AnnotationWithEnumValue : public AnnotationWithValue<T> {
protected:
  T value_from_appl(AST_Annotation_Appl* appl,
                    const T* default_value) const
  {
    return static_cast<T>(get_u32_annotation_member_value(appl, "value", reinterpret_cast<const ACE_UINT32*>(default_value)));
  }
};

// @key ======================================================================

class KeyAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  KeyAnnotation()
    : AbsentValue(false)
  {}

  std::string definition() const;
  std::string name() const;

  bool union_value(AST_Union* node) const;

private:
  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
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
  TopicValue value_from_appl(AST_Annotation_Appl* appl, const TopicValue* default_value) const;
};

// @nested ===================================================================

class NestedAnnotation : public AnnotationWithValue<bool> {
public:
  std::string definition() const;
  std::string name() const;

private:
  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
};

// @default_nested ===========================================================

class DefaultNestedAnnotation : public AnnotationWithValue<bool> {
public:
  std::string definition() const;
  std::string name() const;

private:
  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
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
    : AbsentValue(autoidkind_sequential)
  {}

  std::string definition() const;
  std::string name() const;

private:
  virtual const AutoidKind* default_value() const
  {
    static AutoidKind v = autoidkind_hash;
    return &v;
  }
};

// @hashid ===================================================================

class HashidAnnotation : public AnnotationWithValue<std::string> {
public:
  std::string definition() const;
  std::string name() const;

private:
  virtual const std::string* default_value() const
  {
    static std::string v = "";
    return &v;
  }
};

// @optional ===================================================================

class OptionalAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  OptionalAnnotation()
    : AbsentValue(false)
  {}

  std::string definition() const;
  std::string name() const;

  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
};

// @must_understand ============================================================

class MustUnderstandAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  MustUnderstandAnnotation()
    : AbsentValue(false)
  {}

  std::string definition() const;
  std::string name() const;

  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
};

// @external ===================================================================

class ExternalAnnotation : public AnnotationWithValue<bool>, public AbsentValue<bool> {
public:
  ExternalAnnotation()
    : AbsentValue(false)
  {}

  std::string definition() const;
  std::string name() const;

  virtual const bool* default_value() const
  {
    static bool v = true;
    return &v;
  }
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

class TryConstructAnnotation : public AnnotationWithEnumValue<TryConstructFailAction>, public AbsentValue<TryConstructFailAction> {
public:
  TryConstructAnnotation()
    : AbsentValue(tryconstructfailaction_discard)
  {}

  std::string definition() const;
  std::string name() const;

  TryConstructFailAction union_value(AST_Union* node) const;

private:
  virtual const TryConstructFailAction* default_value() const
  {
    static TryConstructFailAction v = tryconstructfailaction_use_default;
    return &v;
  }
};

// OpenDDS Specific Annotations
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {

  // @OpenDDS::data_representation ===========================================

  struct DataRepresentation {
    bool xcdr1;
    bool xcdr2;
    bool xml;
    bool unaligned_cdr;

    DataRepresentation()
    {
      set_all(false);
    }

    void add(const DataRepresentation& other)
    {
      xcdr1 |= other.xcdr1;
      xcdr2 |= other.xcdr2;
      xml |= other.xml;
      unaligned_cdr |= other.unaligned_cdr;
    }

    void set_all(bool value)
    {
      xcdr1 = value;
      xcdr2 = value;
      xml = value;
      unaligned_cdr = value;
    }

    bool none() const
    {
      return !xcdr1 && !xcdr2 && !xml && !unaligned_cdr;
    }

    bool all() const
    {
      return xcdr1 && xcdr2 && xml && unaligned_cdr;
    }

    bool only_xcdr1() const
    {
      return xcdr1 && !xcdr2 && !xml && !unaligned_cdr;
    }

    bool not_only_xcdr1() const
    {
      return xcdr1 && (xcdr2 || xml || unaligned_cdr);
    }

    bool only_xcdr2() const
    {
      return !xcdr1 && xcdr2 && !xml && !unaligned_cdr;
    }

    bool not_only_xcdr2() const
    {
      return xcdr2 && (xcdr1 || xml || unaligned_cdr);
    }

    bool only_xml() const
    {
      return !xcdr1 && !xcdr2 && xml && !unaligned_cdr;
    }

    bool not_only_xml() const
    {
      return xml && (xcdr1 || xcdr2 || unaligned_cdr);
    }

    bool only_unaligned_cdr() const
    {
      return !xcdr1 && !xcdr2 && !xml && unaligned_cdr;
    }

    bool not_only_unaligned_cdr() const
    {
      return unaligned_cdr && (xcdr1 || xcdr2 || xml);
    }
  };

  /**
   * Used to specifiy the allowed RTPS data representations in XTypes.
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
    DataRepresentation value_from_appl(AST_Annotation_Appl* appl, const DataRepresentation* default_value) const;
  };
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
