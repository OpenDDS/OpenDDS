#ifndef OPENDDS_IDL_ANNOTATIONS_HEADER
#define OPENDDS_IDL_ANNOTATIONS_HEADER

#include <string>
#include <map>

class AST_Decl;
class AST_Union;
class AST_Annotation_Decl;
class AST_Annotation_Appl;
class BuiltinAnnotation;

class BuiltinAnnotations {
public:
  BuiltinAnnotations();
  ~BuiltinAnnotations();

  void register_all();
  BuiltinAnnotation* operator[](const std::string& annotation);

  template<typename T> inline void
  register_one()
  {
    T* annotation = new T;
    map_[annotation->fullname()] = annotation;
    annotation->cache();
  }

private:
  typedef std::map<std::string, BuiltinAnnotation*> MapType;
  MapType map_;
};

/**
 * Wrapper for Annotations
 */
class BuiltinAnnotation {
public:
  BuiltinAnnotation();
  virtual ~BuiltinAnnotation();

  virtual std::string definition() const = 0;
  virtual std::string name() const = 0;
  virtual std::string fullname() const;

  AST_Annotation_Decl* declaration() const;
  AST_Annotation_Appl* find_on(AST_Decl* node) const;
  void cache();

private:
  AST_Annotation_Decl* declaration_;
};

/**
 * Annotation with a Single Boolean Member Named "value"
 */
class BuiltinAnnotationWithBoolValue : public BuiltinAnnotation {
public:
  BuiltinAnnotationWithBoolValue();
  virtual ~BuiltinAnnotationWithBoolValue();

  /**
   * Default value if the node DOESN'T have the annotation. This is different
   * than the default value in the annotation definition.
   */
  virtual bool default_value() const;

  /**
   * Returns the value according to the annotation if it exists, else returns
   * default_value().
   */
  virtual bool node_value(AST_Decl* node) const;

  /**
   * If node has the annotation, this sets value to the annotation value and
   * returns true.
   * If node does not have the annotation, this sets value to default_value()
   * and returns false.
   */
  virtual bool node_value_exists(AST_Decl* node, bool& value) const;

protected:
  bool value_from_appl(AST_Annotation_Appl* appl) const;
};

/// Wrapper for @key
class KeyAnnotation : public BuiltinAnnotationWithBoolValue {
public:
  KeyAnnotation();
  virtual ~KeyAnnotation();

  virtual std::string definition() const;
  virtual std::string name() const;

  virtual bool union_value(AST_Union* node) const;
};

/// Wrapper for @topic
class TopicAnnotation : public BuiltinAnnotation {
public:
  TopicAnnotation();
  virtual ~TopicAnnotation();

  virtual std::string definition() const;
  virtual std::string name() const;
};

/// Wrapper for @nested
class NestedAnnotation : public BuiltinAnnotationWithBoolValue {
public:
  NestedAnnotation();
  virtual ~NestedAnnotation();

  virtual std::string definition() const;
  virtual std::string name() const;
};

/// Wrapper for @default_nested
class DefaultNestedAnnotation : public BuiltinAnnotationWithBoolValue {
public:
  DefaultNestedAnnotation();
  virtual ~DefaultNestedAnnotation();

  virtual std::string definition() const;
  virtual std::string name() const;
};

#endif
