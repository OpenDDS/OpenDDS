#include "idl_defines.h"
#ifdef TAO_IDL_HAS_ANNOTATIONS
#ifndef SAMPLE_KEYS_HEADER
#define SAMPLE_KEYS_HEADER

#include <string>
#include <iterator>
#include <list>
#include <exception>

class AST_Decl;
class AST_Structure;

/**
 * Find Keys in Samples
 *
 * Use like this:
 * AST_Structure* struct_node;
 * // ...
 * SampleKeys keys(struct_node);
 * SampleKeys::Iterator end = keys.end();
 * for (SampleKeys::Iterator i = keys.begin(); i != end; ++i) {
 *   AST_Decl* key = *i;
 *   // ...
 * }
 */
class SampleKeys {
public:
  /**
   * Error in the AST or the application of the @key annotation.
   */
  class Error : public std::exception {
  public:
    Error();
    Error(const Error& error);
    Error(const std::string& message);
    Error(AST_Decl* node, const std::string& message);

    Error& operator=(const Error& error);
    virtual const char* what() const noexcept;

  private:
    std::string message_;
  };

  /**
   * Iterator for traversing the TAO_IDL AST, looking for nodes within a sample
   * node that are annotated with @key.
   */
  class Iterator {
  public:
    typedef AST_Decl* value_type;
    typedef AST_Decl** pointer;
    typedef AST_Decl*& reference;
    typedef std::output_iterator_tag iterator_category;

    /**
     * Create new iterator equal to SampleKey::end()
     */
    Iterator();

    /**
     * Create new iterator pointing to the first sample key or equal to
     * SampleKey::end() if there are no keys.
     */
    Iterator(SampleKeys &parent);

    /**
     * Create completely separate copy of another iterator
     */
    Iterator(const Iterator& other);

    ~Iterator();

    Iterator& operator=(const Iterator& other);
    Iterator& operator++(); // Prefix
    Iterator operator++(int); // Postfix
    value_type operator*() const;
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

    /**
     * Get the path of the key in reference to the root_sample
     */
    std::string path();
    void cleanup();

  private:
    Iterator(AST_Structure* root_sample);

    size_t pos_;
    Iterator* child_;
    value_type current_value_;
    AST_Structure* root_sample_;

    void path_i(std::list<std::string>& name_stack);
  };

  SampleKeys(AST_Structure* root_sample);
  ~SampleKeys();

  AST_Structure* root_sample() const;

  Iterator begin();
  Iterator end();

  /**
   * Count the keys in the sample
   */
  size_t count();

private:
  AST_Structure* root_sample_;
  bool counted_;
  size_t count_;
};
#endif
#endif
