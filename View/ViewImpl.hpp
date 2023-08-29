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

        void currentPageChanged( int currentPage );
        void calculateViewport();
        void setViewport( QRect viewport );
        void updateScrollBars();

        void invalidateDocumentLayout();

        qreal yPositionForPage( int page ) const;

        qreal zoomFactor() const;
        qreal zoomFactorForFitWidth() const;
        qreal zoomFactorForFitInView() const;

        struct DocumentLayout {
            QSize             documentSize;
            QHash<int, QRect> pageGeometries;
        };

        struct DocumentState {
            /**
             * Current page.
             * When a document reloads, we will try to
             * come to this position. DocumentState should be
             * cleared when setDocument(...) is called.
             */
            int     currentPage;

            /**
             * Current position of the document.
             * This is the relative horizontal and vertical position
             * of the document. When a document reloads, we will try
             * to restore this position.
             */
            QPointF currentPosition;
        };

        DocumentLayout calculateDocumentLayout() const;
        DocumentLayout calculateDocumentLayoutSingle() const;
        DocumentLayout calculateDocumentLayoutFacing() const;
        DocumentLayout calculateDocumentLayoutBook() const;
        DocumentLayout calculateDocumentLayoutOverview() const;
        void updateDocumentLayout();

        void highlightFirstSearchInstance( int page, QVector<QRectF> rects );
        void highlightNextSearchInstance();
        void highlightPreviousSearchInstance();

        /**
         * The user may have scrolled to this page.
         * If there is a search rect in this page, then highlight it.
         * IMP: Do not scroll. Attempt to highlight the rect in viewport.
         * If nothing is visible in the viewport, highlight the first rect.
         */
        void highlightSearchInstanceInCurrentPage();

        /**
         * Get current search position - will be calculated each time this function is called.
         */
        QPair<int, int> getCurrentSearchPosition();

        void paintOverlayRects( int, QImage& );

        /**
         * We can have two cases:
         * 1. Search rects, where the given rect is in original page scale and rotation. Case inverse =
         * false.
         * 2. Rubberband selection, where the rect is view page scale and rotation. Case inverse = true.
         * In both the cases, the top-left of the page is taken to be (0, 0)
         */
        QRectF getTransformedRect( QRectF, int page, bool inverse );

        /**
         * Scroll to make the given point/region of a page visible in the viewport.
         */
        void makePointVisible( QPointF, QRectF );
        void makeRegionVisible( QRectF, QRectF );

        QDocument *mDocument = nullptr;
        QDocumentNavigation *mPageNavigation = nullptr;
        QDocumentRenderer *mPageRenderer     = nullptr;

        QColor mPageColor;

        bool mContinuous;
        QDocumentView::PageLayout mPageLayout;
        QDocumentView::ZoomMode mZoomMode;
        qreal mZoomFactor;
        QDocumentRenderOptions mRenderOpts;

        int mPageSpacing;
        QMargins mDocumentMargins;

        bool mBlockPageScrolling;

        QMetaObject::Connection mDocumentStatusChangedConnection;
        QMetaObject::Connection mReloadDocumentConnection;

        QRect mViewPort;

        DocumentLayout mDocumentLayout;
        DocumentState mDocState;

        QDocumentSearch *mSearchThread;
        QHash<int, QVector<QRectF> > searchRects;
        int searchPage;
        QRectF curSearchRect;

        QDocumentView *publ;

        qreal mScreenResolution; // pixels per point
        bool pendingResize = false;
};


Q_DECLARE_TYPEINFO( QDocumentViewImpl::DocumentLayout, Q_MOVABLE_TYPE );
Q_DECLARE_TYPEINFO( QDocumentViewImpl::DocumentState,  Q_MOVABLE_TYPE );
