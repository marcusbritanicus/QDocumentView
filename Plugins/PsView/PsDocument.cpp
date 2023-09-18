/**
 * This file is a part of DesQDocs.
 * DesQDocs is the default document viewer for the DesQ Suite
 * Copyright 2019-2021 Britanicus <marcusbritanicus@gmail.com>
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

#include "PsDocument.hpp"

PsDocument::PsDocument( QString psPath ) : QDocument( psPath ) {
    mPsDoc = nullptr;
}


PsDocument::~PsDocument() {
    spectre_render_context_free( mRenderCtxt );
    mRenderCtxt = nullptr;

    spectre_document_free( mPsDoc );
    mPsDoc = nullptr;
}


void PsDocument::setPassword( QString ) {
    // Nothing to do since PS does not support encryption
}


QString PsDocument::title() const {
    // return mPsDoc->title();
    return QString();
}


QString PsDocument::author() const {
    // return mPsDoc->author();
    return QString();
}


QString PsDocument::creator() const {
    // return mPsDoc->creator();
    return QString();
}


QString PsDocument::producer() const {
    // return mPsDoc->producer();
    return QString();
}


QString PsDocument::created() const {
    // return mPsDoc->creationDate().toString( "MMM DD, yyyy hh:mm:ss t AP" );
    return QString();
}


void PsDocument::load() {
    mStatus = Loading;
    emit statusChanged( Loading );

    if ( not QFile::exists( mDocPath ) ) {
        mStatus = Failed;
        mError  = FileNotFoundError;
        emit statusChanged( Failed );

        return;
    }

    mPsDoc = spectre_document_new();

    spectre_document_load( mPsDoc, QFile::encodeName( mDocPath ) );

    if ( spectre_document_status( mPsDoc ) != SPECTRE_STATUS_SUCCESS ) {
        spectre_document_free( mPsDoc );

        mStatus = Failed;
        mError  = UnknownError;

        return;
    }

    mRenderCtxt = spectre_render_context_new();

    spectre_render_context_set_antialias_bits(
        mRenderCtxt,        // Render context
        4,                  // No. of bits for rendering antialiased graphics
        4                   // No, of bits for rendering antialiased text
    );

    int pages = spectre_document_get_n_pages( mPsDoc );

    for ( int i = 0; i < pages; i++ ) {
        SpectrePage *pg   = spectre_document_get_page( mPsDoc, i );
        PsPage      *page = new PsPage( i, mRenderCtxt );

        if ( pg == nullptr ) {
            mPages << page;
            continue;
        }

        page->setPageData( pg );
        mPages.append( page );

        emit loading( 1.0 * i / pages * 100.0 );
    }

    mStatus = Ready;
    mError  = NoError;

    emit statusChanged( Ready );
    emit pageCountChanged( mPages.count() );
    emit loading( 100 );
}


void PsDocument::close() {
    mStatus = Unloading;
    emit statusChanged( Unloading );

    mPages.clear();
    mZoom = 1.0;
}


PsPage::PsPage( int pgNo, SpectreRenderContext *rndrCtxt ) : QDocumentPage( pgNo ) {
    mRenderCtxt = rndrCtxt;
}


PsPage::~PsPage() {
    spectre_page_free( mPage );
    mPage = nullptr;
}


void PsPage::setPageData( void *data ) {
    mPage = (SpectrePage *)data;

    int w;
    int h;

    spectre_page_get_size( mPage, &w, &h );
    mPageSize = QSizeF( w, h );
}


QSizeF PsPage::pageSize( qreal zoom ) const {
    return mPageSize * zoom;
}


QImage PsPage::thumbnail() const {
    return QImage();
}


QImage PsPage::render( QSize pSize, QDocumentRenderOptions opts ) const {
    qreal wZoom = 1.0 * pSize.width() / mPageSize.width();
    qreal hZoom = 1.0 * pSize.height() / mPageSize.height();

    switch ( opts.rotation() ) {
        case QDocumentRenderOptions::Rotate90: {
            [[fallthrough]];
        }

        case QDocumentRenderOptions::Rotate270: {
            std::swap( wZoom, hZoom );
            break;
        }

        default: {
            break;
        }
    }

    spectre_render_context_set_scale( mRenderCtxt, wZoom, hZoom );

    switch ( opts.rotation() ) {
        default:
        case QDocumentRenderOptions::Rotate0: {
            spectre_render_context_set_rotation( mRenderCtxt, 0 );
            break;
        }

        case QDocumentRenderOptions::Rotate90: {
            spectre_render_context_set_rotation( mRenderCtxt, 90 );
            break;
        }

        case QDocumentRenderOptions::Rotate180: {
            spectre_render_context_set_rotation( mRenderCtxt, 180 );
            break;
        }

        case QDocumentRenderOptions::Rotate270: {
            spectre_render_context_set_rotation( mRenderCtxt, 270 );
            break;
        }
    }

    int w = pSize.width();
    int h = pSize.height();

    if ( (opts.rotation() == QDocumentRenderOptions::Rotate90) || (opts.rotation() == QDocumentRenderOptions::Rotate270) ) {
        qSwap( w, h );
    }

    unsigned char *pageData = 0;
    int           rowLength = 0;

    spectre_page_render( mPage, mRenderCtxt, &pageData, &rowLength );

    if ( spectre_page_status( mPage ) != SPECTRE_STATUS_SUCCESS ) {
        free( pageData );
        pageData = 0;

        return QImage();
    }

    /**
     * Okular, which renders PS documents properly, states:
     * Qt needs the missing alpha of QImage::Format_RGB32 to be 0xff
     */
    if ( pageData && (pageData[ 3 ] != 0xff) ) {
        for ( int i = 3; i < rowLength * h; i += 4 ) {
            pageData[ i ] = 0xff;
        }
    }

    QImage aux;

    if ( rowLength == w * 4 ) {
        aux = QImage( pageData, w, h, QImage::Format_RGB32 );
    }

    else {
        aux = QImage( pageData, rowLength / 4, h, QImage::Format_RGB32 );
    }

    QImage   image( w, h, QImage::Format_RGB32 );
    QPainter painter( &image );

    qreal dx = (w > aux.width() ? (w - aux.width() ) / 2.0 : 0);
    qreal dy = (w > aux.height() ? (w - aux.height() ) / 2.0 : 0);

    painter.drawImage( QPointF( dx, dy ), aux );

    painter.end();

    free( pageData );
    pageData = nullptr;

    return image;
}


QImage PsPage::render( qreal zoomFactor, QDocumentRenderOptions opts ) const {
    return render( round( 72 * zoomFactor ), round( 72 * zoomFactor ), opts );
}


QImage PsPage::render( int dpiX, int dpiY, QDocumentRenderOptions opts ) const {
    double xscale = dpiX / 72.0;
    double yscale = dpiY / 72.0;

    switch ( opts.rotation() ) {
        case QDocumentRenderOptions::Rotate90: {
            [[fallthrough]];
        }

        case QDocumentRenderOptions::Rotate270: {
            std::swap( xscale, yscale );
            break;
        }

        default: {
            break;
        }
    }

    spectre_render_context_set_scale( mRenderCtxt, xscale, yscale );

    switch ( opts.rotation() ) {
        default:
        case QDocumentRenderOptions::Rotate0: {
            spectre_render_context_set_rotation( mRenderCtxt, 0 );
            break;
        }

        case QDocumentRenderOptions::Rotate90: {
            spectre_render_context_set_rotation( mRenderCtxt, 90 );
            break;
        }

        case QDocumentRenderOptions::Rotate180: {
            spectre_render_context_set_rotation( mRenderCtxt, 180 );
            break;
        }

        case QDocumentRenderOptions::Rotate270: {
            spectre_render_context_set_rotation( mRenderCtxt, 270 );
            break;
        }
    }

    int w = qRound( mPageSize.width() * xscale );
    int h = qRound( mPageSize.height() * yscale );

    if ( (opts.rotation() == QDocumentRenderOptions::Rotate90) || (opts.rotation() == QDocumentRenderOptions::Rotate270) ) {
        qSwap( w, h );
    }

    unsigned char *pageData = 0;
    int           rowLength = 0;

    spectre_page_render( mPage, mRenderCtxt, &pageData, &rowLength );

    if ( spectre_page_status( mPage ) != SPECTRE_STATUS_SUCCESS ) {
        free( pageData );
        pageData = 0;

        return QImage();
    }

    /**
     * Okular, which renders PS documents properly, states:
     * Qt needs the missing alpha of QImage::Format_RGB32 to be 0xff
     */
    if ( pageData && (pageData[ 3 ] != 0xff) ) {
        for ( int i = 3; i < rowLength * h; i += 4 ) {
            pageData[ i ] = 0xff;
        }
    }

    QImage aux;

    if ( rowLength == w * 4 ) {
        aux = QImage( pageData, w, h, QImage::Format_RGB32 );
    }

    else {
        aux = QImage( pageData, rowLength / 4, h, QImage::Format_RGB32 );
    }

    QImage   image( w, h, QImage::Format_RGB32 );
    QPainter painter( &image );

    qreal dx = (w > aux.width() ? (w - aux.width() ) / 2.0 : 0);
    qreal dy = (w > aux.height() ? (w - aux.height() ) / 2.0 : 0);

    painter.drawImage( QPointF( dx, dy ), aux );

    painter.end();

    free( pageData );
    pageData = nullptr;

    return image;
}


QString PsPage::pageText() const {
    return QString();
}


QString PsPage::text( QRectF ) const {
    return QString();
}


QList<QRectF> PsPage::search( QString, QDocumentRenderOptions ) const {
    return QList<QRectF>();
}
