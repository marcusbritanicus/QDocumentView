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

#include <QThread>
#include <QPointer>

class QDocumentNavigation;
class QDocumentRenderer;
class QDocument;
class QDocumentSearch;
class QDocumentView;

class QDocumentViewImpl {
    public:
        QDocumentViewImpl( QDocumentView *view );
        ~QDocumentViewImpl();

        void init();

        void documentStatusChanged();
        void currentPageChanged( int currentPage );
        void calculateViewport();
        void setViewport( QRect viewport );
        void updateScrollBars();

        void invalidateDocumentLayout();

        qreal yPositionForPage( int page ) const;

        qreal zoomFactor() const;

        struct DocumentLayout {
            QSize             documentSize;
            QHash<int, QRect> pageGeometries;
        };

        DocumentLayout calculateDocumentLayout() const;
        DocumentLayout calculateDocumentLayoutSingle() const;
        DocumentLayout calculateDocumentLayoutFacing() const;
        DocumentLayout calculateDocumentLayoutBook() const;
        DocumentLayout calculateDocumentLayoutOverview() const;
        void updateDocumentLayout();

        void paintSearchRects( int, QImage& );

        QDocument *m_document = nullptr;
        QDocumentNavigation *m_pageNavigation = nullptr;
        QDocumentRenderer *m_pageRenderer     = nullptr;

        bool m_continuous;
        QDocumentView::PageLayout m_pageLayout;
        QDocumentView::ZoomMode m_zoomMode;
        qreal m_zoomFactor;
        QDocumentRenderOptions m_renderOpts;

        int m_pageSpacing;
        QMargins m_documentMargins;

        bool m_blockPageScrolling;

        QMetaObject::Connection m_documentStatusChangedConnection;
        QMetaObject::Connection m_reloadDocumentConnection;

        QRect m_viewport;

        DocumentLayout m_documentLayout;

        QDocumentSearch *m_searchThread;
        QHash<int, QVector<QRectF> > searchRects;

        QDocumentView *publ;

        qreal m_screenResolution; // pixels per point
        bool pendingResize = false;
};


Q_DECLARE_TYPEINFO( QDocumentViewImpl::DocumentLayout, Q_MOVABLE_TYPE );
