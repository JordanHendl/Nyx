/*
 * Copyright (C) 2020 Jordan Hendl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Memory.h"

namespace nyx
{
    MemoryFlags::MemoryFlags()
  {
    this->bit = MemoryFlags::DeviceLocal ;
  }
  
  MemoryFlags::MemoryFlags( int flags )
  {
    this->set( flags ) ;
  }

  MemoryFlags& MemoryFlags::operator=( int flag )
  {
    this->set( flag ) ;
    
    return *this ;
  }
  
  MemoryFlags& MemoryFlags::operator|( int flag )
  {
    this->set( this->bit | flag ) ;
    
    return *this ;
  }
  
  MemoryFlags::operator int() const
  {
    return this->value() ;
  }
  
  int MemoryFlags::value() const
  {
    return this->bit ;
  }
  
  void MemoryFlags::set( int value )
  {
    this->bit = value ;
  }
}



