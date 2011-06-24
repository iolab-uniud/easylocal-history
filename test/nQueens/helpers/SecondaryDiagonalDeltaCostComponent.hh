// $Id$
// This file is part of EasyLocalpp: a C++ Object-Oriented framework
// aimed at easing the development of Local Search algorithms.
// Copyright (C) 2001--2011 Andrea Schaerf, Luca Di Gaspero. 
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

#if !defined(_SECONDARYDIAGONALDELTACOSTCOMPONENT_HH_)
#define _SECONDARYDIAGONALDELTACOSTCOMPONENT_HH_

#include <helpers/DeltaCostComponent.hh>
#include <vector>
#include "../data/Swap.hh"
#include "SecondaryDiagonalCostComponent.hh"

class SecondaryDiagonalDeltaCostComponent : public FilledDeltaCostComponent<int, std::vector<int>, Swap>
{
public:
  SecondaryDiagonalDeltaCostComponent(const int& in, SecondaryDiagonalCostComponent& cc) 
    : FilledDeltaCostComponent<int, std::vector<int>, Swap>(in,cc,"Secondary diagonal")
{ }
	int ComputeDeltaCost(const std::vector<int>& st, const Swap& sw) const;
};

#endif /*_SECONDARYDIAGONALDELTACOSTCOMPONENT_HH_*/
