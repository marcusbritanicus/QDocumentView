/**
 * This file is a part of QDocumentView Project.
 * QDocumentView is a widget to render multi-page documents
 * Copyright 2021-2022 Britanicus <marcusbritanicus@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * at your option, any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 **/

#include "ViewToolbar.hpp"

ViewToolbar::ViewToolbar( QWidget *parent ) : QWidget( parent ) {
    mZoomInBtn = new QToolButton( this );
    mZoomInBtn->setIcon( QIcon::fromTheme( "zoom-in" ) );
    mZoomInBtn->setFixedSize( QSize( 32, 32 ) );
    mZoomInBtn->setAutoRaise( true );

    mZoomOutBtn = new QToolButton( this );
    mZoomOutBtn->setIcon( QIcon::fromTheme( "zoom-out" ) );
    mZoomOutBtn->setFixedSize( QSize( 32, 32 ) );
    mZoomOutBtn->setAutoRaise( true );

    mZoomLbl = new QLabel( "100%" );
    mZoomLbl->setFixedHeight( 32 );

    mPagePrevBtn = new QToolButton( this );
    mPagePrevBtn->setIcon( QIcon::fromTheme( "arrow-left" ) );
    mPagePrevBtn->setFixedSize( QSize( 32, 32 ) );
    mPagePrevBtn->setAutoRaise( true );
    mPagePrevBtn->setEnabled( false );

    mPageNextBtn = new QToolButton( this );
    mPageNextBtn->setIcon( QIcon::fromTheme( "arrow-right" ) );
    mPageNextBtn->setFixedSize( QSize( 32, 32 ) );
    mPageNextBtn->setAutoRaise( true );
    mPageNextBtn->setEnabled( false );

    mPageLE = new QLineEdit( "1" );
    mPageLE->setAlignment( Qt::AlignCenter );
    mPageLE->setFixedSize( 64, 32 );

    mPageLbl = new QLabel( "of 1" );
    mPageLbl->setFixedHeight( 32 );

    mSearchPrevBtn = new QToolButton( this );
    mSearchPrevBtn->setIcon( QIcon::fromTheme( "arrow-left" ) );
    mSearchPrevBtn->setFixedSize( QSize( 32, 32 ) );
    mSearchPrevBtn->setAutoRaise( true );
    mSearchPrevBtn->setEnabled( false );
    mSearchPrevBtn->setShortcut( QKeySequence( Qt::SHIFT | Qt::Key_Return ) );

    mSearchNextBtn = new QToolButton( this );
    mSearchNextBtn->setIcon( QIcon::fromTheme( "arrow-right" ) );
    mSearchNextBtn->setFixedSize( QSize( 32, 32 ) );
    mSearchNextBtn->setAutoRaise( true );
    mSearchNextBtn->setEnabled( false );
    mSearchNextBtn->setShortcut( QKeySequence( Qt::Key_Return ) );

    mSearchLE = new QLineEdit();
    mSearchLE->setAlignment( Qt::AlignCenter );
    mSearchLE->setFixedSize( 270, 32 );
    mSearchLE->setClearButtonEnabled( true );
    mSearchLE->setPlaceholderText( "Search document" );

    /** Nothing to search */
    setSearchButtonsEnabled( false );

    QHBoxLayout *lyt = new QHBoxLayout();
    lyt->setContentsMargins( 2, 2, 2, 2 );

    lyt->addWidget( mZoomInBtn );
    lyt->addWidget( mZoomLbl );
    lyt->addWidget( mZoomOutBtn );

    lyt->addStretch();

    lyt->addWidget( mPagePrevBtn );
    lyt->addWidget( mPageLE );
    lyt->addWidget( mPageLbl );
    lyt->addWidget( mPageNextBtn );

    lyt->addStretch();

    lyt->addWidget( mSearchPrevBtn );
    lyt->addWidget( mSearchLE );
    lyt->addWidget( mSearchNextBtn );

    setLayout( lyt );

    /** Setup the connections */
    setupConnections();

    /** Animated hide/show toolbar */
    setupAnimationEffects();

    srchDelayTimer = new QBasicTimer();

    /** Become dull after 5s */
    opDownTimer->setInterval( 5000 );
    opDownTimer->start();
}


void ViewToolbar::setZoomButtonsEnabled( bool zoomInBtn, bool zoomOutBtn ) {
    mZoomInBtn->setEnabled( zoomInBtn );
    mZoomOutBtn->setEnabled( zoomOutBtn );
}


void ViewToolbar::setZoomText( QString text ) {
    mZoomLbl->setText( text );
}


void ViewToolbar::setMaximumPages( int pages ) {
    mMaxPages = pages;
    mPageLbl->setText( QString( "of %1" ).arg( pages ) );

    updatePageButtons();
}


void ViewToolbar::setCurrentPage( int page ) {
    if ( page + 1 > mMaxPages ) {
        mCurPage = mMaxPages;
    }

    else {
        mCurPage = page + 1;
    }

    mPageLE->setText( QString::number( mCurPage ) );

    updatePageButtons();
}


void ViewToolbar::focusSearch() {
    mSearchLE->setFocus();

    if ( opDownTimer->isActive() ) {
        opDownTimer->stop();
    }

    opUpTimer->start();
}


void ViewToolbar::clearSearch() {
    mSearchLE->clear();
    mSearchNextBtn->setDisabled( true );
    mSearchPrevBtn->setDisabled( true );

    /** Hide toolbar */
    if ( not underMouse() ) {
        opDownTimer->setInterval( 5000 );
        opDownTimer->start();
    }
}


void ViewToolbar::setSearchButtonsEnabled( bool yes ) {
    mSearchNextBtn->setEnabled( yes );
    mSearchPrevBtn->setEnabled( yes );
}


void ViewToolbar::setupConnections() {
    /** Zoom */
    connect(
        mZoomInBtn, &QToolButton::clicked, [ = ] () {
            emit zoomClicked( "enlarge" );
        }
    );

    connect(
        mZoomOutBtn, &QToolButton::clicked, [ = ] () {
            emit zoomClicked( "dwindle" );
        }
    );

    /** Page */
    connect(
        mPagePrevBtn, &QToolButton::clicked, [ = ] () {
            /** mCurPage is natural number, so logical number is mCurPage - 1 => previous == mCurPage - 2 */
            emit loadPage( mCurPage - 2 );
        }
    );

    connect(
        mPageNextBtn, &QToolButton::clicked, [ = ] () {
            /** mCurPage is natural number, so logical number is mCurPage - 1 => next == mCurPage */
            emit loadPage( mCurPage );
        }
    );

    connect(
        mPageLE, &QLineEdit::returnPressed, [ = ] () {
            /** mPageLE->text().toInt() is natural number, so logical number is mCurPage - 1 */
            emit loadPage( mPageLE->text().toInt() - 1 );
        }
    );

    /** Search */
    connect(
        mSearchLE, &QLineEdit::textEdited, [ = ] ( QString ) {
            /** Stop the timer */
            if ( srchDelayTimer->isActive() ) {
                srchDelayTimer->stop();
            }

            /** Start again: we wait 250 ms after user interaction to search */
            srchDelayTimer->start( 100, Qt::PreciseTimer, this );
        }
    );

    connect(
        mSearchPrevBtn, &QToolButton::clicked, [ = ] () {
            emit search( QString(), false, true );
        }
    );

    connect(
        mSearchNextBtn, &QToolButton::clicked, [ = ] () {
            emit search( QString(), false, false );
        }
    );
}


void ViewToolbar::setupAnimationEffects() {
    /** Dynamically change the opacity */
    setAutoFillBackground( true );

    opacity = new QGraphicsOpacityEffect( this );
    opacity->setOpacity( mOpacity );
    setGraphicsEffect( opacity );

    /** Timer to increase the opacity */
    opUpTimer = new QTimer( this );
    opUpTimer->setInterval( 10 );
    opUpTimer->setSingleShot( true );

    /** Timer to decrease the opacity */
    opDownTimer = new QTimer( this );
    opDownTimer->setSingleShot( true );

    /** On timeout increase the opacity */
    connect(
        opUpTimer, &QTimer::timeout, [ = ]() mutable {
            mOpacity += 0.1;
            opacity->setOpacity( mOpacity );
            QCoreApplication::processEvents();

            if ( mOpacity < 1.0 ) {
                opUpTimer->start();
            }

            else {
                mOpacity = 1.0;
                opacity->setOpacity( mOpacity );
                opUpTimer->stop();

                /** If the user has entered and left the widget in a short time */
                if ( not underMouse() ) {
                    opDownTimer->setInterval( 5000 );
                    opDownTimer->start();
                }
            }
        }
    );

    /** On timeout, start that actual dulling process. */
    connect(
        opDownTimer, &QTimer::timeout, [ = ]() mutable {
            mOpacity -= 0.05;
            opacity->setOpacity( mOpacity );
            QCoreApplication::processEvents();

            if ( mOpacity > 0.1 ) {
                opDownTimer->setInterval( 10 );
                opDownTimer->start();
            }

            else {
                mOpacity = 0.1;
                opacity->setOpacity( mOpacity );
                opDownTimer->stop();
            }
        }
    );
}


void ViewToolbar::updatePageButtons() {
    if ( mCurPage < mMaxPages ) {
        mPageNextBtn->setEnabled( true );
    }

    else {
        mPageNextBtn->setDisabled( true );
    }

    if ( mCurPage > 1 ) {
        mPagePrevBtn->setEnabled( true );
    }

    else {
        mPagePrevBtn->setDisabled( true );
    }
}


#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
void ViewToolbar::enterEvent( QEvent *eEvent ) {
#else
void ViewToolbar::enterEvent( QEnterEvent *eEvent ) {
#endif

    /** Stop the dulling timer */
    if ( opDownTimer->isActive() ) {
        opDownTimer->stop();
    }

    opUpTimer->start();

    eEvent->accept();
}


void ViewToolbar::leaveEvent( QEvent *event ) {
    /** Some timer is running. We have no business here */
    if ( opUpTimer->isActive() or opDownTimer->isActive() ) {
        event->accept();
        return;
    }

    /** Dont lose focus if the text edits have focus */
    if ( mPageLE->hasFocus() or mSearchLE->hasFocus() ) {
        event->accept();
        return;
    }

    /** Start becoming dull after 5s */
    opDownTimer->setInterval( 5000 );

    /** Start */
    opDownTimer->start();

    event->accept();
}


void ViewToolbar::timerEvent( QTimerEvent *tEvent ) {
    if ( tEvent->timerId() == srchDelayTimer->timerId() ) {
        srchDelayTimer->stop();

        QString needle = mSearchLE->text();

        if ( needle == oldSrchStr ) {
            emit search( needle, false, false );
        }

        else {
            oldSrchStr = needle;
            emit search( needle, true, false );
        }

        return;
    }

    QWidget::timerEvent( tEvent );
}
