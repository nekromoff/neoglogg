/*
 * Copyright (C) 2017 Nicolas Bonnefon and other contributors
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

#ifndef NEOGLOGGAPP_H
#define NEOGLOGGAPP_H

#include <QApplication>

// Subclass QApplication to add a custom event handler
class NeogloggApp : public QApplication
{
    Q_OBJECT
  public:
    NeogloggApp( int &argc, char **argv ) : QApplication( argc, argv )
    {}

  signals:
    void loadFile( const QString& file_name );

  protected:
#ifdef __APPLE__
    virtual bool event( QEvent* event );
#endif
};

#endif
