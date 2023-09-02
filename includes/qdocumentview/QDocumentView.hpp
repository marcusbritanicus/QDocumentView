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

#include <QDocument.hpp>
#include <QDocumentRenderOptions.hpp>

class Zoom;
class PageWidget;
class QProgressBar;

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
        Q_ENUM( PageLayout );

        enum ZoomMode {
            CustomZoom,
            FitToWidth,
            FitInView
        };
        Q_ENUM( ZoomMode );

        explicit QDocumentView( QWidget *parent = nullptr );
        ~QDocumentView();

        QDocument * load( QString );

        void setDocument( QDocument *document );
        QDocument * document() const;

        QDocumentNavigation * pageNavigation() const;

        bool isLayoutContinuous() const;
        PageLayout pageLayout() const;
        ZoomMode zoomMode() const;
        qreal zoomFactor() const;
        QDocumentRenderOptions renderOptions() const;

        QColor pageColor();
        void setPageColor( QColor );

        int pageSpacing() const;
        void setPageSpacing( int spacing );

        QMargins documentMargins() const;
        void setDocumentMargins( QMargins margins );

        void searchText( QString str );
        void clearSearch();

        /** Highlight the previous/next search instances */
        void highlightNextSearchInstance();
        void highlightPreviousSearchInstance();

        /** Get the position of the current search */
        QPair<int, int> getCurrentSearchPosition();

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
        /** We want to draw our pages */
        void paintEvent( QPaintEvent *event ) override;

        /** We need to reposition the widgets and repaint */
        void resizeEvent( QResizeEvent *event ) override;

        /** We need to repaint the viewport by moving the pages */
        void scrollContentsBy( int dx, int dy ) override;

        /** Changes pages with keys, zoom */
        void keyPressEvent( QKeyEvent *kEvent ) override;

        /** Change the document zoom */
        void wheelEvent( QWheelEvent *wEvent ) override;

    private:
        QDocumentViewImpl *impl;

        Zoom *mZoomBtn;
        PageWidget *mPagesBtn;
        QProgressBar *progress;

        bool mShowZoom  = true;
        bool mShowPages = true;
};
