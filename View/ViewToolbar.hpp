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

#pragma once

#include <QtCore>
#include <QtGui>
#include <QtWidgets>


class ViewToolbar : public QWidget {
    Q_OBJECT;

    public:
        ViewToolbar( QWidget * );

        /** Zoom related */
        void setZoomButtonsEnabled( bool zoomInBtn, bool zoomOut );
        void setZoomText( QString );

        /** Pages related */
        void setMaximumPages( int );
        void setCurrentPage( int );

        /** Searhc related */
        void focusSearch();
        void clearSearch();
        void setSearchButtonsEnabled( bool yes );

    private:
        QToolButton *mZoomInBtn, *mZoomOutBtn;
        QLabel *mZoomLbl;

        QToolButton *mPagePrevBtn, *mPageNextBtn;
        QLineEdit *mPageLE;
        QLabel *mPageLbl;

        QToolButton *mSearchPrevBtn, *mSearchNextBtn;
        QLineEdit *mSearchLE;

        int mCurPage  = 1;
        int mMaxPages = 1;

        QString oldSrchStr;
        QBasicTimer *srchDelayTimer;

        QGraphicsOpacityEffect *opacity;
        qreal mOpacity = 1.0;
        QTimer *opUpTimer, *opDownTimer;

        void setupConnections();
        void setupAnimationEffects();

        void updatePageButtons();

    protected:
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        void enterEvent( QEvent * ) override;

#else
        void enterEvent( QEnterEvent * ) override;

#endif

        /** We need to reduce the opacity when the mouse leaves the widget */
        void leaveEvent( QEvent * ) override;

        void timerEvent( QTimerEvent *tEvent ) override;

    signals:
        void zoomClicked( QString );
        void loadPage( int );

        /**
         * Search for a string @needle
         * @fresh -> new search?
         * reverse -> older instances?
         * When @fresh is true, we reset the search.
         * If reverse is true, search previous instance, else next instance.
         */
        void search( QString needle, bool fresh, bool reverse );
};
