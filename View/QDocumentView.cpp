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

QDocumentView::QDocumentView( QWidget *parent ) : QAbstractScrollArea( *new QDocumentViewPrivate(), parent ) {
    Q_D( QDocumentView );

    d->init();

    /** Piggy-back the matchesFound, and searchComplete signals from the search thread */
    connect( d->m_searchThread, &QDocumentSearch::matchesFound,   this, &QDocumentView::matchesFound );
    connect( d->m_searchThread, &QDocumentSearch::searchComplete, this, &QDocumentView::searchComplete );

    connect(
        d->m_searchThread, &QDocumentSearch::resultsReady, [ d, this ]( int page ) {
            d->searchRects[ page ] = d->m_searchThread->results( page );
            viewport()->update();
        }
    );

    connect(
        d->m_searchThread, &QDocumentSearch::resultsReady, [ d, this ]( int page ) {
            d->searchRects[ page ] = d->m_searchThread->results( page );
            viewport()->update();
        }
    );

    /* Setup Page Navigation */
    connect(
        d->m_pageNavigation, &QDocumentNavigation::currentPageChanged, this, [ d ]( int page ){
            d->currentPageChanged( page );
        }
    );

    /* Setup Page Renderer */
    connect(
        d->m_pageRenderer, &QDocumentRenderer::pageRendered, [ = ]( int ) {
            viewport()->update();
        }
    );

    /* Zoom buttons */
    mZoomBtn = new Zoom( this );

    if ( d->m_zoomMode == CustomZoom ) {
        mZoomBtn->show();
        mZoomBtn->setEnlargeEnabled( d->m_zoomFactor < 4.0 ? true : false );
        mZoomBtn->setDwindleEnabled( d->m_zoomFactor > 0.1 ? true : false );
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
                    setZoomFactor( d->m_zoomFactor * 1.10 );
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
                    setZoomFactor( d->m_zoomFactor / 1.10 );
                }
            }

            mZoomBtn->setEnlargeEnabled( d->m_zoomFactor < 4.0 ? true : false );
            mZoomBtn->setDwindleEnabled( d->m_zoomFactor > 0.1 ? true : false );
        }
    );

    /* Page buttons */
    mPagesBtn = new PageWidget( this );
    connect( d->m_pageNavigation, &QDocumentNavigation::currentPageChanged, mPagesBtn,           &PageWidget::setCurrentPage );
    connect( mPagesBtn,           &PageWidget::loadPage,                    d->m_pageNavigation, &QDocumentNavigation::setCurrentPage );

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
    d->calculateViewport();

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


QDocumentView::QDocumentView( QDocumentViewPrivate& dd, QWidget *parent ) : QAbstractScrollArea( dd, parent ) {
}


QDocumentView::~QDocumentView() {
    delete mZoomBtn;
    delete mPagesBtn;
    delete progress;
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
    Q_D( QDocumentView );

    if ( d->m_document == document ) {
        return;
    }

    if ( d->m_document ) {
        disconnect( d->m_documentStatusChangedConnection );
        disconnect( d->m_reloadDocumentConnection );
    }

    d->m_document = document;
    emit documentChanged( d->m_document );

    if ( d->m_document ) {
        d->m_documentStatusChangedConnection = connect(
            d->m_document, &QDocument::statusChanged, this, [ = ]( QDocument::Status sts ) {
                d->documentStatusChanged();

                if ( sts == QDocument::Loading ) {
                    progress->show();
                }

                else {
                    progress->hide();
                }
            }
        );

        d->m_reloadDocumentConnection = connect(
            d->m_document, &QDocument::reloadDocument, this, [ d ]() {
                d->m_pageRenderer->reload();
            }
        );
    }

    d->m_pageNavigation->setDocument( d->m_document );
    d->m_pageRenderer->setDocument( d->m_document );

    d->documentStatusChanged();

    mPagesBtn->setMaximumPages( document->pageCount() );
    mPagesBtn->setCurrentPage( d->m_pageNavigation->currentPage() );

    if ( mShowZoom ) {
        mZoomBtn->show();
    }

    if ( mShowPages ) {
        mPagesBtn->show();
    }

    progress->hide();
}


QDocument *QDocumentView::document() const {
    Q_D( const QDocumentView );

    return d->m_document;
}


QDocumentNavigation *QDocumentView::pageNavigation() const {
    Q_D( const QDocumentView );

    return d->m_pageNavigation;
}


bool QDocumentView::isLayoutContinuous() const {
    Q_D( const QDocumentView );

    return d->m_continuous;
}


void QDocumentView::setLayoutContinuous( bool yes ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_continuous == yes ) {
        return;
    }

    d->m_continuous = yes;
    d->invalidateDocumentLayout();

    emit layoutContinuityChanged( yes );
}


QDocumentView::PageLayout QDocumentView::pageLayout() const {
    Q_D( const QDocumentView );

    return d->m_pageLayout;
}


void QDocumentView::setPageLayout( PageLayout lyt ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_pageLayout == lyt ) {
        return;
    }

    d->m_pageLayout = lyt;
    d->invalidateDocumentLayout();

    emit pageLayoutChanged( d->m_pageLayout );
}


QDocumentView::ZoomMode QDocumentView::zoomMode() const {
    Q_D( const QDocumentView );

    return d->m_zoomMode;
}


void QDocumentView::setZoomMode( ZoomMode mode ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_zoomMode == mode ) {
        return;
    }

    d->m_zoomMode = mode;
    d->invalidateDocumentLayout();

    if ( d->m_zoomMode == CustomZoom ) {
        mZoomBtn->show();
    }

    else {
        mZoomBtn->hide();
    }

    emit zoomModeChanged( d->m_zoomMode );
}


qreal QDocumentView::zoomFactor() const {
    Q_D( const QDocumentView );

    return d->zoomFactor();
}


void QDocumentView::setRenderOptions( QDocumentRenderOptions opts ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_renderOpts == opts ) {
        return;
    }

    d->m_renderOpts = opts;
    d->invalidateDocumentLayout();

    emit renderOptionsChanged( d->m_renderOpts );
}


QDocumentRenderOptions QDocumentView::renderOptions() const {
    Q_D( const QDocumentView );
    return d->m_renderOpts;
}


void QDocumentView::setZoomFactor( qreal factor ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_zoomFactor == factor ) {
        return;
    }

    if ( factor > 4.0 ) {
        factor = 4.0;
    }

    if ( factor < 0.1 ) {
        factor = 0.1;
    }

    d->m_zoomFactor = factor;
    d->invalidateDocumentLayout();

    emit zoomFactorChanged( d->m_zoomFactor );
}


int QDocumentView::pageSpacing() const {
    Q_D( const QDocumentView );

    return d->m_pageSpacing;
}


void QDocumentView::setPageSpacing( int spacing ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_pageSpacing == spacing ) {
        return;
    }

    d->m_pageSpacing = spacing;
    d->invalidateDocumentLayout();

    emit pageSpacingChanged( d->m_pageSpacing );
}


QMargins QDocumentView::documentMargins() const {
    Q_D( const QDocumentView );

    return d->m_documentMargins;
}


void QDocumentView::setDocumentMargins( QMargins margins ) {
    Q_D( QDocumentView );

    if ( not d->m_document ) {
        return;
    }

    if ( d->m_documentMargins == margins ) {
        return;
    }

    d->m_documentMargins = margins;
    d->invalidateDocumentLayout();

    emit documentMarginsChanged( d->m_documentMargins );
}


void QDocumentView::searchText( QString str ) {
    Q_D( QDocumentView );

    d->searchRects.clear();

    d->m_searchThread->setSearchString( str );
    d->m_searchThread->searchPage( d->m_pageNavigation->currentPage() );
}


bool QDocumentView::showPagesOSD() const {
    return mShowPages;
}


void QDocumentView::setShowPagesOSD( bool yes ) {
    Q_D( QDocumentView );

    mShowPages = yes;

    if ( not d->m_document ) {
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
    Q_D( QDocumentView );

    mShowZoom = yes;

    if ( not d->m_document ) {
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
    painter.translate( -d->m_viewport.x(), -d->m_viewport.y() );

    for ( auto it = d->m_documentLayout.pageGeometries.cbegin(); it != d->m_documentLayout.pageGeometries.cend(); ++it ) {
        const QRect pageGeometry = it.value();

        if ( pageGeometry.intersects( d->m_viewport ) ) {
            painter.fillRect( pageGeometry, Qt::white );

            const int page = it.key();
            QImage    img  = d->m_pageRenderer->requestPage( page, pageGeometry.size(), d->m_renderOpts );

            if ( img.width() and img.height() ) {
                d->paintSearchRects( page, img );
                painter.drawImage( pageGeometry.topLeft(), img );
            }
        }
    }
}


void QDocumentView::resizeEvent( QResizeEvent *event ) {
    Q_D( QDocumentView );

    event->accept();

    if ( d->m_document ) {
        if ( mZoomBtn and mZoomBtn->isVisible() ) {
            mZoomBtn->move( 5, viewport()->height() - mZoomBtn->height() - 5 );
        }

        if ( mPagesBtn and mPagesBtn->isVisible() ) {
            mPagesBtn->move( viewport()->width() - mPagesBtn->width() - 5, viewport()->height() - mPagesBtn->height() - 5 );
        }

        d->updateScrollBars();

        qApp->processEvents();

        if ( d->pendingResize ) {
            return;
        }

        d->pendingResize = true;
        QTimer::singleShot(
            250, [ d, this ]() {
                d->calculateViewport();
                d->pendingResize = false;
                viewport()->update();
            }
        );
    }
}


void QDocumentView::scrollContentsBy( int dx, int dy ) {
    Q_D( QDocumentView );

    QAbstractScrollArea::scrollContentsBy( dx, dy );

    d->calculateViewport();
}


void QDocumentView::keyPressEvent( QKeyEvent *kEvent ) {
    Q_D( QDocumentView );

    switch ( kEvent->key() ) {
        case Qt::Key_Right: {
            /* Go to next page */
            d->m_pageNavigation->setCurrentPage( d->m_pageNavigation->currentPage() + 1 );
            break;
        }

        case Qt::Key_Left: {
            /* Go to previous page */
            d->m_pageNavigation->setCurrentPage( d->m_pageNavigation->currentPage() - 1 );
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
            setZoomFactor( d->m_zoomFactor * 1.10 );
            break;
        }

        case Qt::Key_Minus: {
            /* Zoom Out */
            setZoomFactor( d->m_zoomFactor / 1.10 );
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
    Q_D( QDocumentView );

    if ( wEvent->modifiers() & Qt::ControlModifier ) {
        QPoint numDegrees = wEvent->angleDelta() / 8;

        int steps = numDegrees.y() / 15;

        if ( steps > 0 ) {
            setZoomFactor( d->m_zoomFactor * 1.10 );
        }

        else {
            setZoomFactor( d->m_zoomFactor / 1.10 );
        }

        return;
    }

    QAbstractScrollArea::wheelEvent( wEvent );
}
