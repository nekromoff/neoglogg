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

#include "theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

#include "log.h"

namespace {

// The style and palette the application started with, captured on the first
// call to apply() so that leaving dark mode can restore them.
bool     defaultsSaved  = false;
QString  defaultStyle;
QPalette defaultPalette;

// Which glyph set iconPath() hands out. Tracks the last apply().
bool darkActive = false;

QPalette darkPalette()
{
    const QColor window( 0x35, 0x35, 0x35 );
    const QColor base( 0x2a, 0x2a, 0x2a );
    const QColor text( 0xd8, 0xd8, 0xd8 );
    const QColor accent( 0x2a, 0x82, 0xda );
    const QColor disabled( 0x7f, 0x7f, 0x7f );

    QPalette palette;

    palette.setColor( QPalette::Window, window );
    palette.setColor( QPalette::WindowText, text );
    palette.setColor( QPalette::Base, base );
    palette.setColor( QPalette::AlternateBase, window );
    palette.setColor( QPalette::ToolTipBase, window );
    palette.setColor( QPalette::ToolTipText, text );
    palette.setColor( QPalette::Text, text );
    palette.setColor( QPalette::Button, window );
    palette.setColor( QPalette::ButtonText, text );
    palette.setColor( QPalette::BrightText, Qt::red );
    palette.setColor( QPalette::Link, accent );
    palette.setColor( QPalette::Highlight, accent );
    palette.setColor( QPalette::HighlightedText, Qt::black );

    // Greyed-out widgets need explicit colours: the defaults are derived from
    // the light palette and end up unreadable on a dark background.
    palette.setColor( QPalette::Disabled, QPalette::WindowText, disabled );
    palette.setColor( QPalette::Disabled, QPalette::Text, disabled );
    palette.setColor( QPalette::Disabled, QPalette::ButtonText, disabled );
    palette.setColor( QPalette::Disabled, QPalette::Highlight,
            QColor( 0x50, 0x50, 0x50 ) );
    palette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabled );

    return palette;
}

} // namespace

void Theme::apply( bool dark )
{
    if ( ! qApp ) {
        LOG(logERROR) << "Theme::apply called before the QApplication exists";
        return;
    }

    if ( ! defaultsSaved ) {
        defaultStyle   = qApp->style()->objectName();
        defaultPalette = qApp->palette();
        defaultsSaved  = true;

        LOG(logDEBUG) << "Theme: default style is "
            << defaultStyle.toStdString();
    }

    // setStyle() installs the style's own standard palette, so the palette
    // has to be set afterwards in both directions.
    if ( dark ) {
        if ( QStyle* fusion = QStyleFactory::create( "Fusion" ) )
            qApp->setStyle( fusion );
        else
            LOG(logWARNING) << "Theme: the Fusion style is not available, "
                "using the current style with the dark palette";

        qApp->setPalette( darkPalette() );
    }
    else {
        if ( QStyle* original = QStyleFactory::create( defaultStyle ) )
            qApp->setStyle( original );

        qApp->setPalette( defaultPalette );
    }

    darkActive = dark;
}

QString Theme::iconPath( const QString& name )
{
    return darkActive ? QStringLiteral( ":/images/dark/" ) + name
                            + QStringLiteral( ".png" )
                      : QStringLiteral( ":/images/" ) + name
                            + QStringLiteral( ".png" );
}
