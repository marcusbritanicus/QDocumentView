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
    publ                 = view;                        // Pointer to the public class
    m_continuous         = true;                        // Continuous layout
    m_pageLayout         = QDocumentView::SinglePage;   // Draw only one page (single column)
    m_zoomMode           = QDocumentView::CustomZoom;   // Start with a custom zoom of 1.0
    m_zoomFactor         = 1.0;                         // Zoom factor for custom zoom
    m_pageSpacing        = 3;                           // Default space between two pages
    m_documentMargins    = QMargins( 6, 6, 6, 6 );      // Page margins
    m_blockPageScrolling = false;                       // Flag to handle setting current page
    m_screenResolution   = QGuiApplication::primaryScreen()->logicalDotsPerInch() / 72.0;

    m_pageNavigation = new QDocumentNavigation( view );
    m_pageRenderer   = new QDocumentRenderer( view );
    m_searchThread   = new QDocumentSearch( view );
}


QDocumentViewImpl::~QDocumentViewImpl() {
    delete m_document;
    delete m_pageNavigation;
    delete m_pageRenderer;

    m_searchThread->stop();
    delete m_searchThread;
}


void QDocumentViewImpl::currentPageChanged( int currentPage ) {
    if ( m_blockPageScrolling ) {
        return;
    }

    publ->verticalScrollBar()->setValue( yPositionForPage( currentPage ) );

    if ( m_pageLayout == QDocumentView::SinglePage ) {
        invalidateDocumentLayout();
    }
}


void QDocumentViewImpl::calculateViewport() {
    if ( not m_document ) {
        return;
    }

    const int x      = publ->horizontalScrollBar()->value();
    const int y      = publ->verticalScrollBar()->value();
    const int width  = publ->viewport()->width();
    const int height = publ->viewport()->height();

    setViewport( QRect( x, y, width, height ) );
}


void QDocumentViewImpl::setViewport( QRect viewport ) {
    if ( m_viewport == viewport ) {
        return;
    }

    const QSize oldSize = m_viewport.size();

    m_viewport = viewport;

    if ( oldSize != m_viewport.size() ) {
        updateDocumentLayout();
    }

    if ( m_continuous ) {
        // An imaginary, 2px height line at the upper half of the viewport, which is used to
        // determine which page is currently located there -> we propagate that as 'current' page
        // to the QDocumentNavigation object
        const QRect currentPageLine( m_viewport.x(), m_viewport.y() + m_viewport.height() * 0.4, m_viewport.width(), 2 );

        int currentPage = 0;
        for ( auto it = m_documentLayout.pageGeometries.cbegin(); it != m_documentLayout.pageGeometries.cend(); ++it ) {
            const QRect pageGeometry = it.value();

            if ( pageGeometry.intersects( currentPageLine ) ) {
                currentPage = it.key();
                break;
            }
        }

        if ( currentPage != m_pageNavigation->currentPage() ) {
            m_blockPageScrolling = true;
            m_pageNavigation->setCurrentPage( currentPage );
            m_blockPageScrolling = false;
        }
    }
}


void QDocumentViewImpl::updateScrollBars() {
    const QSize p = publ->viewport()->size();
    const QSize v = m_documentLayout.documentSize;

    publ->horizontalScrollBar()->setRange( 0, v.width() - p.width() );
    publ->horizontalScrollBar()->setPageStep( p.width() );
    publ->verticalScrollBar()->setRange( 0, v.height() - p.height() );
    publ->verticalScrollBar()->setPageStep( p.height() );
}


void QDocumentViewImpl::invalidateDocumentLayout() {
    updateDocumentLayout();
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayout() const {
    switch ( m_pageLayout ) {
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

    if ( !m_document || (m_document->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = m_document->pageCount() - 1;

    int totalWidth = 0;

    const int startPage = (m_continuous ? 0         : m_pageNavigation->currentPage() );
    const int endPage   = (m_continuous ? pageCount : m_pageNavigation->currentPage() );

    const int horizMargins = m_documentMargins.left() + m_documentMargins.right();
    int       pageY        = m_documentMargins.top();

    // calculate page sizes
    for (int page = startPage; page <= endPage; ++page) {
        QSizeF pageSize = m_document->pageSize( page ) * m_screenResolution;

        switch ( m_renderOpts.rotation() ) {
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

        if ( m_zoomMode == QDocumentView::CustomZoom ) {
            pageSize *= m_zoomFactor;
        }

        else if ( m_zoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = (qreal( m_viewport.width() - horizMargins ) / qreal( pageSize.width() ) );
            pageSize *= factor;
        }

        else if ( m_zoomMode == QDocumentView::FitInView ) {
            const int   margins = (m_continuous ? 0 : m_documentMargins.bottom() + m_documentMargins.top() - m_pageSpacing);
            const QSize viewportSize( m_viewport.size() - QSize( horizMargins, m_pageSpacing + margins ) );

            pageSize += QSizeF( 0, m_pageSpacing );
            pageSize  = pageSize.scaled( viewportSize, Qt::KeepAspectRatio );
        }

        totalWidth             = qMax( totalWidth, pageSize.toSize().width() + horizMargins );
        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), pageSize.toSize() );
    }

    // calculate page positions
    for (int page = startPage; page <= endPage; ++page) {
        const QSize pageSize = pageGeometries[ page ].size();

        // center horizontal inside the viewport
        const int pageX = (qMax( totalWidth, m_viewport.width() ) - pageSize.width() ) / 2;

        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        pageY += pageSize.height() + m_pageSpacing;
    }

    /** We added an amount 'm_pageSpacing' amount extra */
    pageY += m_documentMargins.bottom() - m_pageSpacing;

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize( totalWidth, pageY );

    return documentLayout;
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutFacing() const {
    DocumentLayout documentLayout;

    if ( !m_document || (m_document->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = m_document->pageCount() - 1;
    const int curPage   = m_pageNavigation->currentPage();

    int totalWidth = 0;

    int startPage = 0;
    int endPage   = pageCount;

    if ( not m_continuous ) {
        if ( curPage % 2 == 0 ) {
            startPage = curPage;
            endPage   = curPage;
        }

        else {
            startPage = curPage - 1;
            endPage   = curPage - 1;
        }
    }

    const int horizMargins = m_documentMargins.left() + m_documentMargins.right();

    // calculate page sizes
    for (int page = startPage; page <= endPage; page += 2) {
        QSizeF pageSize;

        QSizeF p1 = m_document->pageSize( page ) * m_screenResolution;
        QSizeF p2 = m_document->pageSize( page + 1 ) * m_screenResolution;

        switch ( m_renderOpts.rotation() ) {
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

        if ( m_zoomMode == QDocumentView::CustomZoom ) {
            pageSize *= m_zoomFactor;
            p1       *= m_zoomFactor;
            p2       *= m_zoomFactor;
            pageSize += QSizeF( m_pageSpacing, 0 );
        }

        else if ( m_zoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = qreal( m_viewport.width() - horizMargins - m_pageSpacing ) / qreal( pageSize.width() );
            pageSize *= factor;
            p1       *= (factor / (p2.isValid() ? 1.0 : 2.0) );
            p2       *= factor;

            if ( p2.isValid() ) {
                pageSize += QSizeF( m_pageSpacing, 0 );
            }
        }

        else if ( m_zoomMode == QDocumentView::FitInView ) {
            const int   margins = (m_continuous ? 0 : m_documentMargins.bottom() + m_documentMargins.top() - m_pageSpacing);
            const QSize viewportSize( m_viewport.size() - QSize( horizMargins, m_pageSpacing + margins ) );

            pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio ) + QSizeF( m_pageSpacing, 0 );

            if ( not p2.isValid() ) {
                p1 = QSizeF( pageSize.width(), pageSize.height() );
            }

            else {
                p1        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                p2        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                pageSize += QSizeF( m_pageSpacing, 0 );
            }
        }

        totalWidth = qMax( totalWidth, pageSize.toSize().width() + horizMargins );

        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), p1.toSize() );

        if ( not not p2.isValid() ) {
            pageGeometries[ page + 1 ] = QRect( QPoint( 0, 0 ), p2.toSize() );
        }
    }

    int pageY = m_documentMargins.top();

    // calculate page positions
    for (int page = startPage; page <= endPage; page += 2) {
        const QSize size1 = pageGeometries.value( page ).size();
        const QSize size2 = pageGeometries.value( page + 1 ).size();

        const int pageHeight = qMax( size1.height(), size2.height() );

        /** center horizontal inside the viewport */
        const int pageX = (qMax( totalWidth, m_viewport.width() ) - size1.width() - (size2.isNull() ? 0 : size2.width() + m_pageSpacing) ) / 2;
        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        if ( size2.isValid() ) {
            pageGeometries[ page + 1 ].moveTopLeft( QPoint( pageX + size1.width() + m_pageSpacing, pageY ) );
        }

        pageY += pageHeight + m_pageSpacing;
    }

    pageY += m_documentMargins.bottom() - m_pageSpacing;

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize( totalWidth, pageY );

    return documentLayout;
}


QDocumentViewImpl::DocumentLayout QDocumentViewImpl::calculateDocumentLayoutBook() const {
    DocumentLayout documentLayout;

    if ( !m_document || (m_document->status() != QDocument::Ready) ) {
        return documentLayout;
    }

    QHash<int, QRect> pageGeometries;

    const int pageCount = m_document->pageCount() - 1;
    const int curPage   = m_pageNavigation->currentPage();

    int totalWidth = 0;

    int startPage = 0;
    int endPage   = pageCount;

    if ( not m_continuous ) {
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

    const int horizMargins = m_documentMargins.left() + m_documentMargins.right();

    // calculate page sizes
    /** First page */
    if ( startPage == 0 ) {
        QSizeF p1 = m_document->pageSize( 0 ) * m_screenResolution;

        switch ( m_renderOpts.rotation() ) {
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

        if ( m_zoomMode == QDocumentView::CustomZoom ) {
            p1 *= m_zoomFactor;
        }

        else if ( m_zoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = (qreal( m_viewport.width() - horizMargins ) / qreal( 2 * p1.width() + m_pageSpacing ) );
            p1 *= factor;
        }

        else if ( m_zoomMode == QDocumentView::FitInView ) {
            const int   margins = (m_continuous ? 0 : m_documentMargins.bottom() + m_documentMargins.top() - m_pageSpacing);
            const QSize viewportSize( m_viewport.size() - QSize( horizMargins, m_pageSpacing + margins ) );

            p1 = p1.scaled( viewportSize, Qt::KeepAspectRatio );
        }

        totalWidth          = qMax( totalWidth, p1.toSize().width() + horizMargins );
        pageGeometries[ 0 ] = QRect( QPoint( 0, 0 ), p1.toSize() );
    }

    for (int page = (startPage == 0 ? 1 : startPage); page <= endPage; page += 2) {
        QSizeF pageSize;

        QSizeF p1 = m_document->pageSize( page ) * m_screenResolution;
        QSizeF p2( 0, 0 );

        if ( page + 1 < m_document->pageCount() ) {
            p2 = m_document->pageSize( page + 1 ) * m_screenResolution;
        }

        switch ( m_renderOpts.rotation() ) {
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

        if ( m_zoomMode == QDocumentView::CustomZoom ) {
            pageSize *= m_zoomFactor;
            p1       *= m_zoomFactor;
            p2       *= m_zoomFactor;

            if ( not not p2.isValid() ) {
                pageSize += QSizeF( m_pageSpacing, 0 );
            }
        }

        else if ( m_zoomMode == QDocumentView::FitToWidth ) {
            const qreal factor = qreal( m_viewport.width() - horizMargins - m_pageSpacing ) / qreal( pageSize.width() );
            pageSize *= factor;
            p1       *= (factor / (p2.width() <= 0 ? 2.0 : 1.0) );
            p2       *= factor;

            if ( not not p2.isValid() ) {
                pageSize += QSizeF( m_pageSpacing, 0 );
            }
        }

        else if ( m_zoomMode == QDocumentView::FitInView ) {
            const int   margins = (m_continuous ? 0 : m_documentMargins.bottom() + m_documentMargins.top() - m_pageSpacing);
            const QSize viewportSize( m_viewport.size() - QSize( horizMargins, m_pageSpacing + margins ) );

            pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio ) + (not not p2.isValid() ? QSizeF( m_pageSpacing, 0 ) : QSizeF( 0, 0 ) );

            if ( not p2.isValid() ) {
                p1 = QSizeF( pageSize.width(), pageSize.height() );
            }

            else {
                p1        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                p2        = QSizeF( pageSize.width() / 2.0, pageSize.height() );
                pageSize += QSizeF( m_pageSpacing, 0 );
            }
        }

        totalWidth = qMax( totalWidth, pageSize.toSize().width() + horizMargins );

        pageGeometries[ page ] = QRect( QPoint( 0, 0 ), p1.toSize() );

        if ( not not p2.isValid() ) {
            pageGeometries[ page + 1 ] = QRect( QPoint( 0, 0 ), p2.toSize() );
        }
    }

    int pageY = m_documentMargins.top();

    // calculate page positions
    if ( startPage == 0 ) {
        const QSize s1         = pageGeometries[ 0 ].size();
        const int   pageHeight = s1.height();
        const int   pageX      = (qMax( totalWidth, m_viewport.width() ) - s1.width() ) / 2;
        pageGeometries[ 0 ].moveTopLeft( QPoint( pageX, pageY ) );
        pageY += pageHeight + m_pageSpacing;
    }

    for (int page = (startPage == 0 ? 1 : startPage); page <= endPage; page += 2) {
        const QSize size1 = pageGeometries.value( page ).size();
        const QSize size2 = pageGeometries.value( page + 1 ).size();

        const int pageHeight = qMax( size1.height(), size2.height() );

        // center horizontal inside the viewport
        const int pageX = (qMax( totalWidth, m_viewport.width() ) - size1.width() - (size2.isNull() ? 0 : size2.width() + m_pageSpacing) ) / 2;

        pageGeometries[ page ].moveTopLeft( QPoint( pageX, pageY ) );

        if ( size2.isValid() ) {
            pageGeometries[ page + 1 ].moveTopLeft( QPoint( pageX + size1.width() + m_pageSpacing, pageY ) );
        }

        pageY += pageHeight + m_pageSpacing;
    }

    pageY += m_documentMargins.bottom() - m_pageSpacing;

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
    const auto it = m_documentLayout.pageGeometries.constFind( pageNumber );

    if ( it == m_documentLayout.pageGeometries.cend() ) {
        return 0.0;
    }

    return (*it).y();
}


void QDocumentViewImpl::updateDocumentLayout() {
    m_documentLayout = calculateDocumentLayout();

    updateScrollBars();
}


qreal QDocumentViewImpl::zoomFactor() const {
    QSize pageSize;

    if ( m_zoomMode == QDocumentView::CustomZoom ) {
        return m_zoomFactor;
    }

    else if ( m_zoomMode == QDocumentView::FitToWidth ) {
        return zoomFactorForFitWidth();
    }

    else if ( m_zoomMode == QDocumentView::FitInView ) {
        return zoomFactorForFitInView();
    }

    return 1.0;
}


qreal QDocumentViewImpl::zoomFactorForFitWidth() const {
    int page = m_pageNavigation->currentPage();

    QSize       pageSize = QSizeF( m_document->pageSize( page ) * m_screenResolution ).toSize();
    const qreal factor   = (qreal( m_viewport.width() - m_documentMargins.left() - m_documentMargins.right() ) / qreal( pageSize.width() ) );

    return factor;
}


qreal QDocumentViewImpl::zoomFactorForFitInView() const {
    int page = m_pageNavigation->currentPage();

    const QSize viewportSize( m_viewport.size() + QSize( -m_documentMargins.left() - m_documentMargins.right(), -m_pageSpacing ) );

    QSize pageSize = QSizeF( m_document->pageSize( page ) * m_screenResolution ).toSize();

    pageSize = pageSize.scaled( viewportSize, Qt::KeepAspectRatio );

    return pageSize.width() / m_document->pageSize( page ).width();
}


void QDocumentViewImpl::paintSearchRects( int page, QImage& img ) {
    /** Search Rects */
    if ( searchRects.contains( page ) ) {
        QColor hPen   = qApp->palette().color( QPalette::Highlight );
        QColor hBrush = QColor( hPen );
        hBrush.setAlphaF( 0.50 );

        if ( not m_documentLayout.pageGeometries.contains( page ) ) {
            return;
        }

        /** Page rectangle */
        QRectF pgRect = m_documentLayout.pageGeometries[ page ];

        if ( pgRect.isNull() ) {
            return;
        }

        /** The rectangles are too small. Buff them up. */
        double xPad = 0.0;
        double yPad = 0.0;

        /** Document Page Size */
        QSizeF dPageSize = m_document->pageSize( page );

        /** Rendered Page Size */
        QSizeF rPageSize = pgRect.size();

        switch ( m_renderOpts.rotation() ) {
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
        painter.setPen( hPen );
        painter.setBrush( hBrush );

        double xZoom = rPageSize.width() / dPageSize.width();
        double yZoom = rPageSize.height() / dPageSize.height();

        for (QRectF rect: searchRects.value( page ) ) {
            xPad = 2;
            yPad = 4;

            /** Transform the rectangle: handle rotations */
            qreal x = 0.0, y = 0.0, w = 0.0, h = 0.0;
            switch ( m_renderOpts.rotation() ) {
                case QDocumentRenderOptions::Rotate0: {
                    x = rect.x() * xZoom - xPad / 2.0;
                    y = rect.y() * yZoom - yPad / 2.0;
                    w = rect.width() * xZoom + 2 * xPad;
                    h = rect.height() * yZoom + yPad;

                    break;
                }

                case QDocumentRenderOptions::Rotate90: {
                    x = pgRect.width() - rect.y() * xZoom - rect.height() * xZoom - yPad / 2.0;
                    y = rect.x() * yZoom - xPad / 2.0;
                    w = rect.height() * xZoom + 2 * yPad;
                    h = rect.width() * yZoom + xPad;

                    break;
                }

                case QDocumentRenderOptions::Rotate180: {
                    x = pgRect.width() - rect.x() * xZoom - rect.width() * xZoom - xPad / 2.0;
                    y = pgRect.height() - rect.y() * yZoom - rect.height() * yZoom - yPad / 2.0;
                    w = rect.width() * xZoom + 2 * xPad;
                    h = rect.height() * yZoom + yPad;

                    break;
                }

                case QDocumentRenderOptions::Rotate270: {
                    x = rect.y() * xZoom - yPad / 2.0;
                    y = pgRect.height() - rect.x() * yZoom - rect.width() * yZoom - xPad / 2.0;
                    w = rect.height() * xZoom + 2 * yPad;
                    h = rect.width() * yZoom + xPad;

                    break;
                }

                default: {
                    break;
                }
            }

            /** Paint the rectangle */
            painter.drawRoundedRect( QRectF( QPointF( x, y ), QSizeF( w, h ) ), 2.0, 2.0 );
        }

        painter.restore();
        painter.end();
    }
}
