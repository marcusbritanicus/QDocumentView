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

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <QDocumentRenderOptions.hpp>

class QDocumentPage;
typedef QList<QDocumentPage *> QDocumentPages;

class QDocument : public QObject {
    Q_OBJECT;

    public:
        enum Status {
            Null,
            Loading,
            Ready,
            Unloading,
            Failed
        };
        Q_ENUM( QDocument::Status );

        enum Error {
            NoError,
            UnknownError,
            FileNotFoundError,
            IncorrectPasswordError
        };
        Q_ENUM( QDocument::Error );

        enum MetaDataField {
            Title,
            Subject,
            Author,
            Keywords,
            Producer,
            Creator,
            CreationDate,
            ModificationDate
        };
        Q_ENUM( MetaDataField );

        QDocument( QString docPath );
        virtual ~QDocument() = default;

        /* Check if a password is needed */
        virtual bool passwordNeeded() const;

        /* Set a password */
        virtual void setPassword( QString password ) = 0;

        /* Document File Name and File Path */
        virtual QString fileName() const;
        virtual QString filePath() const;
        virtual QString fileNameAndPath() const;

        /* Pdf Info / Metadata */
        virtual QString title() const    = 0;
        virtual QString author() const   = 0;
        virtual QString creator() const  = 0;
        virtual QString producer() const = 0;
        virtual QString created() const  = 0;

        /* Number of pages */
        int pageCount() const;

        /* Size of the page */
        QSizeF pageSize( int pageNo ) const;

        /* Reload the current document */
        void reload();

        /* PDF load status and error status */
        QDocument::Status status() const;
        QDocument::Error error() const;

        /* Render and return a page */
        QImage renderPage( int, QSize, QDocumentRenderOptions ) const;
        QImage renderPage( int, qreal zoomFactor, QDocumentRenderOptions ) const;

        /* Page Related */
        QDocumentPages pages() const;
        QDocumentPage * page( int pageNo ) const;
        QImage pageThumbnail( int pageNo ) const;

        /* Page Text */
        QString pageText( int pageNo ) const;

        /* Text of a Selection rectangle */
        QString text( int pageNo, QRectF ) const;

        /* Search for @query in @pageNo or all pages */
        QList<QRectF> search( QString query, int pageNo, QDocumentRenderOptions opts ) const;

        qreal zoomForWidth( int pageNo, qreal width ) const;
        qreal zoomForHeight( int pageNo, qreal width ) const;

        void setZoom( qreal zoom );

    public Q_SLOTS:
        virtual void load()  = 0;
        virtual void close() = 0;

    protected:
        QString mDocPath;
        QDocumentPages mPages;

        mutable QList<QRectF> searchRects;

        qreal mZoom;

        Status mStatus;
        Error mError;
        bool mPassNeeded;

    Q_SIGNALS:
        void passwordRequired();
        void statusChanged( QDocument::Status status );
        void pageCountChanged( int pageCount );

        void loading( int );

        void documentReloading();
        void documentReloaded();
};

class QDocumentPage {
    public:
        QDocumentPage( int );
        virtual ~QDocumentPage() = default;

        virtual void setPageData( void *data ) = 0;

        /* Page number of this page */
        virtual int pageNo();

        /* Render and return a page */
        virtual QImage render( QSize, QDocumentRenderOptions ) const              = 0;
        virtual QImage render( qreal zoomFactor, QDocumentRenderOptions ) const   = 0;
        virtual QImage render( int dpiX, int dpiY, QDocumentRenderOptions ) const = 0;

        /* Page Text */
        virtual QString pageText() const = 0;

        /* Text of a Selection rectangle */
        virtual QString text( QRectF ) const = 0;

        /* Search for @query in @pageNo or all pages */
        virtual QList<QRectF> search( QString query, QDocumentRenderOptions opts ) const = 0;

        /* Size of the page */
        virtual QSizeF pageSize( qreal zoom = 1.0 ) const = 0;

        /* Thumbnail of the page */
        virtual QImage thumbnail() const = 0;

    protected:
        int mPageNo = -1;
};
