/*
 * Copyright (C) 2016 Nicolas Bonnefon and other contributors
 *
 * This file is part of neoglogg.
 *
 * neoglogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * neoglogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with neoglogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef THEME_H
#define THEME_H

// Application-wide colour scheme.
//
// The log views paint themselves from the palette roles (Base, Text,
// Highlight, ...) rather than from hardcoded colours, so switching the
// application palette is enough to re-theme them.
namespace Theme {
    // Switch the application between the dark scheme (Fusion style plus a
    // dark palette) and the platform default it was started with.
    //
    // The first call remembers the style and palette in use, so turning dark
    // mode back off restores the original look rather than leaving Fusion
    // behind. Must be called from the GUI thread, after QApplication exists.
    void apply( bool dark );
};

#endif
