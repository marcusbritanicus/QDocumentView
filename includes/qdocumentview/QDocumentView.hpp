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

#include <QtWidgets/qabstractscrollarea.h>
#include <qdocumentview/QDocumentRenderOptions.hpp>

class Zoom;
class PageWidget;
class QProgressBar;

class QDocument;
class QDocumentNavigation;
class QDocumentRenderer;
class QDocumentView;
class QDocumentViewImpl;

class QDocumentView : public QAbstractScrollArea {
    Q_OBJECT;

    Q_PROPERTY( QDocument * document READ document WRITE setDocument NOTIFY documentChanged );

    Q_PROPERTY( PageLayout pageLayout READ pageLayout WRITE setPageLayout NOTIFY pageLayoutChanged );
    Q_PROPERTY( ZoomMode zoomMode READ zoomMode WRITE setZoomMode NOTIFY zoomModeChanged );
    Q_PROPERTY( qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged );
    Q_PROPERTY( QDocumentRenderOptions renderOptions READ renderOptions WRITE setRenderOptions NOTIFY renderOptionsChanged );

    Q_PROPERTY( int pageSpacing READ pageSpacing WRITE setPageSpacing NOTIFY pageSpacingChanged );
    Q_PROPERTY( QMargins documentMargins READ documentMargins WRITE setDocumentMargins NOTIFY documentMarginsChanged );

    public:
        enum PageLayout {
            SinglePage,
            FacingPages,
            BookView,
            OverView
        };
        Q_ENUM( PageLayout )

        enum ZoomMode {
            CustomZoom,
            FitToWidth,
            FitInView
        };
        Q_ENUM( ZoomMode );

        explicit QDocumentView( QWidget *parent = nullptr );
        ~QDocumentView();

        void load( QString );

        void setDocument( QDocument *document );
        QDocument * document() const;

        QDocumentNavigation * pageNavigation() const;

        bool isLayoutContinuous() const;
        PageLayout pageLayout() const;
        ZoomMode zoomMode() const;
        qreal zoomFactor() const;
        QDocumentRenderOptions renderOptions() const;

        int pageSpacing() const;
        void setPageSpacing( int spacing );

        QMargins documentMargins() const;
        void setDocumentMargins( QMargins margins );

        void searchText( QString str );

        bool showPagesOSD() const;
        void setShowPagesOSD( bool );

        bool showZoomOSD() const;
        void setShowZoomOSD( bool );

    public Q_SLOTS:
        void setLayoutContinuous( bool );
        void setPageLayout( PageLayout mode );
        void setZoomMode( ZoomMode mode );
        void setZoomFactor( qreal factor );
        void setRenderOptions( QDocumentRenderOptions opts );

    Q_SIGNALS:
        void documentChanged( QDocument *document );
        void layoutContinuityChanged( bool continuous );
        void pageLayoutChanged( PageLayout pageLayout );
        void zoomModeChanged( ZoomMode zoomMode );
        void zoomFactorChanged( qreal zoomFactor );
        void renderOptionsChanged( QDocumentRenderOptions opts );
        void pageSpacingChanged( int pageSpacing );
        void documentMarginsChanged( QMargins documentMargins );

        void documentLoadingFailed();

        /** Search signals */
        void matchesFound( int numMatches );
        void searchComplete( int numMatches );

    protected:
        void paintEvent( QPaintEvent *event ) override;
        void resizeEvent( QResizeEvent *event ) override;
        void scrollContentsBy( int dx, int dy ) override;

        void keyPressEvent( QKeyEvent *kEvent );

        void wheelEvent( QWheelEvent *wEvent );

    private:
        QDocumentViewImpl *impl;

        Zoom *mZoomBtn;
        PageWidget *mPagesBtn;
        QProgressBar *progress;

        bool mShowZoom  = true;
        bool mShowPages = true;
};
