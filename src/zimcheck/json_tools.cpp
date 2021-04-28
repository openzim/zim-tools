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

#include "json_tools.h"

namespace JSON
{

OutputStream::OutputStream(std::ostream* out)
  : m_out(out)
{
}

const char* OutputStream::sep() const
{
  if ( m_nesting.empty() )
    return "";

  return m_nesting.top().hasData ? ",\n" : "";
}

std::string OutputStream::indentation() const
{
  return std::string(2*m_nesting.size(), ' ');
}

void OutputStream::output(bool b)
{
  *m_out << (b ? "true" : "false");
}

void OutputStream::output(const char* s)
{
  assert(!m_nesting.empty());
  m_nesting.top().hasData = true;
  auto& out = *m_out;
  out << '\'';
  for (const char* p = s; *p; ++p) {
    if (*p == '\'' || *p == '\\') {
      out << '\\' << *p;
    } else if ( *p == '\n' ) {
      out << "\\n";
    } else {
      out << *p;
    }
  }
  out << '\'';
}

void OutputStream::output(StartObject)
{
  if ( m_out ) {
    *m_out << "{\n" << std::flush;
  }
  m_nesting.push(ScopeInfo{OBJECT, false});
}

void OutputStream::output(EndObject)
{
  assert(!m_nesting.empty());
  assert(m_nesting.top().type == OBJECT);
  m_nesting.pop();
  if ( m_out ) {
    *m_out << "\n" << indentation() << "}" << std::flush;
  }
  if ( !m_nesting.empty() ) {
    m_nesting.top().hasData = true;
  } else {
    *m_out << std::endl;
  }
}

void OutputStream::output(StartArray)
{
  if ( m_out ) {
    *m_out << "[\n" << std::flush;
  }
  m_nesting.push(ScopeInfo{ARRAY, false});
}

OutputStream& OutputStream::operator<<(EndArray)
{
  assert(!m_nesting.empty());
  assert(m_nesting.top().type == ARRAY);
  const char* s = m_nesting.top().hasData ? "\n" : "";
  m_nesting.pop();
  if ( m_out ) {
    *m_out << s << indentation() << "]" << std::flush;
  }
  if ( !m_nesting.empty() ) {
    m_nesting.top().hasData = true;
  }
  return *this;
}

} // namespace JSON
