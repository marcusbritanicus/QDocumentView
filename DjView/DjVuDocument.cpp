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

#include <qdocumentview/DjVuDocument.hpp>

DjVuDocument::DjVuDocument( QString pdfPath ) : QDocument( pdfPath ) {
    mDjDoc = nullptr;
}


DjVuDocument::~DjVuDocument() {
    close();
}


void DjVuDocument::setPassword( QString ) {
}


QString DjVuDocument::title() const {
    return QString();
}


QString DjVuDocument::author() const {
    return QString();
}


QString DjVuDocument::creator() const {
    return QString();
}


QString DjVuDocument::producer() const {
    return QString();
}


QString DjVuDocument::created() const {
    return QString();
}


void DjVuDocument::load() {
    mStatus = Loading;
    emit statusChanged( Loading );

    if ( not QFile::exists( mDocPath ) ) {
        mStatus = Failed;
        mError  = FileNotFoundError;
        emit statusChanged( Failed );

        return;
    }

    mDjCtx = ddjvu_context_create( "qdocumentview" );
    mDjDoc = ddjvu_document_create_by_filename( mDjCtx, mDocPath.toLocal8Bit().data(), 1 );

    /* Wait for decoding to be complete */
    ddjvu_job_t *job = ddjvu_document_job( mDjDoc );

    ddjvu_message_wait( mDjCtx );

    if ( ddjvu_job_status( job ) == DDJVU_JOB_FAILED ) {
        mStatus = Failed;
        mError  = UnknownError;
        qDebug() << "DjVu::Document load failed";
        emit statusChanged( Failed );

        return;
    }

    // if ( mDjDoc->isLocked() ) {
    //     mStatus = Failed;
    //     mError  = IncorrectPasswordError;
    //     qDebug() << "Poppler::Document is locked";
    //     mPassNeeded = true;
    //     emit passwordRequired();
    //     emit statusChanged( Failed );
    //     return;
    // }

    ddjvu_message_wait( mDjCtx );
    int pages = 0;

    pages = ddjvu_document_get_pagenum( mDjDoc );

    /* DjVu page ptr */
    ddjvu_page_t *pg;
    /* DjVu page job ptr */
    ddjvu_job_t *pgjob;

    for ( int i = 0; i < pages; i++ ) {
        pg    = ddjvu_page_create_by_pageno( mDjDoc, i );
        pgjob = ddjvu_page_job( pg );

        while ( ddjvu_job_status( pgjob ) < DDJVU_JOB_OK ) {
            continue;
        }

        DjPage *page = new DjPage( i, mDjDoc );
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


void DjVuDocument::close() {
    mStatus = Unloading;
    emit statusChanged( Unloading );

    mPages.clear();
    mZoom = 1.0;

    ddjvu_document_release( mDjDoc );
    ddjvu_context_release( mDjCtx );
}


DjPage::DjPage( int pgNo, ddjvu_document_t *doc ) : QDocumentPage( pgNo ) {
    mDjDoc = doc;
}


DjPage::~DjPage() {
    if ( m_page != nullptr ) {
        ddjvu_page_release( m_page );
    }
}


void DjPage::setPageData( void *data ) {
    if ( data == nullptr ) {
        return;
    }

    m_page = static_cast<ddjvu_page_t *>(data);

    /* Job Status */
    ddjvu_status_t r;

    /* DjVu page info struct */
    ddjvu_pageinfo_t info;

    while ( (r = ddjvu_document_get_pageinfo_imp( mDjDoc, mPageNo, &info, sizeof(ddjvu_pageinfo_t) ) ) < DDJVU_JOB_OK ) {
        // Decoding...
    }

    if ( r >= DDJVU_JOB_FAILED ) {
        return;
    }

    mPageSize = QSizeF( info.width, info.height );
}


QSizeF DjPage::pageSize( qreal zoom ) const {
    return mPageSize * zoom;
}


QImage DjPage::thumbnail() const {
    int width  = 128;
    int height = 128;

    unsigned int   masks[ 4 ] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
    ddjvu_format_t *fmt       = ddjvu_format_create( DDJVU_FORMAT_RGBMASK32, 4, masks );

    if ( ddjvu_thumbnail_status( mDjDoc, mPageNo, 1 ) >= DDJVU_JOB_FAILED ) {
        return QImage();
    }

    QImage img( width, height, QImage::Format_RGB32 );

    if ( not ddjvu_thumbnail_render( mDjDoc, mPageNo, &width, &height, fmt, img.bytesPerLine(), (char *)img.bits() ) ) {
        return QImage();
    }

    return img;
}


QImage DjPage::render( QSize pSize, QDocumentRenderOptions opts ) const {
    /* DjVu page render format */
    unsigned int   masks[ 4 ] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
    ddjvu_format_t *fmt       = ddjvu_format_create( DDJVU_FORMAT_RGBMASK32, 4, masks );

    ddjvu_page_set_rotation( m_page, (ddjvu_page_rotation_t)opts.rotation() );

    ddjvu_rect_t rect;

    rect.w = pSize.width();
    rect.h = pSize.height();
    rect.x = 0;
    rect.y = 0;

    /* Make DjVu decoder follow X11 conventions: Why? Because DjView4 does so... :P */
    ddjvu_format_set_row_order( fmt, true );
    ddjvu_format_set_y_direction( fmt, true );

    QImage image( pSize.width(), pSize.height(), QImage::Format_RGB32 );

    if ( not ddjvu_page_render( m_page, DDJVU_RENDER_COLOR, &rect, &rect, fmt, image.bytesPerLine(), (char *)image.bits() ) ) {
        return QImage();
    }

    return image;
}


QImage DjPage::render( qreal zoomFactor, QDocumentRenderOptions opts ) const {
    return render( (mPageSize * zoomFactor).toSize(), opts );
}


QString DjPage::pageText() const {
    // return text( QRectF() );
    return QString();
}


QString DjPage::text( QRectF /* rect */ ) const {
    // return m_page->text( rect );
    return QString();
}


QList<QRectF> DjPage::search( QString /* query */, QDocumentRenderOptions /* opts */ ) const {
    // return m_page->search(
    //     query,                                                                  // Search text
    //     Poppler::Page::IgnoreCase | Poppler::Page::IgnoreDiacritics,            // Case insensitive
    //     ( Poppler::Page::Rotation )opts.rotation()                              // Rotation
    // );

    return QList<QRectF>();
}
