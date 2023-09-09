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

#include <QDocument.hpp>
#include <QDocumentRenderOptions.hpp>

#include <libspectre/spectre-document.h>

class PsDocument : public QDocument {
    Q_OBJECT;

    public:
        PsDocument( QString djvuPath );
        ~PsDocument();

        /* Set a password */
        void setPassword( QString password );

        /* Pdf Info / Metadata */
        QString title() const;
        QString author() const;
        QString creator() const;
        QString producer() const;
        QString created() const;

    public Q_SLOTS:
        void load();
        void close();

    private:
        /* Pointer to our actual djvu document */
        SpectreDocument* mPsDoc;
        SpectreRenderContext* mRenderCtxt;
};

class PsPage : public QDocumentPage {
    public:
        PsPage( int, SpectreRenderContext * );
        ~PsPage();

        /* Way to store Poppler::Page */
        void setPageData( void *data );

        /* Size of the page */
        QSizeF pageSize( qreal zoom = 1.0 ) const;

        /* Thumbnail of the page */
        QImage thumbnail() const;

        /* Render and return a page */
        QImage render( QSize, QDocumentRenderOptions ) const;
        QImage render( qreal zoomFactor, QDocumentRenderOptions ) const;
        QImage render( int dpiX, int dpiY, QDocumentRenderOptions ) const;

        /* Page Text */
        QString pageText() const;

        /* Text of a Selection rectangle */
        QString text( QRectF ) const;

        /* Search for @query in @pageNo or all pages */
        QList<QRectF> search( QString query, QDocumentRenderOptions ) const;

    private:
        SpectrePage *mPage;
        SpectreRenderContext* mRenderCtxt;

        QSizeF mPageSize;
};
