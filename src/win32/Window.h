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
 * File:   Window.h
 * Author: Jordan
 *
 * Created on January 9, 2021, 6:04 AM
 */

#pragma once
#ifdef _WIN32
// Win32 Forward declares.
#ifndef _WINDEF_
  struct  HINSTANCE__            ; 
  typedef HINSTANCE__* HINSTANCE ;
  struct  HWND__                 ;
  typedef HWND__* HWND           ;
#endif

namespace nyx
{
  namespace win32
  {
    class Window
    {
      public:
        
        /** Default constructor.
         */
        Window() ;
        
        /** Default deconstructor.
         */
        ~Window() ;
        
        /** Method to initialize this object.
         * @param window_title The title of the window. 
         * @param width The width of the window in pixels.
         * @param height The height of the window in pixels.
         */
        void initialize( const char* window_title, unsigned width, unsigned height ) ;
        
        /** Method to set the monitor to put the window on.
         * @param monitor_id The ID of the monitor to put the window on.
         */
        void setMonitor( unsigned monitor_id ) ;
        
        /** Method to set whether the window should be fullscreen or not.
         * @param value Whether or not the window should be fullscreen.
         */
        void setFullscreen( bool value ) ;
        
        /** Method to set whether or not the window should be resizable.
         * @param value Whether or not the window should be resizable.
         */
        void setResizable( bool value ) ;
        
        /** Method to set whether or not the window should be borderless.
         * @param value Whether or not the window should be borderless.
         */
        void setBorderless( bool value ) ;
        
        /** Method to set whether or not the window should be minimized or not.
         * @param value Whether or not the window should be minimized.
         */
        void setMinimize( bool value ) ;
        
        /** Method to set whether or not the window should be maximized or not.
         * @param value Whether or not the menu should be maximized.
         */
        void setMaximized( bool value ) ;
        
        /** Method to process events being sent to the window.
         */
        void handleEvents() ;
        
        /** Method to retrieve a const referene to this object's win32 instance.
        * @return Const reference to this object's win32 instance.
        */
        const HINSTANCE& instance() const ;

        /** Method to retrieve a const referene to this object's win32 window handle.
        * @return Const reference to this object's win32 window handle.
        */
        const HWND& handle() const ;


        /** Method to reset this object to it's initial state.
         */
        void reset() ;

      private:
        
        /** Forward-declared structure to contain this object's internal data.
         */
        struct WindowData* window_data ;
        
        /** Method to retrieve a reference to this object's internal data.
         * @return Reference to this object's internal data.
         */
        WindowData& data() ;
        
        /** Method to retrieve a const-reference to this object's internal data.
         * @return Const-reference to this object's internal data.
         */
        const WindowData& data() const ;
    };
  }
}

#endif

