/*
 * Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk
 * Copyright (C) 2014, 2015 Nicolas Bonnefon and other contributors
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

#include "tabbedcrawlerwidget.h"

#include <QKeyEvent>
#include <QLabel>
#include <QStyle>

#include "crawlerwidget.h"

#include "log.h"
#include "theme.h"

TabbedCrawlerWidget::TabbedCrawlerWidget() : QTabWidget(),
    olddata_icon_( Theme::iconPath( "data-old" ) ),
    newdata_icon_( Theme::iconPath( "data-new" ) ),
    newfiltered_icon_( Theme::iconPath( "data-filtered" ) ),
    myTabBar_(),
    iconSide_( QTabBar::LeftSide )
{
    // The close button is left where the style puts it, which is the right
    // hand side everywhere we support.
#ifdef WIN32
    myTabBar_.setStyleSheet( "QTabBar::tab {\
            height: 20px; "
            "} "
            "QTabBar::close-button {\
              height: 6px; width: 6px;\
              subcontrol-origin: padding;\
             }" );
#else
    // On GTK style, it looks better with a smaller font
    myTabBar_.setStyleSheet(
            "QTabBar::tab {"
            " height: 20px; "
            " font-size: 9pt; "
            "} "
            "QTabBar::close-button {\
              height: 6px; width: 6px;\
              subcontrol-origin: padding;\
             }" );
#endif
    setTabBar( &myTabBar_ );

    // Put the status icon opposite the close button. Asking the style where
    // the close button goes, rather than assuming, keeps the two apart on
    // platforms that close on the left (macOS): setTabButton() on the side
    // the close button occupies would replace it.
    const int closeSide = myTabBar_.style()->styleHint(
            QStyle::SH_TabBar_CloseButtonPosition, nullptr, &myTabBar_ );

    iconSide_ = ( closeSide == QTabBar::LeftSide )
        ? QTabBar::RightSide : QTabBar::LeftSide;

    myTabBar_.hide();

}

// I know hiding non-virtual functions from the base class is bad form
// and I do it here out of pure laziness: I don't want to encapsulate
// QTabBar with all signals and all just to implement this very simple logic.
// Maybe one day that should be done better...

int TabbedCrawlerWidget::addTab( QWidget* page, const QString& label )
{
    int index = QTabWidget::addTab( page, label );

    if ( auto crawler = dynamic_cast<CrawlerWidget*>( page ) ) {
        // Mmmmhhhh... pointer-to-member signal syntax creates tight coupling
        // between us and the sender, baaaaad....

        // Listen for a changing data status:
        connect( crawler, &CrawlerWidget::dataStatusChanged,
                [ this, index ]( DataStatus status ) { setTabDataStatus( index, status ); } );
    }

    // Display the icon
    QLabel* icon_label = new QLabel();
    icon_label->setPixmap( olddata_icon_.pixmap( 11, 12 ) );
    icon_label->setAlignment( Qt::AlignCenter );
    icon_label->setToolTip( tr("Status of the data in this tab") );
    myTabBar_.setTabButton( index, iconSide_, icon_label );

    LOG(logDEBUG) << "addTab, count = " << count();
    LOG(logDEBUG) << "width = " << olddata_icon_.pixmap( 11, 12 ).devicePixelRatio();

    if ( count() > 1 )
        myTabBar_.show();

    return index;
}

void TabbedCrawlerWidget::removeTab( int index )
{
    QTabWidget::removeTab( index );

    if ( count() <= 1 )
        myTabBar_.hide();
}

void TabbedCrawlerWidget::mouseReleaseEvent( QMouseEvent *event)
{
    LOG(logDEBUG) << "TabbedCrawlerWidget::mouseReleaseEvent";

    if (event->button() == Qt::MiddleButton)
    {
        int tab = this->myTabBar_.tabAt( event->pos() );
        if (-1 != tab)
        {
            emit tabCloseRequested( tab );
        }
    }
}

void TabbedCrawlerWidget::keyPressEvent( QKeyEvent* event )
{
    const auto mod = event->modifiers();
    const auto key = event->key();

    LOG(logDEBUG) << "TabbedCrawlerWidget::keyPressEvent";

    // Ctrl + tab
    if ( ( mod == Qt::ControlModifier && key == Qt::Key_Tab ) ||
         ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier ) && key == Qt::Key_Right ) ) {
        setCurrentIndex( ( currentIndex() + 1 ) % count() );
    }
    // Ctrl + shift + tab
    else if ( ( mod == ( Qt::ControlModifier | Qt::ShiftModifier ) && key == Qt::Key_Tab ) ||
              ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier ) && key == Qt::Key_Left ) ) {
        setCurrentIndex( ( currentIndex() - 1 >= 0 ) ? currentIndex() - 1 : count() - 1 );
    }
    // Ctrl + numbers
    else if ( mod == Qt::ControlModifier && ( key >= Qt::Key_1 && key <= Qt::Key_8 ) ) {
        int new_index = key - Qt::Key_0;
        if ( new_index <= count() )
            setCurrentIndex( new_index - 1 );
    }
    // Ctrl + 9
    else if ( mod == Qt::ControlModifier && key == Qt::Key_9 ) {
        setCurrentIndex( count() - 1 );
    }
    else if ( mod == Qt::ControlModifier && (key == Qt::Key_Q || key == Qt::Key_W) ) {
        emit tabCloseRequested( currentIndex() );
    }
    else {
        QTabWidget::keyPressEvent( event );
    }
}

void TabbedCrawlerWidget::setTabDataStatus( int index, DataStatus status )
{
    LOG(logDEBUG) << "TabbedCrawlerWidget::setTabDataStatus " << index;

    QLabel* icon_label = dynamic_cast<QLabel*>(
            myTabBar_.tabButton( index, iconSide_ ) );

    if ( icon_label ) {
        const QIcon* icon;
        QString tooltip;
        switch ( status ) {
            case DataStatus::OLD_DATA:
                icon = &olddata_icon_;
                tooltip = tr("No new data since this tab was last looked at");
                break;
            case DataStatus::NEW_DATA:
                icon = &newdata_icon_;
                tooltip = tr("New data has been appended to this file");
                break;
            case DataStatus::NEW_FILTERED_DATA:
                icon = &newfiltered_icon_;
                tooltip = tr("New data matching the current search has been "
                        "appended to this file");
                break;
        default:
            return;
        }

        icon_label->setPixmap ( icon->pixmap(12,12) );
        icon_label->setToolTip( tooltip );

    }
}
