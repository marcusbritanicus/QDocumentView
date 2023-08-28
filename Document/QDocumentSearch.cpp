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

#include <qdocumentview/QDocument.hpp>
#include <qdocumentview/QDocumentSearch.hpp>

QDocumentSearch::QDocumentSearch( QObject *parent )
    : QThread( parent )
    , mDoc( nullptr )
    , mStop( false )
    , stop_others( false )
    , matchCount( 0 ) {
    connect(
        this, &QDocumentSearch::pendingRestart, [ this ]() {
            stop_others = false;
            mStartPage  = pages[ 0 ];
            start();
        }
    );
}


QDocumentSearch::~QDocumentSearch() {
    mStop = true;
    wait();

    matchCount = 0;
    emit matchesFound( matchCount );

    mDoc->deleteLater();
}


void QDocumentSearch::setDocument( QDocument *doc ) {
    if ( mDoc == doc ) {
        return;
    }

    mStop = true;

    matchCount = 0;
    emit matchesFound( matchCount );

    needle = QString();
    pages.clear();
    mResults.clear();

    mDoc = doc;
}


void QDocumentSearch::setSearchString( QString str ) {
    if ( needle == str.toLower() ) {
        emit searchComplete( matchCount );
        return;
    }

    mStop = true;

    mStartPage = -1;

    matchCount = 0;
    emit matchesFound( matchCount );

    needle = str.toLower();
    pages.clear();
    mResults.clear();
}


void QDocumentSearch::searchPage( int pageNo ) {
    /** If the document is not set */
    if ( not mDoc ) {
        qCritical() << "Please set the document with ::setDocument(...) before beginning search.";
        return;
    }

    /** Already queued */
    if ( pages.contains( pageNo ) ) {
        return;
    }

    /** Already searched */
    if ( mResults.contains( pageNo ) ) {
        return;
    }

    /** Empty search string */
    if ( needle.isEmpty() ) {
        return;
    }

    if ( mStartPage == -1 ) {
        mStartPage = pageNo;
    }

    /** Add to the list */
    pages.push( pageNo );

    mStop = false;

    if ( isRunning() ) {
        stop_others = true;
    }

    if ( not isRunning() ) {
        mStop       = false;
        stop_others = false;
        start();
    }
}


QVector<QRectF> QDocumentSearch::results( int pageNo ) {
    return mResults.value( pageNo );
}


void QDocumentSearch::stop() {
    mStop = true;
}


void QDocumentSearch::run() {
    /** Pages requested by the user */
    while ( pages.count() ) {
        if ( mStop ) {
            return;
        }

        int curPage = pages.pop();

        QList<QRectF> _results = mDoc->search( needle, curPage, QDocumentRenderOptions() );
        matchCount += _results.length();

        /** Emit signal only if new search results were obtained */
        if ( _results.length() ) {
            emit matchesFound( matchCount );
        }

        mResults[ curPage ] = QVector<QRectF>::fromList( _results );

        /** If we've stopped, then we don't want any signals being emitted */
        if ( not mStop ) {
            /** Emit signal only if new search results were obtained */
            if ( _results.length() ) {
                emit resultsReady( curPage, mResults[ curPage ] );
            }
        }
    }

    /** Search all the subsequent pages */
    for ( int pg = mStartPage + 1; pg < mDoc->pageCount(); pg++ ) {
        if ( mStop ) {
            return;
        }

        if ( stop_others ) {
            emit pendingRestart();
            return;
        }

        /** Already finished with @pg */
        if ( mResults.contains( pg ) ) {
            continue;
        }

        QList<QRectF> _results = mDoc->search( needle, pg, QDocumentRenderOptions() );

        matchCount += _results.count();

        /** Emit signal only if new search results were obtained */
        if ( _results.length() ) {
            emit matchesFound( matchCount );
        }

        mResults[ pg ] = QVector<QRectF>::fromList( _results );

        /** If we've stopped, then we don't want any signals being emitted */
        if ( not mStop ) {
            /** Emit signal only if new search results were obtained */
            if ( _results.length() ) {
                emit resultsReady( pg, mResults[ pg ] );
            }
        }
    }

    /** Search all the pages from the beginning till the starting page */
    for ( int pg = 0; pg < mStartPage; pg++ ) {
        if ( mStop ) {
            return;
        }

        if ( stop_others ) {
            emit pendingRestart();
            return;
        }

        /** Already finished with @pg */
        if ( mResults.contains( pg ) ) {
            continue;
        }

        QList<QRectF> _results = mDoc->search( needle, pg, QDocumentRenderOptions() );

        matchCount += _results.count();

        /** Emit signal only if new search results were obtained */
        if ( _results.length() ) {
            emit matchesFound( matchCount );
        }

        mResults[ pg ] = QVector<QRectF>::fromList( _results );

        /** If we've stopped, then we don't want any signals being emitted */
        if ( not mStop ) {
            /** Emit signal only if new search results were obtained */
            if ( _results.length() ) {
                emit resultsReady( pg, mResults[ pg ] );
            }
        }
    }

    /** We've reached here ==> Search of all pages must be complete. */
    if ( mResults.keys().count() == mDoc->pageCount() ) {
        emit searchComplete( matchCount );
    }

    mStartPage = -1;
}
