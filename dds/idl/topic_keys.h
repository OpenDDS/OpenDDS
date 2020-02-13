#ifndef TOPIC_KEYS_HEADER
#define TOPIC_KEYS_HEADER

#include <string>
#include <iterator>
#include <exception>
#include <sstream>
#include <vector>

class AST_Decl;
class AST_Type;
class AST_Structure;
class AST_Field;
class AST_Union;

/**
 * Find Keys in Topic Types
 *
 * Use like this:
 * AST_Structure* struct_node;
 * // ...
 * TopicKeys keys(struct_node);
 * TopicKeys::Iterator end = keys.end();
 * for (TopicKeys::Iterator i = keys.begin(); i != end; ++i) {
 *   AST_Decl* key = *i;
 *   // ...
 * }
 *
 * The key AST_Decl will be a different type of node depending on the key:
 * - For struct field keys this will be the AST_Field
 * - For Array keys this will be the base AST_Type, repeated for each element
 * - For Union keys this will be the AST_Union
 */
class TopicKeys {
public:
  enum RootType {
    PrimitiveType,
    StructureType,
    ArrayType,
    UnionType,
    InvalidType
  };

  /**
   * Get the RootType of the AST_Type
   */
  static RootType root_type(AST_Type* type);

  /**
   * Error in the AST or the application of the @key annotation.
   */
  class Error : public std::exception {
  public:
    Error();
    Error(AST_Decl* node, const std::string& message);
    virtual ~Error() throw ();

    Error& operator=(const Error& error);
    virtual const char* what() const throw();
    AST_Decl* node();

  private:
    AST_Decl* node_;
    std::string message_;
  };

  /**
   * Iterator for traversing the TAO_IDL AST, looking for nodes within a topic
   * type node that are annotated with @key.
   */
  class Iterator {
  public:
    /**
     * Standard Iterator Type Declarations
     */
    ///{
    typedef AST_Decl* value_type;
    typedef AST_Decl** pointer;
    typedef AST_Decl*& reference;
    typedef std::input_iterator_tag iterator_category;
    ///}

    /**
     * Create new iterator equal to TopicKeys::end()
     */
    Iterator();

    /**
     * Create new iterator pointing to the first topic key or equal to
     * TopicKeys::end() if there are no keys.
     */
    Iterator(TopicKeys &parent);

    /**
     * Create a completely separate copy of another iterator
     */
    Iterator(const Iterator& other);

    ~Iterator();

    Iterator& operator=(const Iterator& other);
    Iterator& operator++(); // Prefix
    Iterator operator++(int); // Postfix
    AST_Decl* operator*() const;
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

    /**
     * Get the path of the key in reference to the root_
     */
    std::string path();

    /**
     * Get the level of recursion
     */
    size_t level() const;

    /**
     * Get the root type of the final child
     */
    RootType root_type() const;

    /**
     * Get the final child's parent's root type
     *
     * Returns InvalidType if it is the root or a invalid iterator
     */
    RootType parents_root_type() const;

    /**
     * Get the AST_Type of the current value
     *
     * Returns 0 if the iterator is invalid
     */
    AST_Type* get_ast_type() const;

    static Iterator end_value();

  private:
    Iterator(AST_Type* root, Iterator* parent);
    Iterator(AST_Field* root, Iterator* parent);

    Iterator* parent_;
    unsigned pos_;
    Iterator* child_;
    /// Current value of the entire iterator stack
    AST_Decl* current_value_;
    AST_Decl* root_;
    RootType root_type_;
    size_t level_;
    bool recursive_;
    /// The Dimensions of the Array
    std::vector<size_t> dimensions_;
    /// Element Count in the Array
    size_t element_count_;

    /// Used in struct field key iteration
    bool implied_keys_;

    /**
     * Internal Recursive Impl. of path()
     */
    void path_i(std::stringstream& ss);

    void cleanup();
  };

  TopicKeys();
  TopicKeys(const TopicKeys& other);
  /**
   * If recursive is false, do a shallow iteration.
   */
  TopicKeys(AST_Structure* root, bool recursive = true);
  TopicKeys(AST_Union* root);
  ~TopicKeys();

  TopicKeys& operator=(const TopicKeys& other);
  AST_Decl* root() const;
  RootType root_type() const;

  Iterator begin();
  Iterator end();

  /**
   * Count the keys in the topic type
   */
  size_t count();

  bool recursive() const;

private:
  AST_Decl* root_;
  RootType root_type_;

  /// Cached Key Count
  ///{
  bool counted_;
  size_t count_;
  ///}

  /// Have iterators recurse into structures
  bool recursive_;
};

#endif
