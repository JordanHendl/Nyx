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

/* 
 * File:   Event.cpp
 * Author: Jordan Hendl
 * 
 * Created on January 5, 2021, 8:18 PM
 */

#include "Event.h"
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <tuple>

namespace nyx
{
  const char* toString( const Event& event )
  {
    switch( event.type() )
    {
      case Event::Type::MouseButtonDown :
      case Event::Type::MouseButtonUp   :
      case Event::Type::MouseWheelDown  :
      case Event::Type::MouseWheelUp    : return toString( event.type() ) ; 
      case Event::Type::KeyDown         :
      case Event::Type::KeyUp           : return ( std::string( toString( event.type() ) ) + std::string( toString( event.key() ) ) ).c_str() ;
      default : return "Unknown" ;
    }
  }
  
  const char* toString( const Event::Type& type )
  {
    switch( type )
    {
      case Event::Type::KeyDown         : return "Key Down"          ;
      case Event::Type::KeyUp           : return "Key Up"            ;
      case Event::Type::MouseButtonDown : return "Mouse Button Down" ;
      case Event::Type::MouseButtonUp   : return "Mouse Button Up"   ;
      case Event::Type::MouseWheelUp    : return "Mouse Wheel Up"    ;
      case Event::Type::MouseWheelDown  : return "Mouse Wheel Down"  ;
      default : return "Unknown" ;
    }
  }
  const char* toString( const Key& key )
  {
    switch( key )
    {
      case Key::A : return "A" ;
      case Key::B : return "B" ;
      case Key::C : return "C" ;
      case Key::D : return "D" ;
      case Key::E : return "E" ;
      case Key::F : return "F" ;
      case Key::G : return "G" ;
      case Key::H : return "H" ;
      case Key::I : return "I" ;
      case Key::J : return "J" ;
      default : return "Unknown" ;
    }
  }
  
  const char* toString( const MouseButton& button )
  {
    switch( button )
    {
      case MouseButton::LeftClick   : return "Left Click"       ;
      case MouseButton::RightClick  : return "Right Click"      ;
      case MouseButton::MiddleClick : return "Middle Click"     ;
      case MouseButton::WheelUp     : return "Mouse Wheel Up"   ;
      case MouseButton::WheelDown   : return "Mouse Wheel Down" ;
      default : return "Unknown" ;
    }
  }
  
  struct StaticEventManagerData
  {
    typedef std::multimap<nyx::Key        , std::tuple<std::string, EventManager::Subscriber*>> KeyCallbackMap  ; ///< TODO
    typedef std::multimap<nyx::Event::Type, std::tuple<std::string, EventManager::Subscriber*>> TypeCallbackMap ; ///< TODO
    typedef std::multimap<std::string,                              EventManager::Subscriber*>  CallbackMap     ; ///< TODO
    
    
    CallbackMap     callbacks      ; ///< TODO
    KeyCallbackMap  key_callbacks  ; ///< TODO
    TypeCallbackMap type_callbacks ; ///< TODO
    
    float mouse_xpos   = 0.0f ;
    float mouse_ypos   = 0.0f ;
    float mouse_xdelta = 0.0f ;
    float mouse_ydelta = 0.0f ;
  };
  
  struct EventManagerData
  {
    std::vector<StaticEventManagerData::CallbackMap::iterator    > local_callbacks      ; ///< TODO
    std::vector<StaticEventManagerData::KeyCallbackMap::iterator > local_key_callbacks  ; ///< TODO
    std::vector<StaticEventManagerData::TypeCallbackMap::iterator> local_type_callbacks ; ///< TODO
  };


  static StaticEventManagerData man_data ; ///< TODO

  Event makeKeyEvent( Event::Type type, nyx::Key key )
  {
    Event event ;
    
    event.event_type = type ;
    event.event_key  = key  ;
    return event ;
  }
  
  Event makeMouseButtonEvent( Event::Type type, nyx::MouseButton button )
  {
    Event event ;
    event.event_type   = type   ;
    event.event_button = button ;
    
    return event ;
  }
  
  Event::Event()
  {
    this->event_type   = Event::Type::None ;
    this->event_button = MouseButton::None ;
    this->event_key    = Key::None         ;
  }
  
  Event::Event( const Event& event )
  {
    this->event_key  = event.event_key  ;
    this->event_type = event.event_type ;
  }
  
  nyx::MouseButton Event::button() const
  {
    return this->event_button ;
  }

  Event::Type Event::type() const
  {
    return this->event_type ;
  }
  
  Key Event::key() const
  {
    return this->event_key ;
  }
  
  EventManager::FunctionSubscriber::FunctionSubscriber( Callback callback )
  {
    this->callback = callback ;
  }
  
  void EventManager::FunctionSubscriber::execute( const Event& event )
  {
    ( this->callback )( event ) ;
  }

  EventManager::EventManager()
  {
    this->manager_data = new EventManagerData() ;
  }
  
  EventManager::~EventManager()
  {
    this->reset() ;
    delete this->manager_data ;
  }

  void EventManager::pushEvent( const Event& event )
  {
    for( auto cb : man_data.callbacks )
    {
      ( cb.second->execute( event ) ) ;
    }
    
    for( auto cb = man_data.key_callbacks.lower_bound( event.key() ); cb != man_data.key_callbacks.upper_bound( event.key() ); ++cb )
    {
      std::get<1>( cb->second )->execute( event ) ;
    }
    
    for( auto cb = man_data.type_callbacks.lower_bound( event.type() ); cb != man_data.type_callbacks.upper_bound( event.type() ); ++cb )
    {
      std::get<1>( cb->second )->execute( event ) ;
    }
  }
  
  void EventManager::enroll( EventCallback callback, nyx::Event::Type type, const char* key )
  {
    typedef EventManager::FunctionSubscriber Callback ;
    
    Callback *cb ;
    
    cb = new Callback( reinterpret_cast<Callback::Callback>( callback ) ) ; 
    auto iter = man_data.type_callbacks.find( type ) ;
    
    auto tup = std::make_tuple( std::string( key ), cb ) ;
    if( iter == man_data.type_callbacks.end() ) data().local_type_callbacks.push_back( man_data.type_callbacks.insert( iter, { type, tup } ) ) ;
  }
  
  void EventManager::enroll( EventCallback callback, nyx::Key keysym, const char* key )
  {
    typedef EventManager::FunctionSubscriber Callback ;
    
    Callback *cb ;
    
    cb = new Callback( reinterpret_cast<Callback::Callback>( callback ) ) ;
    
    auto iter = man_data.key_callbacks.find( keysym ) ;
    auto tup = std::make_tuple( std::string( key ), cb ) ;
    if( iter == man_data.key_callbacks.end() ) data().local_key_callbacks.push_back( man_data.key_callbacks.insert( iter, { keysym, tup } ) ) ;
  }
  
  void EventManager::enroll( EventCallback callback, const char* key )
  {
    typedef EventManager::FunctionSubscriber Callback ;
    
    Callback *cb ;
    
    cb = new Callback( reinterpret_cast<Callback::Callback>( callback ) ) ;

    auto iter = man_data.callbacks.find( key ) ;
    if( iter == man_data.callbacks.end() ) data().local_callbacks.push_back( man_data.callbacks.insert( iter, { key, cb } ) ) ;
  }
  
  void EventManager::enrollBase( Subscriber* subscriber, const char* key )
  {
    auto iter = man_data.callbacks.find( key ) ;
    if( iter == man_data.callbacks.end() ) data().local_callbacks.push_back( man_data.callbacks.insert( iter, { key, subscriber } ) ) ;
  }
  
  void EventManager::enrollBase( Subscriber* subscriber, nyx::Event::Type type, const char* key )
  {
    auto iter = man_data.type_callbacks.find( type ) ;
    
    auto tup = std::make_tuple( std::string( key ), subscriber ) ;
    if( iter == man_data.type_callbacks.end() ) data().local_type_callbacks.push_back( man_data.type_callbacks.insert( iter, { type, tup } ) ) ;
  }
  
  void EventManager::enrollBase( Subscriber* subscriber, nyx::Key keysym, const char* key )
  {
    auto iter = man_data.key_callbacks.find( keysym ) ;
    auto tup = std::make_tuple( std::string( key ), subscriber ) ;
    if( iter == man_data.key_callbacks.end() ) data().local_key_callbacks.push_back( man_data.key_callbacks.insert( iter, { keysym, tup } ) ) ;
  }

  void EventManager::updateMouse( float x, float y )
  {
    man_data.mouse_xpos = x ;
    man_data.mouse_ypos = y ;
  }
  
  void EventManager::updateMouseOffset( float x, float y )
  {
    man_data.mouse_xdelta = x ;
    man_data.mouse_ydelta = y ;
  }

  float EventManager::mouseX() const
  {
    return man_data.mouse_xpos ;
  }

  float EventManager::mouseY() const
  {
    return man_data.mouse_ypos ;
  }

  float EventManager::mouseDeltaX() const
  {
    return man_data.mouse_xdelta ;
  }

  float EventManager::mouseDeltaY() const
  {
    return man_data.mouse_ydelta ;
  }
      
  void EventManager::reset()
  {
    for( auto iter : data().local_callbacks )
    {
      delete iter->second ;
      man_data.callbacks.erase( iter ) ;
    }
    
    for( auto iter : data().local_key_callbacks )
    {
      delete std::get<1>( iter->second ) ;
      man_data.key_callbacks.erase( iter ) ;
    }
    
    for( auto iter : data().local_type_callbacks )
    {
      delete std::get<1>( iter->second ) ;
      man_data.type_callbacks.erase( iter ) ;
    }
  }

  EventManagerData& EventManager::data()
  {
    return *this->manager_data ;
  }
  
  const EventManagerData& EventManager::data() const
  {
    return *this->manager_data ;
  }
}

