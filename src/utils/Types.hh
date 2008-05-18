// $Id$
// This file is part of EasyLocalpp: a C++ Object-Oriented framework
// aimed at easing the development of Local Search algorithms.
// Copyright (C) 2001--2008 Andrea Schaerf, Luca Di Gaspero. 
//
// EasyLocalpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// EasyLocalpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with EasyLocalpp. If not, see <http://www.gnu.org/licenses/>.

#if !defined(_TYPES_HH_)
#define _TYPES_HH_

#include <EasyLocal.conf.hh>
#if defined(HAVE_CONFIG_H)
#include <config.hh>
#endif

template <typename CFtype>
bool IsZero(CFtype value);

template <typename CFtype>
bool EqualTo(CFtype value1, CFtype value2);

template <typename CFtype>
bool LessThan(CFtype value1, CFtype value2);

template <typename CFtype>
bool LessOrEqualThan(CFtype value1, CFtype value2);

template <typename CFtype>
bool GreaterThan(CFtype value1, CFtype value2);

template <typename CFtype>
bool GreaterOrEqualThan(CFtype value1, CFtype value2);

#endif // !defined(_TYPES_HH_)
