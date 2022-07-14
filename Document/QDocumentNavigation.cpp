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

#include <qdocumentview/QDocumentNavigation.hpp>
#include <qdocumentview/QDocument.hpp>

#include "QDocumentNavigationImpl.hpp"

#include <QPointer>

QDocumentNavigation::QDocumentNavigation( QObject *parent ) : QObject( parent ) {
    impl = new QDocumentNavigationImpl( this );
}


QDocumentNavigation::~QDocumentNavigation() {
    delete impl;
}


QDocument * QDocumentNavigation::document() const {
    return impl->m_document;
}


void QDocumentNavigation::setDocument( QDocument *document ) {
    if ( impl->m_document == document ) {
        return;
    }

    if ( impl->m_document ) {
        disconnect( impl->m_documentStatusChangedConnection );
    }

    impl->m_document = document;
    emit documentChanged( impl->m_document );

    if ( impl->m_document ) {
        impl->m_documentStatusChangedConnection = connect(
            impl->m_document.data(), &QDocument::statusChanged, [ this ](){
                impl->documentStatusChanged();
            }
        );
    }

    impl->update();
}


int QDocumentNavigation::currentPage() const {
    return impl->m_currentPage;
}


void QDocumentNavigation::setCurrentPage( int newPage ) {
    if ( (newPage < 0) || (newPage >= impl->m_pageCount) ) {
        return;
    }

    if ( impl->m_currentPage == newPage ) {
        return;
    }

    impl->m_currentPage = newPage;
    emit currentPageChanged( impl->m_currentPage );

    impl->updatePrevNext();
}


int QDocumentNavigation::pageCount() const {
    return impl->m_pageCount;
}


bool QDocumentNavigation::canGoToPreviousPage() const {
    return impl->m_canGoToPreviousPage;
}


bool QDocumentNavigation::canGoToNextPage() const {
    return impl->m_canGoToNextPage;
}


void QDocumentNavigation::goToPreviousPage() {
    if ( impl->m_currentPage > 0 ) {
        setCurrentPage( impl->m_currentPage - 1 );
    }
}


void QDocumentNavigation::goToNextPage() {
    if ( impl->m_currentPage < impl->m_pageCount - 1 ) {
        setCurrentPage( impl->m_currentPage + 1 );
    }
}
