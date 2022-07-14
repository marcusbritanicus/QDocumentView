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
#include <qdocumentview/QDocumentNavigation.hpp>

class QDocumentNavigationImpl : public QObject {
    Q_OBJECT;

    public:
        QDocumentNavigationImpl( QDocumentNavigation *qq ) : QObject() {
            publ = qq;
        }

        void update() {
            const bool documentAvailable = m_document && m_document->status() == QDocument::Ready;

            if ( documentAvailable ) {
                const int newPageCount = m_document->pageCount();

                if ( m_pageCount != newPageCount ) {
                    m_pageCount = newPageCount;
                    emit publ->pageCountChanged( m_pageCount );
                }
            }

            else {
                if ( m_pageCount != 0 ) {
                    m_pageCount = 0;
                    emit publ->pageCountChanged( m_pageCount );
                }
            }

            if ( m_currentPage != 0 ) {
                m_currentPage = 0;
                emit publ->currentPageChanged( m_currentPage );
            }

            updatePrevNext();
        }

        void updatePrevNext() {
            const bool hasPreviousPage = m_currentPage > 0;
            const bool hasNextPage     = m_currentPage < (m_pageCount - 1);

            if ( m_canGoToPreviousPage != hasPreviousPage ) {
                m_canGoToPreviousPage = hasPreviousPage;
                emit publ->canGoToPreviousPageChanged( m_canGoToPreviousPage );
            }

            if ( m_canGoToNextPage != hasNextPage ) {
                m_canGoToNextPage = hasNextPage;
                emit publ->canGoToNextPageChanged( m_canGoToNextPage );
            }
        }

        void documentStatusChanged() {
            update();
        }

        QPointer<QDocument> m_document = nullptr;
        int m_currentPage          = 0;
        int m_pageCount            = 0;
        bool m_canGoToPreviousPage = false;
        bool m_canGoToNextPage     = false;

        QMetaObject::Connection m_documentStatusChangedConnection;

    private:
        QDocumentNavigation *publ;
};
