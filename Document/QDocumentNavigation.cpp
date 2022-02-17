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

#include "QDocumentNavigation.hpp"
#include "QDocument.hpp"

#include <QPointer>

QDocumentNavigation::QDocumentNavigation( QObject *parent ) : QObject( *new QDocumentNavigationPrivate, parent ) {
}


QDocumentNavigation::~QDocumentNavigation() {
}


QDocument * QDocumentNavigation::document() const {
    Q_D( const QDocumentNavigation );

    return d->m_document;
}


void QDocumentNavigation::setDocument( QDocument *document ) {
    Q_D( QDocumentNavigation );

    if ( d->m_document == document ) {
        return;
    }

    if ( d->m_document ) {
        disconnect( d->m_documentStatusChangedConnection );
    }

    d->m_document = document;
    emit documentChanged( d->m_document );

    if ( d->m_document ) {
        d->m_documentStatusChangedConnection = connect(
            d->m_document.data(), &QDocument::statusChanged, this, [ d ](){
                d->documentStatusChanged();
            }
        );
    }

    d->update();
}


int QDocumentNavigation::currentPage() const {
    Q_D( const QDocumentNavigation );

    return d->m_currentPage;
}


void QDocumentNavigation::setCurrentPage( int newPage ) {
    Q_D( QDocumentNavigation );

    if ( (newPage < 0) || (newPage >= d->m_pageCount) ) {
        return;
    }

    if ( d->m_currentPage == newPage ) {
        return;
    }

    d->m_currentPage = newPage;
    emit currentPageChanged( d->m_currentPage );

    d->updatePrevNext();
}


int QDocumentNavigation::pageCount() const {
    Q_D( const QDocumentNavigation );
    return d->m_pageCount;
}


bool QDocumentNavigation::canGoToPreviousPage() const {
    Q_D( const QDocumentNavigation );
    return d->m_canGoToPreviousPage;
}


bool QDocumentNavigation::canGoToNextPage() const {
    Q_D( const QDocumentNavigation );
    return d->m_canGoToNextPage;
}


void QDocumentNavigation::goToPreviousPage() {
    Q_D( QDocumentNavigation );

    if ( d->m_currentPage > 0 ) {
        setCurrentPage( d->m_currentPage - 1 );
    }
}


void QDocumentNavigation::goToNextPage() {
    Q_D( QDocumentNavigation );

    if ( d->m_currentPage < d->m_pageCount - 1 ) {
        setCurrentPage( d->m_currentPage + 1 );
    }
}
