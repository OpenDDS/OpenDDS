/*
 * $Id$
 */

#ifndef ERL_UTILITY_H
#define ERL_UTILITY_H

#include <ostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ast_expression.h"
#include "utl_identifier.h"
#include "utl_string.h"

class erl_identifier
{
public:
  static const char* sep;

  erl_identifier(Identifier* name);
  erl_identifier(UTL_ScopedName* name);

  ~erl_identifier();

  std::string as_var() const;

  std::string str() const;

  operator std::string() const;

private:
  std::string str_;
  
  void init();
  
  friend std::ostream& operator<<(std::ostream& os, const erl_identifier& rhs);
};

class erl_identifier_list
{
public:
  typedef std::vector<erl_identifier>::iterator iterator;
  typedef std::vector<erl_identifier>::const_iterator const_iterator;

  erl_identifier_list();

  template <typename InputIterator>
  erl_identifier_list(InputIterator first, InputIterator last)
  {
    for (InputIterator it(first); it != last; ++it)
    {
      add((*it)->local_name());
    }
  }

  ~erl_identifier_list();

  void add(const erl_identifier& name);
  
  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  std::size_t size() const;

  std::string as_param_list() const;

  std::string as_init_list() const;

private:
  std::vector<erl_identifier> v_;

  friend std::ostream& operator<<(std::ostream& os, const erl_identifier_list& rhs);
};

class erl_literal
{
public:
  explicit erl_literal(AST_Expression* expr);

  ~erl_literal();

  std::string str() const;

  static std::string to_str(AST_Expression* expr);
  
  operator std::string() const;

private:
  std::string str_;
  
  friend std::ostream& operator<<(std::ostream& os, const erl_literal& rhs);
};

class erl_file
{
public:
  virtual ~erl_file();

  virtual std::string basename() = 0;

  virtual const char* filename() = 0;
  
  std::ostream& open_stream();

protected:
  std::ofstream os_;

  erl_file();

  virtual void write_header() = 0;
};

class erl_header : public erl_file
{
public:
  static const char* ext;

  explicit erl_header(UTL_ScopedName* name);

  ~erl_header();

  std::string basename();

  const char* filename();

  std::string guard();

protected:
  void write_header();
  
  void write_footer();

private:
  erl_identifier name_;
  
  std::string filename_;

  std::string guard_;
};

class erl_module : public erl_file
{
public:
  static const char* ext;

  explicit erl_module(UTL_ScopedName* name);

  ~erl_module();

  std::string basename();

  const char* filename();
 
  void add_export(const std::string& fn);

  template <typename T>
  void add_export(const T& name, int arity)
  {
    std::ostringstream os;

    os << name << "/" << arity;

    add_export(os.str());
  }

  template <class InputIterator>
  void add_exports(InputIterator first, InputIterator last, int arity)
  {
    for (InputIterator it(first); it != last; ++it)
    {
      add_export<erl_identifier>((*it)->local_name(), arity);
    }
  }

  void add_include(const std::string& file);

protected:
  void write_header();

private:
  erl_identifier name_;

  std::string filename_;

  std::vector<std::string> exports_;

  std::vector<std::string> includes_;
  
  friend std::ostream& operator<<(std::ostream& os, const erl_module& rhs);
};

std::string to_list(std::vector<std::string>& v);

#endif /* ERL_UTILITY_H */
