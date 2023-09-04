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
#include "ViewToolbar.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QScroller>

QDocumentView::QDocumentView( QWidget *parent ) : QAbstractScrollArea( parent ) {
    impl = new QDocumentViewImpl( this );

    /** Piggy-back the matchesFound, and searchComplete signals from the search thread */
    connect( impl->mSearchThread, &QDocumentSearch::matchesFound,   this, &QDocumentView::matchesFound );
    connect( impl->mSearchThread, &QDocumentSearch::searchComplete, this, &QDocumentView::searchComplete );

    /** Update our inbuilt toolbar */
    connect(
        impl->mSearchThread, &QDocumentSearch::matchesFound, [ = ] () {
            /** If we have more than one match, we can enable the search buttons */
        }
    );

    connect(
        impl->mSearchThread, &QDocumentSearch::resultsReady, [ this ]( int page, QVector<QRectF> results ) {
            /** Store the search results */
            impl->searchRects[ page ] = results;

            /** Highlight the first search rect from the current point */
            impl->highlightFirstSearchInstance( page, results );

            /** Now, we're ready for page repaints */
            viewport()->update();
        }
    );

    /* Setup Page Navigation */
    connect(
        impl->mPageNavigation, &QDocumentNavigation::currentPageChanged, this, [ this ]( int page ) {
            impl->currentPageChanged( page );

            /**
             * Reset the search upon scroll if it's running: Prioritize current page.
             * This will result in highlighting the first instance in the current page.
             */
            if ( impl->mSearchThread->isRunning() ) {
                impl->mSearchThread->searchPage( page );
            }

            /**
             * If search is already finished, highlight the search in the current page
             * If there is no search, do nothing.
             */
            else {
                impl->highlightSearchInstanceInCurrentPage();
            }
        }
    );

    /* Setup Page Renderer */
    connect(
        impl->mPageRenderer, &QDocumentRenderer::pageRendered, [ = ]( int ) {
            viewport()->update();
        }
    );

    /* ToolBar */
    toolBar = new ViewToolbar( this );

    /* Zoom buttons */
    if ( impl->mZoomMode == CustomZoom ) {
        toolBar->setZoomButtonsEnabled( impl->mZoomFactor < 4.0 ? true : false, impl->mZoomFactor > 0.1 ? true : false );
    }

    else {
        toolBar->setZoomButtonsEnabled( false, false );
    }

    /* Zoom buttons signals */
    connect(
        toolBar, &ViewToolbar::zoomClicked, [ = ]( QString action ) {
            if ( action == "enlarge" ) {
                /**
                 * Check if the next zoom factor is greater than zoomFactor for FitInView
                 * If yes, set the zoom to Fit the page in view.
                 */
                if ( impl->mZoomFactor * 1.10 > impl->zoomFactorForFitInView() ) {
                    setZoomFactor( impl->zoomFactorForFitInView() );
                }

                /** Check if the next zoom factor is greater than zoomFactor for FitWidth */
                else if ( impl->mZoomFactor * 1.10 > impl->zoomFactorForFitWidth() ) {
                    setZoomFactor( impl->zoomFactorForFitWidth() );
                }

                else {
                    setZoomFactor( impl->mZoomFactor * 1.10 );
                }
            }

            else {
                /**
                 * Check if the next zoom factor is greater than zoomFactor for FitInView
                 * If yes, set the zoom to Fit the page in view.
                 */
                if ( impl->mZoomFactor / 1.10 > impl->zoomFactorForFitInView() ) {
                    setZoomFactor( impl->zoomFactorForFitInView() );
                }

                /** Check if the next zoom factor is greater than zoomFactor for FitWidth */
                else if ( impl->mZoomFactor / 1.10 > impl->zoomFactorForFitWidth() ) {
                    setZoomFactor( impl->zoomFactorForFitWidth() );
                }

                else {
                    setZoomFactor( impl->mZoomFactor / 1.10 );
                }
            }

            toolBar->setZoomButtonsEnabled( impl->mZoomFactor < 4.0 ? true : false, impl->mZoomFactor > 0.1 ? true : false );
        }
    );

    /* Page buttons signals */
    connect( impl->mPageNavigation, &QDocumentNavigation::currentPageChanged, toolBar, &ViewToolbar::setCurrentPage );
    connect(
        toolBar, &ViewToolbar::loadPage, [ = ] ( int pageNum ) {
            impl->mPageNavigation->setCurrentPage( pageNum );
            /** Grab focus */
            setFocus();
        }
    );

    /** Search */
    connect(
        toolBar, &ViewToolbar::search, [ = ] ( QString needle, bool fresh, bool reverse ) {
            /** Text changed */
            if ( fresh ) {
                /** Non-zero text length: search for it. */
                if ( needle.length() ) {
                    searchText( needle );
                }

                /** No text: clear search */
                else {
                    clearSearch();
                }
            }

            /** Same text */
            else {
                /** Previous instance */
                if ( reverse ) {
                    highlightPreviousSearchInstance();
                }

                else {
                    highlightNextSearchInstance();
                }
            }
        }
    );

    /* 100% Zoom */
    QShortcut *shortcut = new QShortcut( this );

    shortcut->setKey( QKeySequence( Qt::CTRL | Qt::Key_0 ) );
    connect(
        shortcut, &QShortcut::activated, [ = ]() {
            if ( impl->mDocument ) {
                setZoomFactor( 1.0 );
            }
        }
    );

    /* ProgressBar */
    progress = new QProgressBar( this );
    progress->move( 5, 5 );
    progress->setFixedSize( 50, 10 );
    progress->setRange( 0, 100 );
    progress->setStyle( QStyleFactory::create( "fusion" ) );
    progress->setFormat( "" );

    progress->hide();

    /* Does this work? */
    verticalScrollBar()->setSingleStep( 20 );
    horizontalScrollBar()->setSingleStep( 20 );

    /* First load */
    impl->calculateViewport();

    /* Don't draw a frame around the QScrollArea */
    setFrameStyle( QFrame::NoFrame );
}


QDocumentView::~QDocumentView() {
    delete toolBar;
    delete progress;
    delete impl;
}


QDocument * QDocumentView::load( QString path ) {
    QDocument *doc;

    if ( path.toLower().endsWith( "pdf" ) ) {
        doc = new PopplerDocument( path );
    }

    else if ( path.toLower().endsWith( "djv" ) or path.toLower().endsWith( "djvu" ) ) {
        doc = new DjVuDocument( path );
    }

    else {
        qWarning() << "Unknown document type:" << path;
        return nullptr;
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
                return nullptr;
            }

            /** Everything seems okay */
            break;
        }

        default: {
            return nullptr;
        }
    }

    /* Password has been supplied (if needed), document ready to be loaded */
    setDocument( doc );

    return doc;
}


void QDocumentView::setDocument( QDocument *document ) {
    if ( impl->mDocument == document ) {
        return;
    }

    if ( impl->mDocument ) {
        disconnect( impl->mDocumentStatusChangedConnection );
        disconnect( impl->mReloadDocumentConnection );
    }

    impl->mDocument = document;
    emit documentChanged( impl->mDocument );

    if ( impl->mDocument ) {
        impl->mDocumentStatusChangedConnection = connect(
            impl->mDocument, &QDocument::statusChanged, [ = ]( QDocument::Status sts ) {
                if ( sts == QDocument::Loading ) {
                    progress->show();
                }

                else if ( sts == QDocument::Ready ) {
                    impl->invalidateDocumentLayout();
                    viewport()->update();
                }

                else {
                    progress->hide();
                }
            }
        );

        connect(
            impl->mDocument, &QDocument::documentReloading, this, [ this ]() mutable {
                if ( impl->mDocument->status() != QDocument::Ready ) {
                    return;
                }

                impl->mDocState.currentPage = impl->mPageNavigation->currentPage();

                qreal hFrac = 1.0 * horizontalScrollBar()->value() / horizontalScrollBar()->maximum();
                qreal vFrac = 1.0 * verticalScrollBar()->value() / verticalScrollBar()->maximum();

                impl->mDocState.currentPosition = QPointF( hFrac, vFrac );
            },
            Qt::DirectConnection
        );

        impl->mReloadDocumentConnection = connect(
            impl->mDocument, &QDocument::documentReloaded, [ this ]() mutable {
                impl->mPageRenderer->reload();

                impl->mPageNavigation->setCurrentPage( impl->mDocState.currentPage );
                horizontalScrollBar()->setValue( impl->mDocState.currentPosition.x() * horizontalScrollBar()->maximum() );
                verticalScrollBar()->setValue( impl->mDocState.currentPosition.y() * verticalScrollBar()->maximum() );
            }
        );
    }

    impl->mPageNavigation->setDocument( impl->mDocument );
    impl->mPageRenderer->setDocument( impl->mDocument );

    toolBar->setMaximumPages( document->pageCount() );
    toolBar->setCurrentPage( impl->mPageNavigation->currentPage() );

    if ( showToolBar ) {
        toolBar->show();
    }

    else {
        toolBar->hide();
    }

    if ( document->status() == QDocument::Ready ) {
        impl->calculateViewport();
        viewport()->update();

        impl->mSearchThread->setDocument( document );
    }
}


QDocument *QDocumentView::document() const {
    return impl->mDocument;
}


QDocumentNavigation *QDocumentView::pageNavigation() const {
    return impl->mPageNavigation;
}


bool QDocumentView::isLayoutContinuous() const {
    return impl->mContinuous;
}


void QDocumentView::setLayoutContinuous( bool yes ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mContinuous == yes ) {
        return;
    }

    impl->mContinuous = yes;
    impl->invalidateDocumentLayout();

    emit layoutContinuityChanged( yes );
}


QDocumentView::PageLayout QDocumentView::pageLayout() const {
    return impl->mPageLayout;
}


void QDocumentView::setPageLayout( PageLayout lyt ) {
    if ( impl->mPageLayout == lyt ) {
        return;
    }

    impl->mPageLayout = lyt;

    /** Emit the signal */
    emit pageLayoutChanged( impl->mPageLayout );

    /** Don't do the document layout if there is no document */
    if ( not impl->mDocument ) {
        return;
    }

    impl->invalidateDocumentLayout();
    viewport()->update();
}


QDocumentView::ZoomMode QDocumentView::zoomMode() const {
    return impl->mZoomMode;
}


void QDocumentView::setZoomMode( ZoomMode mode ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mZoomMode == mode ) {
        return;
    }

    impl->mZoomMode = mode;
    impl->invalidateDocumentLayout();

    if ( impl->mZoomMode == CustomZoom ) {
        toolBar->setZoomButtonsEnabled( impl->mZoomFactor < 4.0 ? true : false, impl->mZoomFactor > 0.1 ? true : false );
    }

    else {
        toolBar->setZoomButtonsEnabled( false, false );
    }

    emit zoomModeChanged( impl->mZoomMode );
}


qreal QDocumentView::zoomFactor() const {
    return impl->zoomFactor();
}


void QDocumentView::setRenderOptions( QDocumentRenderOptions opts ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mRenderOpts == opts ) {
        return;
    }

    impl->mRenderOpts = opts;
    impl->invalidateDocumentLayout();

    emit renderOptionsChanged( impl->mRenderOpts );
}


QDocumentRenderOptions QDocumentView::renderOptions() const {
    return impl->mRenderOpts;
}


void QDocumentView::setZoomFactor( qreal factor ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mZoomFactor == factor ) {
        return;
    }

    if ( factor > 4.0 ) {
        factor = 4.0;
    }

    if ( factor < 0.1 ) {
        factor = 0.1;
    }

    impl->mZoomFactor = factor;
    impl->invalidateDocumentLayout();
    viewport()->update();

    emit zoomFactorChanged( impl->mZoomFactor );
}


QColor QDocumentView::pageColor() {
    return impl->mPageColor;
}


void QDocumentView::setPageColor( QColor clr ) {
    if ( impl->mPageColor == clr ) {
        return;
    }

    impl->mPageColor = clr;
    viewport()->update();
}


int QDocumentView::pageSpacing() const {
    return impl->mPageSpacing;
}


void QDocumentView::setPageSpacing( int spacing ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mPageSpacing == spacing ) {
        return;
    }

    impl->mPageSpacing = spacing;
    impl->invalidateDocumentLayout();

    emit pageSpacingChanged( impl->mPageSpacing );
}


QMargins QDocumentView::documentMargins() const {
    return impl->mDocumentMargins;
}


void QDocumentView::setDocumentMargins( QMargins margins ) {
    if ( not impl->mDocument ) {
        return;
    }

    if ( impl->mDocumentMargins == margins ) {
        return;
    }

    impl->mDocumentMargins = margins;
    impl->invalidateDocumentLayout();

    emit documentMarginsChanged( impl->mDocumentMargins );
}


void QDocumentView::focusSearch() {
    if ( impl->mDocument and showToolBar ) {
        toolBar->focusSearch();
    }
}


void QDocumentView::searchText( QString str ) {
    /** Clear previous search */
    impl->searchRects.clear();

    /** Set the current search string */
    impl->mSearchThread->setSearchString( str );

    /** Clear the current search page and rect */
    impl->searchPage    = -1;
    impl->curSearchRect = QRectF();

    /** Begin the search on the current page */
    impl->mSearchThread->searchPage( impl->mPageNavigation->currentPage() );
}


void QDocumentView::clearSearch() {
    /** Clear previous search */
    impl->searchRects.clear();

    /** Clear current search instance rect */
    impl->curSearchRect = QRectF();

    /** Clear the search text from the toolbar */
    toolBar->clearSearch();

    /** Focus the view */
    setFocus();

    /** Update the viewport */
    viewport()->update();
}


void QDocumentView::highlightNextSearchInstance() {
    impl->highlightNextSearchInstance();

    /** The Next search rect has been highlighted. Now redraw. */
    viewport()->update();
}


void QDocumentView::highlightPreviousSearchInstance() {
    impl->highlightPreviousSearchInstance();

    /** The Previous search rect has been highlighted. Now redraw. */
    viewport()->update();
}


QPair<int, int> QDocumentView::getCurrentSearchPosition() {
    return impl->getCurrentSearchPosition();
}


bool QDocumentView::showToolsOSD() const {
    return showToolBar;
}


void QDocumentView::setShowToolsOSD( bool yes ) {
    showToolBar = yes;

    if ( impl->mDocument == nullptr ) {
        return;
    }

    if ( yes ) {
        toolBar->show();

        toolBar->move( 0, viewport()->height() - toolBar->height() );
        toolBar->setFixedWidth( viewport()->width() );
    }

    else {
        toolBar->hide();
    }
}


void QDocumentView::paintEvent( QPaintEvent *event ) {
    if ( impl->mDocument == nullptr ) {
        // impl->calculateViewport();
        QAbstractScrollArea::paintEvent( event );
        return;
    }

    QPainter painter( viewport() );

    painter.fillRect( event->rect(), palette().brush( QPalette::Dark ) );
    painter.translate( -impl->mViewPort.x(), -impl->mViewPort.y() );

    for ( auto it = impl->mDocumentLayout.pageGeometries.cbegin(); it != impl->mDocumentLayout.pageGeometries.cend(); ++it ) {
        const QRect pageGeometry = it.value();

        if ( pageGeometry.intersects( impl->mViewPort ) ) {
            if ( it.key() == impl->mDocState.currentPage ) {
                painter.fillRect( QRectF( pageGeometry ).adjusted( -2, -2, 2, 2 ), qApp->palette().color( QPalette::Highlight ) );
            }

            painter.fillRect( pageGeometry, impl->mPageColor );

            const int page = it.key();
            QImage    img  = impl->mPageRenderer->requestPage( page, pageGeometry.size(), impl->mRenderOpts );

            if ( img.width() and img.height() ) {
                impl->paintOverlayRects( page, img );
                painter.drawImage( pageGeometry.topLeft(), img );
            }
        }
    }
}


void QDocumentView::resizeEvent( QResizeEvent *event ) {
    QAbstractScrollArea::resizeEvent( event );

    if ( showToolBar ) {
        toolBar->show();
        toolBar->move( 0, viewport()->height() - toolBar->height() );
        toolBar->setFixedWidth( viewport()->width() );
    }

    if ( impl->mDocument ) {
        toolBar->setEnabled( true );

        impl->updateScrollBars();

        if ( impl->pendingResize ) {
            return;
        }

        impl->pendingResize = true;
        QTimer::singleShot(
            250, [ = ]() {
                impl->calculateViewport();
                viewport()->update();
                impl->pendingResize = false;
            }
        );
    }

    else {
        toolBar->setDisabled( true );
    }
}


void QDocumentView::scrollContentsBy( int dx, int dy ) {
    QAbstractScrollArea::scrollContentsBy( dx, dy );

    impl->calculateViewport();
}


void QDocumentView::keyReleaseEvent( QKeyEvent *kEvent ) {
    switch ( kEvent->key() ) {
        case Qt::Key_Right: {
            /* Go to next page */
            impl->mPageNavigation->setCurrentPage( impl->mPageNavigation->currentPage() + 1 );
            break;
        }

        case Qt::Key_Left: {
            /* Go to previous page */
            impl->mPageNavigation->setCurrentPage( impl->mPageNavigation->currentPage() - 1 );
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
            setZoomFactor( impl->mZoomFactor * 1.10 );
            break;
        }

        case Qt::Key_Minus: {
            /* Zoom Out */
            setZoomFactor( impl->mZoomFactor / 1.10 );
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
            setZoomFactor( impl->mZoomFactor * 1.10 );
        }

        else {
            setZoomFactor( impl->mZoomFactor / 1.10 );
        }

        return;
    }

    QAbstractScrollArea::wheelEvent( wEvent );
}
