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

#include <libgen.h>
#include <qdocumentview/QDocument.hpp>

/*
 * Helper functions
 */
QString dirName( QString path ) {
    if ( path == "/" or path == "//" ) {
        return "/";
    }

    /* Simple clean path: remove '//' and './' */
    path = path.replace( "//", "/" ).replace( "/./", "/" );

    char    *dupPath = strdup( path.toLocal8Bit().constData() );
    QString dirPth   = QString::fromLocal8Bit( dirname( dupPath ) );

    dirPth += (dirPth.endsWith( "/" ) ? "" : "/");
    free( dupPath );

    return dirPth;
}


QString baseName( QString path ) {
    if ( path == "/" or path == "//" ) {
        return "/";
    }

    /* Simple clean path" remove '//' and './' */
    path = path.replace( "//", "/" ).replace( "/./", "/" );

    char    *dupPath = strdup( path.toLocal8Bit().constData() );
    QString basePth  = QString::fromLocal8Bit( basename( dupPath ) );

    free( dupPath );

    return basePth;
}


/*
 * Generic class to handle document
 */

QDocument::QDocument( QString path ) : QObject() {
    mZoom       = 1.0;
    mStatus     = Null;
    mError      = NoError;
    mPassNeeded = false;

    mDocPath = QFileInfo( path ).absoluteFilePath();

    QFileSystemWatcher *fsw = new QFileSystemWatcher();

    fsw->addPath( mDocPath );
    connect(
        fsw, &QFileSystemWatcher::fileChanged, [ = ]( QString file ) {
            /* File deleted and created again: add it to the watcher */
            if ( not fsw->files().contains( file ) and QFile::exists( file ) ) {
                fsw->addPath( file );
            }

            reload();
        }
    );
}


QString QDocument::fileName() const {
    return baseName( mDocPath );
}


QString QDocument::filePath() const {
    return dirName( mDocPath );
}


bool QDocument::passwordNeeded() const {
    return mPassNeeded;
}


int QDocument::pageCount() const {
    return mPages.count();
}


QSizeF QDocument::pageSize( int pageNo ) const {
    return mPages.at( pageNo )->pageSize() * mZoom;
}


void QDocument::reload() {
    emit documentReloading();

    mStatus = Null;
    mPages.clear();

    load();

    if ( mStatus == Ready ) {
        qDebug() << "Reload your pages..";
        emit documentReloaded();
    }
}


QDocument::Status QDocument::status() const {
    return mStatus;
}


QDocument::Error QDocument::error() const {
    return mError;
}


QImage QDocument::renderPage( int pageNo, QSize size, QDocumentRenderOptions opts ) const {
    if ( pageNo >= mPages.count() ) {
        return QImage();
    }

    return mPages.at( pageNo )->render( size, opts );
}


QImage QDocument::renderPage( int pageNo, qreal zoomFactor, QDocumentRenderOptions opts ) const {
    if ( pageNo >= mPages.count() ) {
        return QImage();
    }

    return mPages.at( pageNo )->render( zoomFactor, opts );
}


QDocumentPages QDocument::pages() const {
    return mPages;
}


QDocumentPage * QDocument::page( int pageNo ) const {
    if ( pageNo >= mPages.count() ) {
        return nullptr;
    }

    if ( pageNo < 0 ) {
        return nullptr;
    }

    if ( not mPages.count() ) {
        return nullptr;
    }

    return mPages.at( pageNo );
}


QImage QDocument::pageThumbnail( int pageNo ) const {
    if ( pageNo >= mPages.count() ) {
        return QImage();
    }

    return mPages.at( pageNo )->thumbnail();
}


QString QDocument::pageText( int pageNo ) const {
    return mPages.at( pageNo )->text( QRectF() );
}


QString QDocument::text( int pageNo, QRectF rect ) const {
    return mPages.at( pageNo )->text( rect );
}


QList<QRectF> QDocument::search( QString query, int pageNo, QDocumentRenderOptions opts ) const {
    searchRects.clear();

    if ( pageNo >= 0 or pageNo < mPages.count() ) {
        for ( QRectF rect: mPages.at( pageNo )->search( query, opts ) ) {
            searchRects << QRect( rect.x() * mZoom, rect.y() * mZoom, rect.width() * mZoom, rect.height() * mZoom );
        }
    }

    return searchRects;
}


qreal QDocument::zoomForWidth( int pageNo, qreal width ) const {
    if ( pageNo >= mPages.count() ) {
        return 0.0;
    }

    return 1.0 * width / mPages.at( pageNo )->pageSize().width();
}


qreal QDocument::zoomForHeight( int pageNo, qreal height ) const {
    if ( pageNo >= mPages.count() ) {
        return 0.0;
    }

    return 1.0 * height / mPages.at( pageNo )->pageSize().height();
}


void QDocument::setZoom( qreal zoom ) {
    mZoom = zoom;
}


/*
 * Generic class to handle a document page
 */

QDocumentPage::QDocumentPage( int pgNo ) {
    mPageNo = pgNo;
}


QDocumentPage::~QDocumentPage() {
}


int QDocumentPage::pageNo() {
    return mPageNo;
}
