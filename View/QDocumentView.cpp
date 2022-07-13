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
#include <qdocumentview/QDocumentSearch.hpp>
#include <qdocumentview/QDocument.hpp>
#include <qdocumentview/QDocumentNavigation.hpp>
#include <qdocumentview/PopplerDocument.hpp>
#include <qdocumentview/DjVuDocument.hpp>

#include "ViewImpl.hpp"
#include "ViewWidgets.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QScroller>

QDocumentView::QDocumentView( QWidget *parent ) : QAbstractScrollArea( parent ) {
    impl = new QDocumentViewImpl( this );
    impl->init();

    /** Piggy-back the matchesFound, and searchComplete signals from the search thread */
    connect( impl->m_searchThread, &QDocumentSearch::matchesFound,   this, &QDocumentView::matchesFound );
    connect( impl->m_searchThread, &QDocumentSearch::searchComplete, this, &QDocumentView::searchComplete );

    connect(
        impl->m_searchThread, &QDocumentSearch::resultsReady, [ this ]( int page ) {
            impl->searchRects[ page ] = impl->m_searchThread->results( page );
            viewport()->update();
        }
    );

    connect(
        impl->m_searchThread, &QDocumentSearch::resultsReady, [ this ]( int page ) {
            impl->searchRects[ page ] = impl->m_searchThread->results( page );
            viewport()->update();
        }
    );

    /* Setup Page Navigation */
    connect(
        impl->m_pageNavigation, &QDocumentNavigation::currentPageChanged, this, [ this ]( int page ){
            impl->currentPageChanged( page );
        }
    );

    /* Setup Page Renderer */
    connect(
        impl->m_pageRenderer, &QDocumentRenderer::pageRendered, [ = ]( int ) {
            viewport()->update();
        }
    );

    /* Zoom buttons */
    mZoomBtn = new Zoom( this );

    if ( impl->m_zoomMode == CustomZoom ) {
        mZoomBtn->show();
        mZoomBtn->setEnlargeEnabled( impl->m_zoomFactor < 4.0 ? true : false );
        mZoomBtn->setDwindleEnabled( impl->m_zoomFactor > 0.1 ? true : false );
    }

    else {
        mZoomBtn->hide();
    }

    connect(
        mZoomBtn, &Zoom::clicked, [ = ]( QString action ) {
            if ( action == "enlarge" ) {
                /**
                 * Check if the next zoom factor is greater than zoomFactor for FitInView
                 * If yes, set the zoom to Fit the page in view.
                 */
                if ( false ) {
                    //
                }

                /** Check if the next zoom factor is greater than zoomFactor for FitWidth */
                else if ( false ) {
                    //
                }

                else {
                    setZoomFactor( impl->m_zoomFactor * 1.10 );
                }
            }

            else {
                /**
                 * Check if the next zoom factor is greater than zoomFactor for FitInView
                 * If yes, set the zoom to Fit the page in view.
                 */
                if ( false ) {
                    //
                }

                /** Check if the next zoom factor is greater than zoomFactor for FitWidth */
                else if ( false ) {
                    //
                }

                else {
                    setZoomFactor( impl->m_zoomFactor / 1.10 );
                }
            }

            mZoomBtn->setEnlargeEnabled( impl->m_zoomFactor < 4.0 ? true : false );
            mZoomBtn->setDwindleEnabled( impl->m_zoomFactor > 0.1 ? true : false );
        }
    );

    /* Page buttons */
    mPagesBtn = new PageWidget( this );
    connect( impl->m_pageNavigation, &QDocumentNavigation::currentPageChanged, mPagesBtn,              &PageWidget::setCurrentPage );
    connect( mPagesBtn,              &PageWidget::loadPage,                    impl->m_pageNavigation, &QDocumentNavigation::setCurrentPage );

    /* ProgressBar */
    progress = new QProgressBar( this );
    progress->move( 5, 5 );
    progress->setFixedSize( 50, 10 );
    progress->setRange( 0, 100 );
    progress->setStyle( QStyleFactory::create( "fusion" ) );
    progress->setFormat( "" );

    mZoomBtn->hide();
    mPagesBtn->hide();
    progress->hide();

    /* Does this work? */
    verticalScrollBar()->setSingleStep( 20 );
    horizontalScrollBar()->setSingleStep( 20 );

    /* First load */
    impl->calculateViewport();

    /* Don't draw a frame around the QScrollArea */
    setFrameStyle( QFrame::NoFrame );

    /* 100% Zoom */
    QShortcut *shortcut = new QShortcut( this );

    shortcut->setKey( QKeySequence( Qt::CTRL + Qt::Key_0 ) );
    connect(
        shortcut, &QShortcut::activated, [ = ]() {
            setZoomFactor( 1.0 );
        }
    );
}


QDocumentView::~QDocumentView() {
    delete mZoomBtn;
    delete mPagesBtn;
    delete progress;
    delete impl;
}


void QDocumentView::load( QString path ) {
    QDocument *doc;

    if ( path.toLower().endsWith( "pdf" ) ) {
        doc = new PopplerDocument( path );
    }

    else if ( path.toLower().endsWith( "djv" ) or path.toLower().endsWith( "djvu" ) ) {
        doc = new DjVuDocument( path );
    }

    else {
        qDebug() << "Unknown document type:" << path;
        return;
    }

    progress->show();
    connect(
        doc, &QDocument::loading, [ = ]( int pc ) {
            progress->setValue( pc );

            if ( pc == 100 ) {
                progress->hide();
            }

            qApp->processEvents();
        }
    );

    doc->load();

    switch ( doc->error() ) {
        case QDocument::NoError: {
            break;
        }

        case QDocument::IncorrectPasswordError: {
            bool ok    = true;
            int  count = 0;
            do {
                QString passwd = QInputDialog::getText(
                    this,
                    "Encrypted Document",
                    QString( "%1Please enter the document password:" ).arg( count ? "You may have entered the wrong password.<br>" : "" ),
                    QLineEdit::Password,
                    QString(),
                    &ok,
                    Qt::WindowFlags(),
                    Qt::ImhSensitiveData
                );

                doc->setPassword( passwd );
                count++;
            } while ( ok == true and doc->passwordNeeded() );

            /* User cancelled loading the document */
            if ( not ok ) {
                return;
            }
        }

        default: {
            return;
        }
    }

    /* Password has been supplied (if needed), document ready to be loaded */
    setDocument( doc );
}


void QDocumentView::setDocument( QDocument *document ) {
    if ( impl->m_document == document ) {
        return;
    }

    if ( impl->m_document ) {
        disconnect( impl->m_documentStatusChangedConnection );
        disconnect( impl->m_reloadDocumentConnection );
    }

    impl->m_document = document;
    emit documentChanged( impl->m_document );

    if ( impl->m_document ) {
        impl->m_documentStatusChangedConnection = connect(
            impl->m_document, &QDocument::statusChanged, [ = ]( QDocument::Status sts ) {
                impl->documentStatusChanged();

                if ( sts == QDocument::Loading ) {
                    progress->show();
                }

                else {
                    progress->hide();
                }
            }
        );

        impl->m_reloadDocumentConnection = connect(
            impl->m_document, &QDocument::reloadDocument, [ this ]() {
                impl->m_pageRenderer->reload();
            }
        );
    }

    impl->m_pageNavigation->setDocument( impl->m_document );
    impl->m_pageRenderer->setDocument( impl->m_document );

    impl->documentStatusChanged();

    mPagesBtn->setMaximumPages( document->pageCount() );
    mPagesBtn->setCurrentPage( impl->m_pageNavigation->currentPage() );

    if ( mShowZoom ) {
        mZoomBtn->show();
    }

    if ( mShowPages ) {
        mPagesBtn->show();
    }

    progress->hide();
}


QDocument *QDocumentView::document() const {
    return impl->m_document;
}


QDocumentNavigation *QDocumentView::pageNavigation() const {
    return impl->m_pageNavigation;
}


bool QDocumentView::isLayoutContinuous() const {
    return impl->m_continuous;
}


void QDocumentView::setLayoutContinuous( bool yes ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_continuous == yes ) {
        return;
    }

    impl->m_continuous = yes;
    impl->invalidateDocumentLayout();

    emit layoutContinuityChanged( yes );
}


QDocumentView::PageLayout QDocumentView::pageLayout() const {
    return impl->m_pageLayout;
}


void QDocumentView::setPageLayout( PageLayout lyt ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_pageLayout == lyt ) {
        return;
    }

    impl->m_pageLayout = lyt;
    impl->invalidateDocumentLayout();

    emit pageLayoutChanged( impl->m_pageLayout );
}


QDocumentView::ZoomMode QDocumentView::zoomMode() const {
    return impl->m_zoomMode;
}


void QDocumentView::setZoomMode( ZoomMode mode ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_zoomMode == mode ) {
        return;
    }

    impl->m_zoomMode = mode;
    impl->invalidateDocumentLayout();

    if ( impl->m_zoomMode == CustomZoom ) {
        mZoomBtn->show();
    }

    else {
        mZoomBtn->hide();
    }

    emit zoomModeChanged( impl->m_zoomMode );
}


qreal QDocumentView::zoomFactor() const {
    return impl->zoomFactor();
}


void QDocumentView::setRenderOptions( QDocumentRenderOptions opts ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_renderOpts == opts ) {
        return;
    }

    impl->m_renderOpts = opts;
    impl->invalidateDocumentLayout();

    emit renderOptionsChanged( impl->m_renderOpts );
}


QDocumentRenderOptions QDocumentView::renderOptions() const {
    return impl->m_renderOpts;
}


void QDocumentView::setZoomFactor( qreal factor ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_zoomFactor == factor ) {
        return;
    }

    if ( factor > 4.0 ) {
        factor = 4.0;
    }

    if ( factor < 0.1 ) {
        factor = 0.1;
    }

    impl->m_zoomFactor = factor;
    impl->invalidateDocumentLayout();

    emit zoomFactorChanged( impl->m_zoomFactor );
}


int QDocumentView::pageSpacing() const {
    return impl->m_pageSpacing;
}


void QDocumentView::setPageSpacing( int spacing ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_pageSpacing == spacing ) {
        return;
    }

    impl->m_pageSpacing = spacing;
    impl->invalidateDocumentLayout();

    emit pageSpacingChanged( impl->m_pageSpacing );
}


QMargins QDocumentView::documentMargins() const {
    return impl->m_documentMargins;
}


void QDocumentView::setDocumentMargins( QMargins margins ) {
    if ( not impl->m_document ) {
        return;
    }

    if ( impl->m_documentMargins == margins ) {
        return;
    }

    impl->m_documentMargins = margins;
    impl->invalidateDocumentLayout();

    emit documentMarginsChanged( impl->m_documentMargins );
}


void QDocumentView::searchText( QString str ) {
    impl->searchRects.clear();

    impl->m_searchThread->setSearchString( str );
    impl->m_searchThread->searchPage( impl->m_pageNavigation->currentPage() );
}


bool QDocumentView::showPagesOSD() const {
    return mShowPages;
}


void QDocumentView::setShowPagesOSD( bool yes ) {
    mShowPages = yes;

    if ( not impl->m_document ) {
        return;
    }

    if ( yes ) {
        mPagesBtn->show();
    }

    else {
        mPagesBtn->hide();
    }
}


bool QDocumentView::showZoomOSD() const {
    return mShowZoom;
}


void QDocumentView::setShowZoomOSD( bool yes ) {
    mShowZoom = yes;

    if ( not impl->m_document ) {
        return;
    }

    if ( yes ) {
        mZoomBtn->show();
    }

    else {
        mZoomBtn->hide();
    }
}


void QDocumentView::paintEvent( QPaintEvent *event ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        QAbstractScrollArea::paintEvent( event );
        return;
    }

    QPainter painter( viewport() );

    painter.fillRect( event->rect(), palette().brush( QPalette::Dark ) );
    painter.translate( -impl->m_viewport.x(), -impl->m_viewport.y() );

    for ( auto it = impl->m_documentLayout.pageGeometries.cbegin(); it != impl->m_documentLayout.pageGeometries.cend(); ++it ) {
        const QRect pageGeometry = it.value();

        if ( pageGeometry.intersects( impl->m_viewport ) ) {
            painter.fillRect( pageGeometry, Qt::white );

            const int page = it.key();
            QImage    img  = impl->m_pageRenderer->requestPage( page, pageGeometry.size(), impl->m_renderOpts );

            if ( img.width() and img.height() ) {
                impl->paintSearchRects( page, img );
                painter.drawImage( pageGeometry.topLeft(), img );
            }
        }
    }
}


void QDocumentView::resizeEvent( QResizeEvent *event ) {
    QAbstractScrollArea::resizeEvent( event );
    qApp->processEvents();

    if ( impl->m_document ) {
        if ( mZoomBtn and mZoomBtn->isVisible() ) {
            mZoomBtn->move( 5, viewport()->height() - mZoomBtn->height() - 5 );
        }

        if ( mPagesBtn and mPagesBtn->isVisible() ) {
            mPagesBtn->move( viewport()->width() - mPagesBtn->width() - 5, viewport()->height() - mPagesBtn->height() - 5 );
        }

        impl->updateScrollBars();

        qApp->processEvents();

        if ( impl->pendingResize ) {
            return;
        }

        impl->pendingResize = true;
        QTimer::singleShot(
            250, [ this ]() {
                impl->calculateViewport();
                impl->pendingResize = false;
                viewport()->update();
            }
        );
    }
}


void QDocumentView::scrollContentsBy( int dx, int dy ) {
    QAbstractScrollArea::scrollContentsBy( dx, dy );

    impl->calculateViewport();
}


void QDocumentView::keyPressEvent( QKeyEvent *kEvent ) {
    switch ( kEvent->key() ) {
        case Qt::Key_Right: {
            /* Go to next page */
            impl->m_pageNavigation->setCurrentPage( impl->m_pageNavigation->currentPage() + 1 );
            break;
        }

        case Qt::Key_Left: {
            /* Go to previous page */
            impl->m_pageNavigation->setCurrentPage( impl->m_pageNavigation->currentPage() - 1 );
            break;
        }

        case Qt::Key_Space: {
            /* Move by approximately 90% of the viewport height */
            verticalScrollBar()->setValue( verticalScrollBar()->value() + viewport()->height() * 0.9 );
            break;
        }

        case Qt::Key_Home: {
            /* Go to first page */
            verticalScrollBar()->setValue( 0 );
            break;
        }

        case Qt::Key_End: {
            /* Go to end of the document */
            verticalScrollBar()->setValue( verticalScrollBar()->maximum() );
            break;
        }

        case Qt::Key_Plus: {
            /* Zoom In */
            setZoomFactor( impl->m_zoomFactor * 1.10 );
            break;
        }

        case Qt::Key_Minus: {
            /* Zoom Out */
            setZoomFactor( impl->m_zoomFactor / 1.10 );
            break;
        }

        default: {
            /* Default action */
            QAbstractScrollArea::keyPressEvent( kEvent );
            break;
        }
    }
}


void QDocumentView::wheelEvent( QWheelEvent *wEvent ) {
    if ( wEvent->modifiers() & Qt::ControlModifier ) {
        QPoint numDegrees = wEvent->angleDelta() / 8;

        int steps = numDegrees.y() / 15;

        if ( steps > 0 ) {
            setZoomFactor( impl->m_zoomFactor * 1.10 );
        }

        else {
            setZoomFactor( impl->m_zoomFactor / 1.10 );
        }

        return;
    }

    QAbstractScrollArea::wheelEvent( wEvent );
}
