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

#include <qdocumentview/PopplerDocument.hpp>

PopplerDocument::PopplerDocument( QString pdfPath ) : QDocument( pdfPath ) {
    mPdfDoc = nullptr;
}


void PopplerDocument::setPassword( QString password ) {
    if ( mPdfDoc->unlock( password.toLatin1(), password.toLatin1() ) ) {
        mStatus = Failed;
        mError  = IncorrectPasswordError;

        qDebug() << "Invalid password. Please try again.";
        mPassNeeded = true;
        emit statusChanged( Failed );
        emit passwordRequired();

        return;
    }

    mPassNeeded = false;
    mStatus     = Loading;
    mError      = NoError;

    emit statusChanged( Loading );

    mPdfDoc->setRenderHint( Poppler::Document::Antialiasing );
    mPdfDoc->setRenderHint( Poppler::Document::TextAntialiasing );
    mPdfDoc->setRenderHint( Poppler::Document::TextHinting );

    for ( int i = 0; i < mPdfDoc->numPages(); i++ ) {
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        Poppler::Page *p = mPdfDoc->page( i );
#else
        Poppler::Page *p = mPdfDoc->page( i ).release();
#endif

        PdfPage *page = new PdfPage( i );
        page->setPageData( p );
        mPages.append( page );

        emit loading( 1.0 * i / mPdfDoc->numPages() * 100.0 );
    }

    mStatus = Ready;
    mError  = NoError;

    emit statusChanged( Ready );
    emit pageCountChanged( mPages.count() );
    emit loading( 100 );
}


QString PopplerDocument::title() const {
    return mPdfDoc->title();
}


QString PopplerDocument::author() const {
    return mPdfDoc->author();
}


QString PopplerDocument::creator() const {
    return mPdfDoc->creator();
}


QString PopplerDocument::producer() const {
    return mPdfDoc->producer();
}


QString PopplerDocument::created() const {
    return mPdfDoc->creationDate().toString( "MMM DD, yyyy hh:mm:ss t AP" );
}


void PopplerDocument::load() {
    mStatus = Loading;
    emit statusChanged( Loading );

    if ( not QFile::exists( mDocPath ) ) {
        mStatus = Failed;
        mError  = FileNotFoundError;
        emit statusChanged( Failed );

        return;
    }

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    mPdfDoc = std::unique_ptr<Poppler::Document>( Poppler::Document::load( mDocPath ) );
#else
    mPdfDoc = Poppler::Document::load( mDocPath );
#endif

    if ( not mPdfDoc or not mPdfDoc->numPages() ) {
        mStatus = Failed;
        mError  = UnknownError;
        qDebug() << "Poppler::Document load failed";
        emit statusChanged( Failed );

        return;
    }

    if ( mPdfDoc->isLocked() ) {
        mStatus = Failed;
        mError  = IncorrectPasswordError;
        qDebug() << "Poppler::Document is locked";
        mPassNeeded = true;
        emit passwordRequired();
        emit statusChanged( Failed );
        return;
    }

    mPdfDoc->setRenderHint( Poppler::Document::Antialiasing );
    mPdfDoc->setRenderHint( Poppler::Document::TextAntialiasing );
    mPdfDoc->setRenderHint( Poppler::Document::TextHinting );

    for ( int i = 0; i < mPdfDoc->numPages(); i++ ) {
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        Poppler::Page *p = mPdfDoc->page( i );
#else
        Poppler::Page *p = mPdfDoc->page( i ).release();
#endif

        PdfPage *page = new PdfPage( i );
        page->setPageData( p );
        mPages.append( page );

        emit loading( 1.0 * i / mPdfDoc->numPages() * 100.0 );
    }

    mStatus = Ready;
    mError  = NoError;

    emit statusChanged( Ready );
    emit pageCountChanged( mPages.count() );
    emit loading( 100 );
}


void PopplerDocument::close() {
    mStatus = Unloading;
    emit statusChanged( Unloading );

    mPages.clear();
    mZoom = 1.0;

    mPdfDoc.reset();
}


PdfPage::PdfPage( int pgNo ) : QDocumentPage( pgNo ) {
    // Nothing much to be done here
}


PdfPage::~PdfPage() {
    m_page.reset();
}


void PdfPage::setPageData( void *data ) {
    m_page = std::unique_ptr<Poppler::Page>( (Poppler::Page *)data );
}


QSizeF PdfPage::pageSize( qreal zoom ) const {
    return m_page->pageSizeF() * zoom;
}


QImage PdfPage::thumbnail() const {
    return m_page->thumbnail();
}


QImage PdfPage::render( QSize pSize, QDocumentRenderOptions opts ) const {
    qreal wZoom = 1.0 * pSize.width() / m_page->pageSizeF().width();
    qreal hZoom = 1.0 * pSize.height() / m_page->pageSizeF().height();

    return m_page->renderToImage( 72 * wZoom, 72 * hZoom, -1, -1, -1, -1, ( Poppler::Page::Rotation )opts.rotation() );
}


QImage PdfPage::render( qreal zoomFactor, QDocumentRenderOptions opts ) const {
    return m_page->renderToImage( 72 * zoomFactor, 72 * zoomFactor, -1, -1, -1, -1, ( Poppler::Page::Rotation )opts.rotation() );
}


QImage PdfPage::render( int dpiX, int dpiY, QDocumentRenderOptions opts ) const {
    return render( dpiX / 72.0, dpiY / 72.0, opts );
}


QString PdfPage::pageText() const {
    return text( QRectF() );
}


QString PdfPage::text( QRectF rect ) const {
    return m_page->text( rect );
}


QList<QRectF> PdfPage::search( QString query, QDocumentRenderOptions opts ) const {
    return m_page->search(
        query,                                                                                      // Search
                                                                                                    // text
        Poppler::Page::IgnoreCase | Poppler::Page::IgnoreDiacritics,                                // Case
                                                                                                    // insensitive
        ( Poppler::Page::Rotation )opts.rotation()                                                  // Rotation
    );
}
