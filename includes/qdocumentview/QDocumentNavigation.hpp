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

#include <QObject>
#include <qdocumentview/QDocument.hpp>

class QDocument;
class QDocumentNavigation;
class QDocumentNavigationImpl;

class QDocumentNavigation : public QObject {
    Q_OBJECT;

    Q_PROPERTY( QDocument * document READ document WRITE setDocument NOTIFY documentChanged )

    Q_PROPERTY( int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged )
    Q_PROPERTY( int pageCount READ pageCount NOTIFY pageCountChanged )
    Q_PROPERTY( bool canGoToPreviousPage READ canGoToPreviousPage NOTIFY canGoToPreviousPageChanged )
    Q_PROPERTY( bool canGoToNextPage READ canGoToNextPage NOTIFY canGoToNextPageChanged )

    public:
        explicit QDocumentNavigation( QObject *parent = nullptr );
        ~QDocumentNavigation();

        QDocument * document() const;
        void setDocument( QDocument *document );

        int currentPage() const;
        void setCurrentPage( int currentPage );

        int pageCount() const;

        bool canGoToPreviousPage() const;
        bool canGoToNextPage() const;

    public Q_SLOTS:
        void goToPreviousPage();
        void goToNextPage();

    Q_SIGNALS:
        void documentChanged( QDocument *document );
        void currentPageChanged( int currentPage );
        void pageCountChanged( int pageCount );
        void canGoToPreviousPageChanged( bool canGo );
        void canGoToNextPageChanged( bool canGo );

    private:
        QDocumentNavigationImpl *impl;
};
