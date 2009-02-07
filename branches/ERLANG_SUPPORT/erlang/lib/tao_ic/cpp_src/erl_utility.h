/*
 * $Id$
 */

#ifndef TAO_IC_ERL_UTILITY_H
#define TAO_IC_ERL_UTILITY_H

#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "ast_expression.h"
#include "ast_decl.h"
#include "utl_identifier.h"
#include "utl_string.h"

class erl_identifier
{
public:
  static const char* sep;

  erl_identifier(const char* name);
  erl_identifier(Identifier* name);
  erl_identifier(UTL_ScopedName* name);

  ~erl_identifier();

  std::string as_var() const;

  std::string str() const;

  operator std::string() const;

private:
  void init();
  
  std::string str_;
  
  friend std::ostream& operator<<(std::ostream& os, const erl_identifier& e);
};

class erl_identifier_list
{
public:
  typedef std::vector<erl_identifier>::iterator iterator;
  typedef std::vector<erl_identifier>::const_iterator const_iterator;

  erl_identifier_list();

  ~erl_identifier_list();

  void add(const erl_identifier& name);
  
  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  size_t size() const;

  std::string as_param_list() const;

  std::string as_init_list() const;

private:
  std::vector<erl_identifier> v_;

  friend std::ostream& operator<<(std::ostream& os, const erl_identifier_list& e);
};

class erl_literal
{
public:
  explicit erl_literal(AST_Expression* e);

  ~erl_literal();

  std::string str() const;

  operator std::string() const;

private:
  std::string str_;
  
  friend std::ostream& operator<<(std::ostream& os, const erl_literal& e);
};

class erl_file
{
public:
  virtual ~erl_file();

  virtual std::string basename() = 0;

  virtual const char* filename() = 0;
  
  std::ostream& open_stream();

protected:
  erl_file();

  virtual void write_header() = 0;
  
  std::ofstream os_;
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
 
  void add_export(const std::string& function);
  void add_export(const erl_identifier& name, int arity); 

  void add_include(const std::string& file);

protected:
  void write_header();

private:
  erl_identifier name_;

  std::string filename_;

  std::vector<std::string> exports_;

  std::vector<std::string> includes_;
  
  friend std::ostream& operator<<(std::ostream& os, const erl_module& e);
};

std::string to_list(std::vector<std::string>& v);

#endif /* TAO_IC_ERL_UTILITY_H */
