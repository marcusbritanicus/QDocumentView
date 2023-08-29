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

#include <qdocumentview/QDocumentView.hpp>
#include <qdocumentview/QDocumentRenderer.hpp>
#include <qdocumentview/QDocument.hpp>
#include <qdocumentview/QDocumentNavigation.hpp>
#include <qdocumentview/QDocumentSearch.hpp>

#include "ViewImpl.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QScroller>


QDocumentViewImpl::QDocumentViewImpl( QDocumentView *view ) {
    publ       = view;                                 // Pointer to the public class
    mPageColor = QColor( 0xff, 0xff, 0xff );           // Page color (works only for transparent pdfs, i.e.,
                                                       // non-scan)
    mContinuous         = true;                        // Continuous layout
    mPageLayout         = QDocumentView::SinglePage;   // Draw only one page (single column)
    mZoomMode           = QDocumentView::CustomZoom;   // Start with a custom zoom of 1.0
    mZoomFactor         = 1.0;                         // Zoom factor for custom zoom
    mPageSpacing        = 5;                           // Default space between two pages
    mDocumentMargins    = QMargins( 6, 6, 6, 6 );      // Page margins
    mBlockPageScrolling = false;                       // Flag to handle setting current page
    mScreenResolution   = QGuiApplication::primaryScreen()->logicalDotsPerInch() / 72.0;

    mDocState.currentPage     = 0;
    mDocState.currentPosition = QPointF( 0, 0 );

    mPageNavigation = new QDocumentNavigation( view );
    mPageRenderer   = new QDocumentRenderer( view );
    mSearchThread   = new QDocumentSearch( view );
}


QDocumentViewImpl::~QDocumentViewImpl() {
    delete mDocument;
    delete mPageNavigation;
    delete mPageRenderer;

    mSearchThread->stop();
    delete mSearchThread;
}


void QDocumentViewImpl::currentPageChanged( int currentPage ) {
    if ( mBlockPageScrolling ) {
        return;
    }

    mDocState.currentPage = mPageNavigation->currentPage();
    publ->verticalScrollBar()->setValue( yPositionForPage( currentPage ) );

    if ( mPageLayout == QDocumentView::SinglePage ) {
        invalidateDocumentLayout();
    }
}


void QDocumentViewImpl::calculateViewport() {
    if ( not mDocument ) {
        return;
    }

    const int x      = publ->horizontalScrollBar()->value();
    const int y      = publ->verticalScrollBar()->value();
    const int width  = publ->viewport()->width();
    const int height = publ->viewport()->height();

    setViewport( QRect( x, y, width, height ) );
}


void QDocumentViewImpl::setViewport( QRect viewport ) {
    if ( mViewPort == viewport ) {
        return;
    }

    const QSize oldSize = mViewPort.size();

    mViewPort = viewport;

    if ( oldSize != mViewPort.size() ) {
        updateDocumentLayout();
    }

    if ( mContinuous ) {
        // An imaginary, 2px height line at the upper half of the viewport, which is used to
        // determine which page is currently located there -> we propagate that as 'current' page
        // to the QDocumentNavigation object
        const QRect currentPageLine( mViewPort.x(), mViewPort.y() + mViewPort.height() * 0.4, mViewPort.width(), 2 );

        int currentPage = 0;
        for ( auto it = mDocumentLayout.pageGeometries.cbegin(); it != mDocumentLayout.pageGeometries.cend(); ++it ) {
            const QRect pageGeometry = it.value();

            if ( pageGeometry.intersects( currentPageLine ) ) {
                currentPage = it.key();
                break;
            }
        }

        if ( currentPage != mPageNavigation->currentPage() ) {
            mBlockPageScrolling   = true;
            mDocState.currentPage = currentPage;
            mPageNavigation->setCurrentPage( currentPage );
            mBlockPageScrolling = false;
        }
    }
}


void QDocumentViewImpl::updateScrollBars() {
    const QSize p = publ->viewport()->size();
    const QSize v = mDocumentLayout.documentSize;

    QScrollBar *vScroll = publ->verticalScrollBar();
    QScrollBar *hScroll = publ->horizontalScrollBar();

    qreal vPos = 1.0 * vScroll->value() / vScroll->maximum();
    qreal hPos = 0.5;

    int vMax = v.height() - p.height();
    int hMax = v.width() - p.width();

    vScroll->setRange( 0, vMax );
    vScroll->setPageStep( p.height() );
    vScroll->setValue( vPos * vMax );

    hScroll->setRange( 0, hMax );
    hScroll->setPageStep( p.width() );
    hScroll->setValue( hPos * hMax );
}


void QDocumentViewImpl::invalidateDocumentLayout() {
    updateDocumentLayout();
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayout() const {
    switch ( mPageLayout ) {
        /** One column */
        case QDocumentView::SinglePage: {
            return calculateDocumentLayoutSingle();
        }

        /** Two columns */
        case QDocumentView::FacingPages: {
            return calculateDocumentLayoutFacing();
        }

        /** Two columns; First page treated as cover */
        case QDocumentView::BookView: {
            return calculateDocumentLayoutBook();
        }

        /** Multiple columns, depending on zoom */
        case QDocumentView::OverView: {
            return calculateDocumentLayoutOverview();
        }
    }

    return QDocumentViewImpl::DocumentLayout();
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutSingle() const {
    DocumentLayout documentLayout;

    if ( !mDocument || (mDocument->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = mDocument->pageCount() - 1;

    int totalWidth = 0;

    const int startPage = (mContinuous ? 0         : mPageNavigation->currentPage() );
    const int endPage   = (mContinuous ? pageCount : mPageNavigation->currentPage() );

    const int horizMargins = mDocumentMargins.left() + mDocumentMargins.right();
    int       pageY        = mDocumentMargins.top();

    // calculate page sizes
    for (int page = startPage; page <= endPage; ++page) {
        QSizeF pageSize = mDocument->pageSize( page ) * mScreenResolution;

        switch ( mRenderOpts.rotation() ) {
            /* 90 degree rotated */
            case QDocumentRenderOptions::Rotate90:
            case QDocumentRenderOptions::Rotate270: {
                /** Swap width <-> height */
                pageSize.transpose();
            }

            default: {
                break;
            }
        }

        if ( mZoomMode == QDocumentView::CustomZoom ) {
            pageSize *= mZoomFactor;
        }

        else if ( mZoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = (qreal( mViewPort.width() - horizMargins ) / qreal( pageSize.width() ) );
            pageSize *= factor;
        }

        else if ( mZoomMode == QDocumentView::FitInView ) {
            const int   margins = (mContinuous ? 0 : mDocumentMargins.bottom() + mDocumentMargins.top() - mPageSpacing);
            const QSize viewportSize( mViewPort.size() - QSize( horizMargins, mPageSpacing + margins ) );

            pageSize += QSizeF( 0, mPageSpacing );
            pageSize  = pageSize.scaled( viewportSize, Qt::KeepAspectRatio );
        }

        totalWidth             = qMax( totalWidth, pageSize.toSize().width() + horizMargins );
        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), pageSize.toSize() );
    }

    // calculate page positions
    for (int page = startPage; page <= endPage; ++page) {
        const QSize pageSize = pageGeometries[ page ].size();

        // center horizontal inside the viewport
        const int pageX = (qMax( totalWidth, mViewPort.width() ) - pageSize.width() ) / 2;

        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        pageY += pageSize.height() + mPageSpacing;
    }

    /** We added an amount 'mPageSpacing' amount extra */
    pageY += mDocumentMargins.bottom() - mPageSpacing;

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize( totalWidth, pageY );

    return documentLayout;
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutFacing() const {
    DocumentLayout documentLayout;

    if ( !mDocument || (mDocument->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = mDocument->pageCount() - 1;
    const int curPage   = mPageNavigation->currentPage();

    int totalWidth = 0;

    int startPage = 0;
    int endPage   = pageCount;

    if ( not mContinuous ) {
        if ( curPage % 2 == 0 ) {
            startPage = curPage;
            endPage   = curPage;
        }

        else {
            startPage = curPage - 1;
            endPage   = curPage - 1;
        }
    }

    const int horizMargins = mDocumentMargins.left() + mDocumentMargins.right();

    // calculate page sizes
    for (int page = startPage; page <= endPage; page += 2) {
        QSizeF pageSize;

        QSizeF p1 = mDocument->pageSize( page ) * mScreenResolution;
        QSizeF p2 = mDocument->pageSize( page + 1 ) * mScreenResolution;

        switch ( mRenderOpts.rotation() ) {
            /* 90 degree rotated */
            case QDocumentRenderOptions::Rotate90:
            case QDocumentRenderOptions::Rotate270: {
                /** Swap width <-> height */
                p1.transpose();
                p2.transpose();
            }

            default: {
                break;
            }
        }

        /** Page size is sum of individual pages, suitable pre-rotated */
        pageSize = QSizeF( p1.width() + p2.width(), qMax( p1.height(), p2.height() ) );

        if ( mZoomMode == QDocumentView::CustomZoom ) {
            pageSize *= mZoomFactor;
            p1       *= mZoomFactor;
            p2       *= mZoomFactor;
            pageSize += QSizeF( mPageSpacing, 0 );
        }

        else if ( mZoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = qreal( mViewPort.width() - horizMargins - mPageSpacing ) / qreal( pageSize.width() );
            pageSize *= factor;
            p1       *= (factor / (p2.isValid() ? 1.0 : 2.0) );
            p2       *= factor;

            if ( p2.isValid() ) {
                pageSize += QSizeF( mPageSpacing, 0 );
            }
        }

        else if ( mZoomMode == QDocumentView::FitInView ) {
            const int   margins = (mContinuous ? 0 : mDocumentMargins.bottom() + mDocumentMargins.top() - mPageSpacing);
            const QSize viewportSize( mViewPort.size() - QSize( horizMargins, mPageSpacing + margins ) );

            pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio ) + QSizeF( mPageSpacing, 0 );

            if ( not p2.isValid() ) {
                p1 = QSizeF( pageSize.width(), pageSize.height() );
            }

            else {
                p1        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                p2        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                pageSize += QSizeF( mPageSpacing, 0 );
            }
        }

        totalWidth = qMax( totalWidth, pageSize.toSize().width() + horizMargins );

        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), p1.toSize() );

        if ( not not p2.isValid() ) {
            pageGeometries[ page + 1 ] = QRect( QPoint( 0, 0 ), p2.toSize() );
        }
    }

    int pageY = mDocumentMargins.top();

    // calculate page positions
    for (int page = startPage; page <= endPage; page += 2) {
        const QSize size1 = pageGeometries.value( page ).size();
        const QSize size2 = pageGeometries.value( page + 1 ).size();

        const int pageHeight = qMax( size1.height(), size2.height() );

        /** center horizontal inside the viewport */
        const int pageX = (qMax( totalWidth, mViewPort.width() ) - size1.width() - (size2.isNull() ? 0 : size2.width() + mPageSpacing) ) / 2;
        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        if ( size2.isValid() ) {
            pageGeometries[ page + 1 ].moveTopLeft( QPoint( pageX + size1.width() + mPageSpacing, pageY ) );
        }

        pageY += pageHeight + mPageSpacing;
    }

    pageY += mDocumentMargins.bottom() - mPageSpacing;

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize( totalWidth, pageY );

    return documentLayout;
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutBook() const {
    DocumentLayout documentLayout;

    if ( !mDocument || (mDocument->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = mDocument->pageCount() - 1;
    const int curPage   = mPageNavigation->currentPage();

    int totalWidth = 0;

    int startPage = 0;
    int endPage   = pageCount;

    if ( not mContinuous ) {
        /** First page */
        if ( curPage == 0 ) {
            startPage = 0;
            endPage   = 0;
        }

        /** Odd pages */
        else if ( curPage % 2 == 1 ) {
            startPage = curPage;
            endPage   = curPage;
        }

        /** Even pages */
        else {
            startPage = curPage - 1;
            endPage   = curPage - 1;
        }
    }

    const int horizMargins = mDocumentMargins.left() + mDocumentMargins.right();

    // calculate page sizes
    /** First page */
    if ( startPage == 0 ) {
        QSizeF p1 = mDocument->pageSize( 0 ) * mScreenResolution;

        switch ( mRenderOpts.rotation() ) {
            /* 90 degree rotated */
            case QDocumentRenderOptions::Rotate90:
            case QDocumentRenderOptions::Rotate270: {
                /** Swap width <-> height */
                p1.transpose();
            }

            default: {
                break;
            }
        }

        if ( mZoomMode == QDocumentView::CustomZoom ) {
            p1 *= mZoomFactor;
        }

        else if ( mZoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = (qreal( mViewPort.width() - horizMargins ) / qreal( 2 * p1.width() + mPageSpacing ) );
            p1 *= factor;
        }

        else if ( mZoomMode == QDocumentView::FitInView ) {
            const int   margins = (mContinuous ? 0 : mDocumentMargins.bottom() + mDocumentMargins.top() - mPageSpacing);
            const QSize viewportSize( mViewPort.size() - QSize( horizMargins, mPageSpacing + margins ) );

            p1 = p1.scaled( viewportSize, Qt::KeepAspectRatio );
        }

        totalWidth          = qMax( totalWidth, p1.toSize().width() + horizMargins );
        pageGeometries[ 0 ] = QRect( QPoint( 0, 0 ), p1.toSize() );
    }

    for (int page = (startPage == 0 ? 1 : startPage); page <= endPage; page += 2) {
        QSizeF pageSize;

        QSizeF p1 = mDocument->pageSize( page ) * mScreenResolution;
        QSizeF p2( 0, 0 );

        if ( page + 1 < mDocument->pageCount() ) {
            p2 = mDocument->pageSize( page + 1 ) * mScreenResolution;
        }

        switch ( mRenderOpts.rotation() ) {
            /* 90 degree rotated */
            case QDocumentRenderOptions::Rotate90:
            case QDocumentRenderOptions::Rotate270: {
                /** Swap width <-> height */
                p1.transpose();
                p2.transpose();
            }

            default: {
                break;
            }
        }

        /** Page size is sum of individual pages, suitably pre-rotated */
        pageSize = QSizeF( p1.width() + p2.width(), qMax( p1.height(), p2.height() ) );

        if ( mZoomMode == QDocumentView::CustomZoom ) {
            pageSize *= mZoomFactor;
            p1       *= mZoomFactor;
            p2       *= mZoomFactor;

            if ( not not p2.isValid() ) {
                pageSize += QSizeF( mPageSpacing, 0 );
            }
        }

        else if ( mZoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = qreal( mViewPort.width() - horizMargins - mPageSpacing ) / qreal( pageSize.width() );
            pageSize *= factor;
            p1       *= (factor / (p2.width() <= 0 ? 2.0 : 1.0) );
            p2       *= factor;

            if ( not not p2.isValid() ) {
                pageSize += QSizeF( mPageSpacing, 0 );
            }
        }

        else if ( mZoomMode == QDocumentView::FitInView ) {
            const int   margins = (mContinuous ? 0 : mDocumentMargins.bottom() + mDocumentMargins.top() - mPageSpacing);
            const QSize viewportSize( mViewPort.size() - QSize( horizMargins, mPageSpacing + margins ) );

            pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio ) + (not not p2.isValid() ? QSizeF( mPageSpacing, 0 ) : QSizeF( 0, 0 ) );

            if ( not p2.isValid() ) {
                p1 = QSizeF( pageSize.width(), pageSize.height() );
            }

            else {
                p1        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                p2        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                pageSize += QSizeF( mPageSpacing, 0 );
            }
        }

        totalWidth = qMax( totalWidth, pageSize.toSize().width() + horizMargins );

        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), p1.toSize() );

        if ( not not p2.isValid() ) {
            pageGeometries[ page + 1 ] = QRect( QPoint( 0, 0 ), p2.toSize() );
        }
    }

    int pageY = mDocumentMargins.top();

    // calculate page positions
    if ( startPage == 0 ) {
        const QSize s1         = pageGeometries[ 0 ].size();
        const int   pageHeight = s1.height();
        const int   pageX      = (qMax( totalWidth, mViewPort.width() ) - s1.width() ) / 2;
        pageGeometries[ 0 ].moveTopLeft( QPoint( pageX, pageY ) );
        pageY += pageHeight + mPageSpacing;
    }

    for (int page = (startPage == 0 ? 1 : startPage); page <= endPage; page += 2) {
        const QSize size1 = pageGeometries.value( page ).size();
        const QSize size2 = pageGeometries.value( page + 1 ).size();

        const int pageHeight = qMax( size1.height(), size2.height() );

        // center horizontal inside the viewport
        const int pageX = (qMax( totalWidth, mViewPort.width() ) - size1.width() - (size2.isNull() ? 0 : size2.width() + mPageSpacing) ) / 2;

        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        if ( size2.isValid() ) {
            pageGeometries[ page + 1 ].moveTopLeft( QPoint( pageX + size1.width() + mPageSpacing, pageY ) );
        }

        pageY += pageHeight + mPageSpacing;
    }

    pageY += mDocumentMargins.bottom() - mPageSpacing;

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize( totalWidth, pageY );

    return documentLayout;
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutOverview() const {
    DocumentLayout docLyt;

    return docLyt;
}


qreal QDocumentViewImpl::yPositionForPage( int pageNumber ) const {
    const auto it = mDocumentLayout.pageGeometries.constFind( pageNumber );

    if ( it == mDocumentLayout.pageGeometries.cend() ) {
        return 0.0;
    }

    return (*it).y();
}


void QDocumentViewImpl::updateDocumentLayout() {
    mDocumentLayout = calculateDocumentLayout();

    updateScrollBars();
}


qreal QDocumentViewImpl::zoomFactor() const {
    QSize pageSize;

    if ( mZoomMode == QDocumentView::CustomZoom ) {
        return mZoomFactor;
    }

    else if ( mZoomMode == QDocumentView::FitToWidth ) {
        return zoomFactorForFitWidth();
    }

    else if ( mZoomMode == QDocumentView::FitInView ) {
        return zoomFactorForFitInView();
    }

    return 1.0;
}


qreal QDocumentViewImpl::zoomFactorForFitWidth() const {
    int page = mPageNavigation->currentPage();

    QSize       pageSize = QSizeF( mDocument->pageSize( page ) * mScreenResolution ).toSize();
    const qreal factor   = (qreal( mViewPort.width() - mDocumentMargins.left() - mDocumentMargins.right() ) / qreal( pageSize.width() ) );

    return factor;
}


qreal QDocumentViewImpl::zoomFactorForFitInView() const {
    int page = mPageNavigation->currentPage();

    const QSize viewportSize( mViewPort.size() + QSize( -mDocumentMargins.left() - mDocumentMargins.right(), -mPageSpacing ) );

    QSize pageSize = QSizeF( mDocument->pageSize( page ) * mScreenResolution ).toSize();

    pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio );

    return pageSize.width() / mDocument->pageSize( page ).width();
}


void QDocumentViewImpl::highlightFirstSearchInstance( int page, QVector<QRectF> rects ) {
    /** We have already highlighted the first instance. */
    if ( curSearchRect.isValid() ) {
        return;
    }

    /**
     * How `Search` results come.
     * - Current page is searched first.
     * - Then all the subsequent pages will be scanned till the end of the document.
     * - Finally, the document will be scanned from the beginning til lthe current page.
     * Since the search (at present) is sequential, the first ping will be taken to be
     * the first search result.
     */

    /** Set the curSearchRect, and searchPage */
    curSearchRect = rects[ 0 ];
    searchPage    = page;

    /** Geometry of @page */
    QRectF pageGeometry    = mDocumentLayout.pageGeometries[ page ];
    QRectF transformedRect = getTransformedRect( curSearchRect, page, false );

    /** If this is the current page, make sure to focus the current search rect */
    if ( page == mDocState.currentPage ) {
        makeRegionVisible( transformedRect, pageGeometry );
    }

    /** Not the current page. So navigate to this page, amek visible the search rect */
    else {
        /** Going to the current page */
        mPageNavigation->setCurrentPage( page );
        makeRegionVisible( transformedRect, pageGeometry );
    }
}


void QDocumentViewImpl::highlightNextSearchInstance() {
    /** Not the last search rect of the search page */
    if ( curSearchRect != searchRects[ searchPage ].last() ) {
        int idx = searchRects[ searchPage ].indexOf( curSearchRect );

        /** Update the highlighted rect */
        curSearchRect = searchRects[ searchPage ][ idx + 1 ];
    }

    /** Last search rect od the search page => Go to next page. */
    else {
        QList<int> searchPages = searchRects.keys();
        std::sort( searchPages.begin(), searchPages.end() );

        /** Last page of search */
        if ( searchPage == searchPages.last() ) {
            /** Go to first search page */
            searchPage = searchPages.first();
        }

        else {
            /** Go to next search page */
            searchPage = searchPages[ searchPages.indexOf( searchPage ) + 1 ];
        }

        /** Update the highlighted rect: First rect of the searchPage */
        curSearchRect = searchRects[ searchPage ].first();
    }

    /** Go to that page if we're not already there */
    if ( mDocState.currentPage != searchPage ) {
        mPageNavigation->setCurrentPage( searchPage );
    }

    /** Make the rectangle visible */
    QRectF pageGeometry    = mDocumentLayout.pageGeometries[ searchPage ];
    QRectF transformedRect = getTransformedRect( curSearchRect, searchPage, false );
    makeRegionVisible( transformedRect, pageGeometry );
}


void QDocumentViewImpl::highlightPreviousSearchInstance() {
    /** Not the last search rect of the search page */
    if ( curSearchRect != searchRects[ searchPage ].first() ) {
        int idx = searchRects[ searchPage ].indexOf( curSearchRect );

        /** Update the highlighted rect */
        curSearchRect = searchRects[ searchPage ][ idx - 1 ];
    }

    /** Last search rect od the search page => Go to next page. */
    else {
        QList<int> searchPages = searchRects.keys();
        std::sort( searchPages.begin(), searchPages.end() );

        /** First page of search */
        if ( searchPage == searchPages.first() ) {
            /** Go to first search page */
            searchPage = searchPages.last();
        }

        else {
            /** Go to next search page */
            searchPage = searchPages[ searchPages.indexOf( searchPage ) - 1 ];
        }

        /** Update the highlighted rect: First rect of the searchPage */
        curSearchRect = searchRects[ searchPage ].last();
    }

    /** Go to that page if we're not already there */
    if ( mDocState.currentPage != searchPage ) {
        mPageNavigation->setCurrentPage( searchPage );
    }

    /** Make the rectangle visible */
    QRectF pageGeometry    = mDocumentLayout.pageGeometries[ searchPage ];
    QRectF transformedRect = getTransformedRect( curSearchRect, searchPage, false );
    makeRegionVisible( transformedRect, pageGeometry );
}


void QDocumentViewImpl::highlightSearchInstanceInCurrentPage() {
}


QPair<int, int> QDocumentViewImpl::getCurrentSearchPosition() {
    int idx = 0;
    int total = 0;

    /** We're counting the total search items */
    for( int pg: searchRects.keys() ) {
        for( QRectF rect: searchRects[ pg ] ) {
            total++;

            /** If (searchPage, curSearchRect) == (pg, rect), then this is the current search index */
            if ( ( searchPage == pg ) and ( rect == curSearchRect ) ) {
                idx = total;
            }
        }
    }

    return qMakePair( idx, total );
}


void QDocumentViewImpl::paintOverlayRects( int page, QImage& img ) {
    /** Search Rects */
    if ( searchRects.contains( page ) ) {
        QColor hBrush = qApp->palette().color( QPalette::Highlight );
        hBrush.setAlphaF( 0.50 );

        if ( not mDocumentLayout.pageGeometries.contains( page ) ) {
            return;
        }

        /** Page rectangle */
        QRectF pgRect = mDocumentLayout.pageGeometries[ page ];

        if ( pgRect.isNull() ) {
            return;
        }

        /** The rectangles are too small. Buff them up. */
        double xPad = 0.0;
        double yPad = 0.0;

        /** Document Page Size */
        QSizeF dPageSize = mDocument->pageSize( page );

        /** Rendered Page Size */
        QSizeF rPageSize = pgRect.size();

        switch ( mRenderOpts.rotation() ) {
            case QDocumentRenderOptions::Rotate90:
            case QDocumentRenderOptions::Rotate270: {
                dPageSize.transpose();
            }

            default: {
                break;
            }
        }

        /** Layout not done. */
        if ( not (rPageSize.width() and rPageSize.height() ) ) {
            return;
        }

        QPainter painter( &img );

        painter.save();
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setCompositionMode( QPainter::CompositionMode_Darken );

        for (QRectF rect: searchRects.value( page ) ) {
            xPad = 2.0;
            yPad = 3.0;

            /** If this is the current rect, draw a border. Otherwise, Don't */
            if ( (page == mPageNavigation->currentPage() ) and (rect == curSearchRect) ) {
                painter.setPen( qApp->palette().color( QPalette::Highlight ) );
                painter.setBrush( qApp->palette().color( QPalette::Link ) );
            }

            else {
                painter.setPen( Qt::NoPen );
                painter.setBrush( hBrush );
            }

            /** Transform the rectangle: handle rotations */
            QRectF transformedRect = getTransformedRect( rect, page, false );

            /** Paint the rectangle along with 2px padding */
            painter.drawRoundedRect( transformedRect.adjusted( -xPad, -yPad, xPad, yPad ), 2.0, 2.0 );
        }

        painter.restore();
        painter.end();
    }
}


QRectF QDocumentViewImpl::getTransformedRect( QRectF rect, int page, bool inverse ) {
    QRectF pgRect    = mDocumentLayout.pageGeometries[ page ];
    QSizeF dPageSize = mDocument->pageSize( page );
    QSizeF rPageSize = pgRect.size();

    double xZoom = rPageSize.width() / dPageSize.width();
    double yZoom = rPageSize.height() / dPageSize.height();
    qreal  x = 0.0, y = 0.0, w = 0.0, h = 0.0;

    if ( inverse ) {
        switch ( mRenderOpts.rotation() ) {
            case QDocumentRenderOptions::Rotate0: {
                x = rect.x() / xZoom;
                y = rect.y() / yZoom;
                w = rect.width() / xZoom;
                h = rect.height() / yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate90: {
                x = pgRect.width() - rect.y() / xZoom - rect.height() / xZoom;
                y = rect.x() / yZoom;
                w = rect.height() / xZoom;
                h = rect.width() / yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate180: {
                x = pgRect.width() - rect.x() / xZoom - rect.width() / xZoom;
                y = pgRect.height() - rect.y() / yZoom - rect.height() / yZoom;
                w = rect.width() / xZoom;
                h = rect.height() / yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate270: {
                x = rect.y() / xZoom;
                y = pgRect.height() - rect.x() / yZoom - rect.width() / yZoom;
                w = rect.height() / xZoom;
                h = rect.width() / yZoom;

                break;
            }

            default: {
                break;
            }
        }
    }

    /** Forward transformation */
    else {
        switch ( mRenderOpts.rotation() ) {
            case QDocumentRenderOptions::Rotate0: {
                x = rect.x() * xZoom;
                y = rect.y() * yZoom;
                w = rect.width() * xZoom;
                h = rect.height() * yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate90: {
                x = pgRect.width() - rect.y() * xZoom - rect.height() * xZoom;
                y = rect.x() * yZoom;
                w = rect.height() * xZoom;
                h = rect.width() * yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate180: {
                x = pgRect.width() - rect.x() * xZoom - rect.width() * xZoom;
                y = pgRect.height() - rect.y() * yZoom - rect.height() * yZoom;
                w = rect.width() * xZoom;
                h = rect.height() * yZoom;

                break;
            }

            case QDocumentRenderOptions::Rotate270: {
                x = rect.y() * xZoom;
                y = pgRect.height() - rect.x() * yZoom - rect.width() * yZoom;
                w = rect.height() * xZoom;
                h = rect.width() * yZoom;

                break;
            }

            default: {
                break;
            }
        }
    }

    return QRectF( QPointF( x, y ), QSizeF( w, h ) );
}


void QDocumentViewImpl::makeRegionVisible( QRectF region, QRectF pageGeometry ) {
    /** The search rect is hiding above the viewport. */
    if ( region.y() + pageGeometry.y() < mViewPort.y() ) {
        /** Scroll up until we see the search rect */
        while ( region.y() + pageGeometry.y() <= mViewPort.y() ) {
            publ->verticalScrollBar()->setValue( publ->verticalScrollBar()->value() - 1 );
        }
    }

    /** The search rect is hiding below the viewport. */
    if ( region.y() + pageGeometry.y() + region.height() > mViewPort.y() + mViewPort.height() ) {
        /** Scroll up until we see the search rect */
        while ( region.y() + pageGeometry.y() + region.height() >= mViewPort.y() ) {
            publ->verticalScrollBar()->setValue( publ->verticalScrollBar()->value() + 1 );
        }
    }

    /** The search rect is hiding left of the viewport. */
    if ( region.x() + pageGeometry.x() < mViewPort.x() ) {
        /** Scroll up until we see the search rect */
        while ( region.x() + pageGeometry.x() <= mViewPort.x() ) {
            publ->horizontalScrollBar()->setValue( publ->horizontalScrollBar()->value() - 1 );
        }
    }

    /** The search rect is hiding right of the viewport. */
    if ( region.x() + pageGeometry.x() + region.width() > mViewPort.x() + mViewPort.width() ) {
        /** Scroll up until we see the search rect */
        while ( region.x() + pageGeometry.x() + region.width() >= mViewPort.x() ) {
            publ->horizontalScrollBar()->setValue( publ->horizontalScrollBar()->value() - 1 );
        }
    }
}


void QDocumentViewImpl::makePointVisible( QPointF pt, QRectF pageGeometry ) {
    /** The search rect is hiding above the viewport. */
    if ( pt.y() + pageGeometry.y() < mViewPort.y() ) {
        /** Scroll up until we see the search rect */
        while ( pt.y() + pageGeometry.y() <= mViewPort.y() ) {
            publ->verticalScrollBar()->setValue( publ->verticalScrollBar()->value() - 1 );
        }
    }

    /** The search rect is hiding below the viewport. */
    if ( pt.y() + pageGeometry.y() > mViewPort.y() + mViewPort.height() ) {
        /** Scroll up until we see the search rect */
        while ( pt.y() + pageGeometry.y() >= mViewPort.y() ) {
            publ->verticalScrollBar()->setValue( publ->verticalScrollBar()->value() + 1 );
        }
    }

    /** The search rect is hiding left of the viewport. */
    if ( pt.x() + pageGeometry.x() < mViewPort.x() ) {
        /** Scroll up until we see the search rect */
        while ( pt.x() + pageGeometry.x() <= mViewPort.x() ) {
            publ->horizontalScrollBar()->setValue( publ->horizontalScrollBar()->value() - 1 );
        }
    }

    /** The search rect is hiding right of the viewport. */
    if ( pt.x() + pageGeometry.x() > mViewPort.x() + mViewPort.width() ) {
        /** Scroll up until we see the search rect */
        while ( pt.x() + pageGeometry.x() >= mViewPort.x() ) {
            publ->horizontalScrollBar()->setValue( publ->horizontalScrollBar()->value() - 1 );
        }
    }
}
