#ifndef itl_hpp
#define itl_hpp

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include "rapidjson/document.h"

namespace itl {

  class TypeVisitor;
  class Dictionary;

  class Type {
  public:
    virtual ~Type() { }
    virtual void accept(TypeVisitor& visitor) = 0;

    const rapidjson::Value* note() const { return note_; }

  protected:
    Type(const rapidjson::Value* note)
      : note_(note)
    { }

    const rapidjson::Value* note_;
  };

  class Alias : public Type {
  public:
    const std::string& name() const { return name_; }
    Type* type() const { return type_; }

  private:
    Alias(const rapidjson::Value* note, const std::string& name, Type* type)
      : Type(note)
      , name_(name)
      , type_(type)
    { }

    virtual void accept(TypeVisitor& visitor);

    std::string const name_;
    Type* const type_;
    friend class Dictionary;
  };

  struct ConstrainedType {
    typedef std::map<std::string, std::string> ValuesType;

    ConstrainedType (const ValuesType& v,
                     bool ic)
      : values(v)
      , isConstrained(ic)
    { }

    ValuesType values;
    bool isConstrained;
  };

  class Int : public Type, public ConstrainedType {
  public:
    unsigned int bits() const { return bits_; }
    bool isUnsigned() const { return isUnsigned_; }

  private:
    Int(const rapidjson::Value* note,
        unsigned int bits,
        bool isUnsigned,
        const ConstrainedType::ValuesType& values,
        bool isConstrained)
      : Type(note)
      , ConstrainedType(values, isConstrained)
      , bits_(bits)
      , isUnsigned_(isUnsigned)
    { }

    virtual void accept(TypeVisitor& visitor);

    unsigned int bits_;
    bool isUnsigned_;

    friend class Dictionary;
  };

  class Float : public Type {
  public:
    enum Model {
      None,
      Binary16,
      Binary32,
      Binary64,
      Binary128,
      Decimal32,
      Decimal64,
      Decimal128
    };

    Model model() const { return model_; }

  private:
    Float(const rapidjson::Value* note,
          Model model)
      : Type(note)
      , model_(model)
    { }

    virtual void accept(TypeVisitor& visitor);

    Model model_;

    friend class Dictionary;
  };

  class Fixed : public Type, public ConstrainedType {
  private:
    Fixed(const rapidjson::Value* note,
          unsigned int base,
          unsigned int digits,
          unsigned int scale,
          const ConstrainedType::ValuesType& values,
          bool isConstrained)
      : Type(note)
      , ConstrainedType(values, isConstrained)
      , base_ (base)
      , digits_ (digits)
      , scale_ (scale)
    { }

    virtual void accept(TypeVisitor& visitor);

    unsigned int base_;
    unsigned int digits_;
    unsigned int scale_;

    friend class Dictionary;
  };

  class Sequence : public Type {
  public:
    Type* element_type() const { return type_; }
    std::vector<unsigned int> size() const { return size_; }

  private:
    Sequence(const rapidjson::Value* note,
             Type* type,
             const std::vector<unsigned int>& size,
             unsigned int capacity)
      : Type(note)
      , type_(type)
      , size_(size)
      , capacity_(capacity)
    { }

    virtual void accept(TypeVisitor& visitor);

    Type* type_;
    std::vector<unsigned int> size_;
    unsigned int capacity_;

    friend class Dictionary;
  };

  class String : public Type, public ConstrainedType {
  private:
    String(const rapidjson::Value* note,
           const ConstrainedType::ValuesType& values,
           bool isConstrained)
      : Type(note)
      , ConstrainedType(values, isConstrained)
    { }

    virtual void accept(TypeVisitor& visitor);

    friend class Dictionary;
  };

  class Record : public Type {
  public:
    struct Field {
      std::string name;
      Type* type;
      bool optional;

      Field()
        : type(0)
        , optional(false)
      { }

      Field(const std::string& n, Type* t, bool o)
        : name(n), type(t), optional(o)
      { }
    };

    typedef std::vector<Field> FieldsType;
    typedef FieldsType::iterator iterator;
    typedef FieldsType::const_iterator const_iterator;

    iterator begin() { return fields_.begin(); }
    const_iterator begin() const { return fields_.begin(); }
    iterator end() { return fields_.end(); }
    const_iterator end() const { return fields_.end(); }

  private:
    Record(const rapidjson::Value* note, const FieldsType& fields)
      : Type(note)
      , fields_(fields)
    { }

    virtual void accept(TypeVisitor& visitor);

    FieldsType fields_;

    friend class Dictionary;
  };

  class Union : public Type {
  public:
    struct Field {
      typedef std::set<std::string> LabelsType;
      typedef LabelsType::iterator iterator;
      typedef LabelsType::const_iterator const_iterator;

      std::string name;
      Type* type;
      LabelsType labels;
      Field(const std::string& n, Type* t, const std::set<std::string> v)
        : name(n), type(t), labels(v)
      { }

      iterator begin() { return labels.begin(); }
      const_iterator begin() const { return labels.begin(); }
      iterator end() { return labels.end(); }
      const_iterator end() const { return labels.end(); }
    };

    typedef std::vector<Field> FieldsType;
    typedef FieldsType::iterator iterator;
    typedef FieldsType::const_iterator const_iterator;

    Type* discriminator() const { return discriminator_; }

    iterator begin() { return fields_.begin(); }
    const_iterator begin() const { return fields_.begin(); }
    iterator end() { return fields_.end(); }
    const_iterator end() const { return fields_.end(); }

  private:
    Union(const rapidjson::Value* note,
          Type* discriminator,
          const FieldsType& fields)
      : Type(note)
      , discriminator_(discriminator)
      , fields_(fields)
    { }

    virtual void accept(TypeVisitor& visitor);

    Type* discriminator_;
    std::vector<Field> fields_;

    friend class Dictionary;
  };

  class TypeRef : public Type {
  public:
    Type* type() const { return type_; }

  private:
    TypeRef(const rapidjson::Value* note, const std::string& name)
      : Type(note)
      , name_(name)
      , type_(NULL)
    { }

    virtual void accept(TypeVisitor& visitor);

    std::string name_;
    Type* type_;

    friend class Dictionary;
  };

  class TypeVisitor {
  public:
    virtual ~TypeVisitor() { }
    virtual void visit (Int&) = 0;
    virtual void visit (Float&) = 0;
    virtual void visit (Fixed&) = 0;
    virtual void visit (String&) = 0;

    virtual void visit (Sequence&) = 0;
    virtual void visit (Record&) = 0;
    virtual void visit (Union&) = 0;
    virtual void visit (TypeRef&) = 0;
    virtual void visit (Alias&) = 0;
  };

  inline void Alias::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Int::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Float::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Fixed::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Sequence::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void String::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Record::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void Union::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  inline void TypeRef::accept(TypeVisitor& visitor) {
    visitor.visit (*this);
  }

  // From https://github.com/miloyip/rapidjson/blob/a2354cd7455e45486ebd4c43513011ce5b1cc8a7/doc/stream.md
  class IStreamWrapper {
  public:
    typedef char Ch;

    IStreamWrapper(std::istream& is) : is_(is) {
    }

    Ch Peek() const { // 1
      int c = is_.peek();
      return c == std::char_traits<char>::eof() ? '\0' : (Ch)c;
    }

    Ch Take() { // 2
      int c = is_.get();
      return c == std::char_traits<char>::eof() ? '\0' : (Ch)c;
    }

    size_t Tell() const { return (size_t)is_.tellg(); } // 3

    Ch* PutBegin() { assert(false); return 0; }
    void Put(Ch) { assert(false); }
    void Flush() { assert(false); }
    size_t PutEnd(Ch*) { assert(false); return 0; }

  private:
    IStreamWrapper(const IStreamWrapper&);
    IStreamWrapper& operator=(const IStreamWrapper&);

    std::istream& is_;
  };

  class Dictionary {
  public:
    typedef std::map<std::string, Alias*> AliasesType;
    typedef AliasesType::iterator iterator;
    typedef AliasesType::const_iterator const_iterator;

    iterator begin() { return aliases_.begin(); }
    const_iterator begin() const { return aliases_.begin(); }

    iterator end() { return aliases_.end(); }
    const_iterator end() const { return aliases_.end(); }

    void fromJson(std::istream& in) {
      clear();
      IStreamWrapper wrapper(in);
      document_.ParseStream(wrapper);
      parseRoot(document_);

      struct visitor : public TypeVisitor {
        Dictionary& dictionary;

        visitor (Dictionary& d)
          : dictionary(d)
        { }

        void visit (Int&) { }
        void visit (Float&) { }
        void visit (Fixed&) { }
        void visit (String&) { }
        void visit (Sequence&) { }
        void visit (Record&) { }
        void visit (Union&) { }
        void visit (TypeRef& tr) {
          AliasesType::const_iterator pos = dictionary.aliases_.find(tr.name_);
          if (pos != dictionary.aliases_.end()) {
            tr.type_ = pos->second;
          }
          else {
            throw std::runtime_error(tr.name_ + " is not defined");
          }
        }

        void visit (Alias&) { }
      };

      visitor v(*this);
      for (AllTypes::const_iterator pos = allTypes_.begin(), limit = allTypes_.end();
           pos != limit;
           ++pos) {
        (*pos)->accept(v);
      }
    }

    void clear() {
      aliases_.clear();
      for (AllTypes::const_iterator pos = allTypes_.begin(), limit = allTypes_.end();
           pos != limit;
           ++pos) {
        delete *pos;
      }
      allTypes_.clear();
      // Cleared after types since types refer to storage in document_.
      document_.SetNull();
    }

    rapidjson::Document::AllocatorType& allocator() { return document_.GetAllocator(); }

  private:
    rapidjson::Document document_;

    AliasesType aliases_;
    typedef std::vector<Type*> AllTypes;
    AllTypes allTypes_;

    template<typename T>
    T* registerType(T* type) {
      allTypes_.push_back(type);
      return type;
    }

    bool extractString(const char* name, const rapidjson::Value& value, std::string& out) {
      if (value.IsObject() && value.HasMember(name)) {
        const rapidjson::Value& n = value[name];
        if (n.IsString()) {
          out = n.GetString();
          return true;
        }
        else {
          throw std::runtime_error("expected string");
        }
      }
      return false;
    }

    bool extractUint(const char* name, const rapidjson::Value& value, unsigned int& out) {
      if (value.IsObject() && value.HasMember(name)) {
        const rapidjson::Value& n = value[name];
        if (n.IsUint()) {
          out = n.GetUint();
          return true;
        }
        else {
          throw std::runtime_error("expected unsigned integer");
        }
      }
      return false;
    }

    bool extractBool(const char* name, const rapidjson::Value& value, bool& out) {
      if (value.IsObject() && value.HasMember(name)) {
        const rapidjson::Value& n = value[name];
        if (n.IsBool()) {
          out = n.GetBool();
          return true;
        }
        else {
          throw std::runtime_error("expected boolean");
        }
      }
      return false;
    }

    bool extractValue(const char* name, const rapidjson::Value& value, const rapidjson::Value*& v) {
      if (value.IsObject() && value.HasMember(name)) {
        v = &value[name];
        return true;
      }
      return false;
    }

    Type* extractType(const char* name, const rapidjson::Value& value) {
      if (value.IsObject() && value.HasMember(name)) {
        const rapidjson::Value& b = value[name];

        if (b.IsString()) {
          return registerType(new TypeRef(NULL, b.GetString()));
        }
        else if (b.IsObject()) {
          return parseTypeDef(b);
        }
      }

      throw std::runtime_error("expected type definition or reference");
    }

    bool extractValues(const char* name, const rapidjson::Value& value, std::map<std::string, std::string>& v) {
      if (value.IsObject() && value.HasMember(name)) {
        const rapidjson::Value& b = value[name];
        if (!b.IsObject()) throw std::runtime_error("values is not an object");
        for (rapidjson::Value::ConstMemberIterator pos = b.MemberBegin(), limit = b.MemberEnd();
             pos != limit;
             ++pos) {
          if (!pos->value.IsString()) throw std::runtime_error("value is not a string");
          v[pos->name.GetString()] = pos->value.GetString();
        }
        return true;
      }
      return false;
    }

    Alias* parseAlias(const rapidjson::Value& value) {
      std::string name;
      Type* type;
      const rapidjson::Value* note = NULL;
      if (!extractString("name", value, name)) throw std::runtime_error("no name for alias");
      if (name.empty()) throw std::runtime_error("name for alias must not be empty");
      type = extractType("type", value);
      extractValue("note", value, note);
      Alias* a = registerType(new Alias(note, name, type));
      if (aliases_.find(name) != aliases_.end()) throw std::runtime_error("duplicate alias: " + name);
      aliases_[name] = a;
      return a;
    }

    Int* parseInt(const rapidjson::Value& value) {
      unsigned int bits = 0;
      bool isUnsigned = false;
      const rapidjson::Value* note = NULL;
      std::map<std::string, std::string> values;
      bool isConstrained = false;
      extractUint("bits", value, bits);
      extractBool("unsigned", value, isUnsigned);
      extractValue("note", value, note);
      extractValues("values", value, values);
      extractBool("constrained", value, isConstrained);
      return registerType(new Int(note, bits, isUnsigned, values, isConstrained));
    }

    Float* parseFloat(const rapidjson::Value& value) {
      Float::Model model = Float::None;
      std::string modelString;
      const rapidjson::Value* note = NULL;
      extractString("model", value, modelString);

      if (modelString == "") {
        model = Float::None;
      }
      else if (modelString == "binary16") {
        model = Float::Binary16;
      }
      else if (modelString == "binary32") {
        model = Float::Binary32;
      }
      else if (modelString == "binary64") {
        model = Float::Binary64;
      }
      else if (modelString == "binary128") {
        model = Float::Binary128;
      }
      else if (modelString == "decimal32") {
        model = Float::Decimal32;
      }
      else if (modelString == "decimal64") {
        model = Float::Decimal64;
      }
      else if (modelString == "decimal128") {
        model = Float::Decimal128;
      }
      else {
        throw std::runtime_error("Unknown float model: " + modelString);
      }
      extractValue("note", value, note);

      return registerType(new Float(note, model));
    }

    Fixed* parseFixed(const rapidjson::Value& value) {
      unsigned int base = 0;
      unsigned int digits = 0;
      unsigned int scale = 0;
      const rapidjson::Value* note = NULL;
      std::map<std::string, std::string> values;
      bool isConstrained = false;

      if (!extractUint("base", value, base)) throw std::runtime_error("No base for fixed");
      if (!extractUint("digits", value, digits)) throw std::runtime_error("No digits for fixed");
      if (!extractUint("scale", value, scale)) throw std::runtime_error("No scale for fixed");

      if (base == 0) {
        throw std::runtime_error("Illegal base for fixed");
      }

      if (scale > digits) {
        throw std::runtime_error("Scale exceeds digits in fixed");
      }
      extractValue("note", value, note);
      extractValues("values", value, values);
      extractBool("constrained", value, isConstrained);

      return registerType(new Fixed(note, base, digits, scale, values, isConstrained));
    }

    Sequence* parseSequence(const rapidjson::Value& value) {
      Type* type = extractType("type", value);
      std::vector<unsigned int> size;
      unsigned int capacity = 0;
      const rapidjson::Value* note = NULL;

      if (value.HasMember("size")) {
        const rapidjson::Value& s = value["size"];
        if (s.IsUint()) {
          size.push_back(s.GetUint());
        }
        else if (s.IsArray()) {
          for (rapidjson::Value::ConstValueIterator pos = s.Begin(), limit = s.End();
               pos != limit;
               ++pos) {
            if (pos->IsUint()) {
              size.push_back(pos->GetUint());
            }
            else {
              throw std::runtime_error("sequence size is not integer or array");
            }
          }
        }
        else {
          throw std::runtime_error("sequence size is not integer or array");
        }
      }

      extractUint("capacity", value, capacity);
      extractValue("note", value, note);

      return registerType(new Sequence(note, type, size, capacity));
    }

    String* parseString(const rapidjson::Value& value) {
      const rapidjson::Value* note = NULL;
      std::map<std::string, std::string> values;
      bool isConstrained = false;
      extractValue("note", value, note);
      extractValues("values", value, values);
      extractBool("constrained", value, isConstrained);
      return registerType(new String(note, values, isConstrained));
    }

    Record* parseRecord(const rapidjson::Value& value) {
      if (!value.HasMember("fields")) throw std::runtime_error("record has no fields");
      const rapidjson::Value& s = value["fields"];
      if (!s.IsArray()) throw std::runtime_error("fields is not an array");
      Record::FieldsType fields;
      for (rapidjson::Value::ConstValueIterator pos = s.Begin(), limit = s.End();
           pos != limit;
           ++pos) {
        std::string name;
        if (!extractString("name", *pos, name)) throw std::runtime_error("record field has no name");
        Type* type = extractType("type", *pos);
        bool optional = false;
        extractBool("optional", *pos, optional);
        fields.push_back(Record::Field(name, type, optional));
      }
      const rapidjson::Value* note = NULL;
      extractValue("note", value, note);
      return registerType(new Record(note, fields));
    }

    Union* parseUnion(const rapidjson::Value& value) {
      Type* discriminator = extractType("discriminator", value);
      if (!value.HasMember("fields")) throw std::runtime_error("union has no fields");
      const rapidjson::Value& s = value["fields"];
      if (!s.IsArray()) throw std::runtime_error("union fields is not an array");
      Union::FieldsType fields;
      for (rapidjson::Value::ConstValueIterator pos = s.Begin(), limit = s.End();
           pos != limit;
           ++pos) {
        std::string name;
        if (!extractString("name", *pos, name)) throw std::runtime_error("union field has no name");
        Type* type = extractType("type", *pos);
        std::set<std::string> labels;
        if (!(*pos).HasMember("labels")) throw std::runtime_error("union field has no labels");
        const rapidjson::Value& dv = (*pos)["labels"];
        for (rapidjson::Value::ConstValueIterator pos = dv.Begin(), limit = dv.End();
             pos != limit;
             ++pos) {
          if (!pos->IsString()) throw std::runtime_error("union field label is not a string");
          labels.insert (pos->GetString());
        }
        fields.push_back(Union::Field(name, type, labels));
      }
      const rapidjson::Value* note = NULL;
      extractValue("note", value, note);
      return registerType(new Union(note, discriminator, fields));
    }

    Type* parseTypeDef(const rapidjson::Value& value) {
      std::string kind;
      if (!extractString("kind", value, kind)) throw std::runtime_error("type definition has no kind");

      if (kind == "alias") {
        return parseAlias(value);
      }
      else if (kind == "int") {
        return parseInt(value);
      }
      else if (kind == "float") {
        return parseFloat(value);
      }
      else if (kind == "fixed") {
        return parseFixed(value);
      }
      else if (kind == "sequence") {
        return parseSequence(value);
      }
      else if (kind == "string") {
        return parseString(value);
      }
      else if (kind == "record") {
        return parseRecord(value);
      }
      else if (kind == "union") {
        return parseUnion(value);
      }
      throw std::runtime_error("unknown kind in type definition: " + kind);
    }

    void parseTypes(const rapidjson::Value& value) {
      if (!value.IsArray()) throw std::runtime_error("types is not an array");
      for (rapidjson::Value::ConstValueIterator pos = value.Begin(), limit = value.End();
           pos != limit;
           ++pos) {
        parseTypeDef(*pos);
      }
    }

    void parseRoot(const rapidjson::Value& value) {
      if (!value.IsObject()) throw std::runtime_error("Expected object at top level");
      if (!value.HasMember("types")) throw std::runtime_error("No types");
      parseTypes(value["types"]);
    }
  };

  inline void toJson(const Dictionary& /*dictionary*/, std::ostream& /*out*/) {

  }

}

#endif /* itl_hpp */
