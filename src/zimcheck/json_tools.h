/*
 * Copyright (C) 2021 Veloman Yunkan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _ZIM_JSON_TOOLS_H_
#define _ZIM_JSON_TOOLS_H_

#include <iostream>
#include <stack>
#include <cassert>

namespace JSON
{

template<class T>
struct Property
{
  const std::string key;
  const T value;
};

template<class T>
Property<T> property(const std::string& key, const T& value)
{
  return {key, value};
}

enum StartObject { startObject };
enum EndObject   { endObject   };
enum StartArray  { startArray  };
enum EndArray    { endArray    };

class OutputStream
{
public: // functions
  explicit OutputStream(std::ostream* out);

  bool enabled() const { return m_out != nullptr; }

  template<class T> OutputStream& operator<<(const T& t);

  // This operation is implemented as an overload of operator<<()
  // rather than an overload of function output() in order to suppress
  // the output of the array element separator after the last element.
  OutputStream& operator<<(EndArray);

private: // functions
  const char* sep() const;
  std::string indentation() const;

  void output(bool b);
  void output(const char* s);
  void output(const std::string& s) { output(s.c_str()); }

  void output(StartObject);
  void output(EndObject);
  void output(StartArray);

  template<class T> void output(const T& t) { *m_out << t; }
  template<class T> void output(const Property<T>& p);

private: // types
  enum ScopeType { OBJECT, ARRAY };
  struct ScopeInfo
  {
    ScopeType type;
    bool hasData;
  };

private: // data
  std::ostream* const m_out;
  std::stack<ScopeInfo> m_nesting;
};

template<class T>
OutputStream& OutputStream::operator<<(const T& t)
{
  if ( m_out ) {
    if ( !m_nesting.empty() && m_nesting.top().type == ARRAY )
      *m_out << sep() << indentation();
    output(t);
  }
  return *this;
}

template<class T>
void OutputStream::output(const Property<T>& p)
{
    *m_out << sep() << indentation();
    m_nesting.top().hasData = true;
    *this  << p.key;
    *m_out << " : ";
    *this  << p.value;
    *m_out << std::flush;
}

} // namespace JSON

#endif // _ZIM_JSON_TOOLS_H_
