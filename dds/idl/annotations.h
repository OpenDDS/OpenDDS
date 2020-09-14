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

AST_Expression::AST_ExprValue* get_annotation_member_ev(
  AST_Annotation_Appl* appl, const char* member_name);

bool get_bool_annotation_member_value(
  AST_Annotation_Appl* appl, const char* member_name);

ACE_UINT32 get_u32_annotation_member_value(
  AST_Annotation_Appl* appl, const char* member_name);

/**
 * Annotation with a Single Member Named "value"
 */
template <typename T>
class AnnotationWithValue : public Annotation {
public:
  /**
   * Default value if the node DOESN'T have the annotation. This is different
   * than the default value in the annotation definition.
   */
  virtual T default_value() const
  {
    return T();
  }

  /**
   * If node has the annotation, this sets value to the annotation value and
   * returns true.
   * If node does not have the annotation, this sets value to default_value()
   * and returns false.
   */
  virtual bool node_value_exists(AST_Decl* node, T& value) const
  {
    AST_Annotation_Appl* appl = find_on(node);
    value = value_from_appl(appl);
    return appl;
  }

  /**
   * Returns the value according to the annotation if it exists, else returns
   * default_value().
   */
  virtual T node_value(AST_Decl* node) const
  {
    T value;
    return node_value_exists(node, value) ? value : default_value();
  }

protected:
  /**
   * Get value from an annotation application. Returns default_value if appl is
   * null. Must be specialized.
   */
  virtual T value_from_appl(AST_Annotation_Appl*) const {
    return T();
  }
};

template<>
bool AnnotationWithValue<bool>::value_from_appl(AST_Annotation_Appl* appl) const;

template<>
unsigned AnnotationWithValue<ACE_UINT32>::value_from_appl(
  AST_Annotation_Appl* appl) const;

template <typename T>
class AnnotationWithEnumValue : public AnnotationWithValue<T> {
protected:
  T value_from_appl(AST_Annotation_Appl* appl) const
  {
    return appl ? static_cast<T>(
      get_u32_annotation_member_value(appl, "value")) : this->default_value();
  }
};

// @key ======================================================================

class KeyAnnotation : public AnnotationWithValue<bool> {
public:
  std::string definition() const;
  std::string name() const;

  bool union_value(AST_Union* node) const;
};

// @topic ====================================================================

class TopicAnnotation : public AnnotationWithValue<bool> {
public:
  TopicAnnotation();

  std::string definition() const;
  std::string name() const;

  bool node_value(AST_Decl* node) const;

private:
  std::set<std::string> platforms_;

  bool value_from_appl(AST_Annotation_Appl* appl) const;
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

class AutoidAnnotation : public AnnotationWithEnumValue<AutoidKind> {
public:
  std::string definition() const;
  std::string name() const;

  AutoidKind default_value() const;
};

// @hashid ===================================================================

class HashidAnnotation : public AnnotationWithValue<std::string> {
public:
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

class TryConstructAnnotation : public AnnotationWithEnumValue<TryConstructFailAction> {
public:
  std::string definition() const;
  std::string name() const;
  virtual TryConstructFailAction default_value() const
  {
    return tryconstructfailaction_discard;
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

    DataRepresentation()
    {
      set_all(false);
    }

    void add(const DataRepresentation& other)
    {
      xcdr1 |= other.xcdr1;
      xcdr2 |= other.xcdr2;
      xml |= other.xml;
    }

    void set_all(bool value)
    {
      xcdr1 = value;
      xcdr2 = value;
      xml = value;
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
    DataRepresentation value_from_appl(AST_Annotation_Appl* appl) const;
  };
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
