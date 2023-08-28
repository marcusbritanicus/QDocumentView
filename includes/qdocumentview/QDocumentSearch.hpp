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

class QDocument;

class QDocumentSearch : public QThread {
    Q_OBJECT;

    public:
        QDocumentSearch( QObject *parent = nullptr );
        ~QDocumentSearch();

        /** This will reset the search */
        void setDocument( QDocument *doc );

        /** Set the search string. Call searchPage(...) after this */
        void setSearchString( QString );

        /** Start or queue the search */
        void searchPage( int pageNo );

        /** Call searchPage(...) before calling results(...) */
        QVector<QRectF> results( int );

        /** Stop the search, and hence the thread */
        void stop();

    private:
        QDocument *mDoc;
        QString needle;

        QStack<int> pages;

        int mStartPage = -1;

        QHash<int, QVector<QRectF> > mResults;

        /** Stop completely */
        bool mStop;

        /** Stop-other-loops-and-do-this_page flag */
        bool stop_others;

        /** Matches found so far */
        int matchCount;

    protected:
        /** We perform the actual search here, and emit the signal */
        void run();

    Q_SIGNALS:
        /** Results of @pageNo are ready */
        void resultsReady( int pageNo, QVector<QRectF> );

        void matchesFound( int );
        void searchComplete( int numMatches );

        /** Internal signal - used for prioritizing a particular page */
        void pendingRestart();
};
