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

#include <qdocumentview/QDocumentRenderer.hpp>
#include <qdocumentview/QDocument.hpp>

#include "RendererImpl.hpp"

RenderTask::RenderTask( QDocumentPage *pg, QSize imgSz, QDocumentRenderOptions opts, qint64 id ) {
    mPage    = pg;
    mImgSize = imgSz;
    mOpts    = opts;
    mId      = id;
}


int RenderTask::pageNumber() {
    if ( mPage ) {
        return mPage->pageNo();
    }

    else {
        return -1;
    }
}


qint64 RenderTask::requestId() {
    return mId;
}


QSize RenderTask::imageSize() {
    return mImgSize;
}


void RenderTask::invalidate() {
    /* Set the request ID to -1. */
    mId = -1;
}


void RenderTask::run() {
    /* If this task has been invalidated */
    if ( mId < 0 ) {
        return;
    }

    /* If this page is null, do nothing */
    if ( mPage == nullptr ) {
        return;
    }

    /** The size is pre-rotated for the given rotation. */
    QSize tgtSize;
    switch ( mOpts.rotation() ) {
        case QDocumentRenderOptions::Rotate0: {
            tgtSize = QSize( mImgSize.width(), mImgSize.height() );
            break;
        }

        case QDocumentRenderOptions::Rotate90: {
            tgtSize = QSize( mImgSize.height(), mImgSize.width() );
            break;
        }

        case QDocumentRenderOptions::Rotate180: {
            tgtSize = QSize( mImgSize.width(), mImgSize.height() );
            break;
        }

        case QDocumentRenderOptions::Rotate270: {
            tgtSize = QSize( mImgSize.height(), mImgSize.width() );
            break;
        }
    }

    QImage img = mPage->render( tgtSize, mOpts );

    /* Emit only if the task is valid */
    if ( mId > 0 ) {
        emit imageReady( mPage->pageNo(), img, mId );
    }
}


QDocumentRenderer::QDocumentRenderer( QObject *parent ) : QObject( parent ) {
    mDoc = nullptr;
}


void QDocumentRenderer::setDocument( QDocument *doc ) {
    if ( mDoc == doc ) {
        return;
    }

    /* Clear the cache */
    pageCache.clear();
    pages.clear();

    /* Clear the requests */
    for ( int rq = 0; rq < requests.count(); rq++ ) {
        RenderTask *task = requestCache.take( rq );
        /* Invalidate */
        task->invalidate();

        /* Disconnect: Don't waste time validating it when it's complete */
        task->disconnect();

        /* Remove @pg from @requests */
        requests.removeAll( rq );
    }

    /* Clear the queue */
    for ( int q = 0; q < queue.count(); q++ ) {
        RenderTask *task = queuedRequests.take( q );
        /* Invalidate */
        task->invalidate();

        /* Disconnect: Don't waste time validating it when it's complete */
        task->disconnect();

        /* Remove @pg from @requests */
        queue.removeAll( q );
    }

    mDoc      = doc;
    validFrom = QDateTime::currentDateTime().toSecsSinceEpoch();
}


QImage QDocumentRenderer::requestPage( int pg, QSize imgSz, QDocumentRenderOptions opts ) {
    if ( pg >= mDoc->pageCount() ) {
        return QImage();
    }

    /* Check if we have the image in the cache */
    QImage img;

    if ( pages.contains( pg ) ) {
        /* Retrieve the image */
        img = pageCache.value( pg );

        /* If the image has proper size, return it */
        if ( img.size() == imgSz ) {
            return img;
        }
    }

    /* Check if a request has already been made */
    if ( requests.contains( pg ) ) {
        /* Get the request */
        RenderTask *request = requestCache.value( pg );
        QSize      rq       = request->imageSize();

        /* If the image has proper size, return it */
        if ( rq == imgSz ) {
            return (img.isNull() ? img : img.scaled( imgSz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
        }

        /* Request is smaller. Invalidate it and remove from cache */
        else {
            /* Invalidate */
            request->invalidate();

            /* Disconnect: Don't waste time validating it when it's complete */
            request->disconnect();

            /* Remove from cache */
            requestCache.remove( pg );

            /* Remove @pg from @requests */
            requests.removeAll( pg );
        }
    }

    /* Check if a request is in the queue */
    if ( queue.contains( pg ) ) {
        /* Get the request */
        RenderTask *request = queuedRequests.value( pg );
        QSize      rq       = request->imageSize();

        /* If the image has proper size, return it */
        if ( rq == imgSz ) {
            return (img.isNull() ? img : img.scaled( imgSz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
        }

        /* Requested image is smaller. Remove from the queue */
        else {
            /* Invalidate */
            request->invalidate();

            /* Disconnect: Don't waste time validating it when it's complete */
            request->disconnect();

            /* Remove from queue */
            queuedRequests.remove( pg );

            /* Remove @pg from @queue */
            queue.removeAll( pg );
        }
    }

    RenderTask *task = new RenderTask( mDoc->page( pg ), imgSz, opts, QDateTime::currentDateTime().toSecsSinceEpoch() );

    task->setAutoDelete( false );

    connect( task, &RenderTask::imageReady, this, &QDocumentRenderer::validateImage );

    /* Start rendering if there is an available slot */
    if ( requests.count() < requestLimit ) {
        requests << pg;
        requestCache.insert( pg, task );

        /* Start the rendering in a separate thread */
        QThreadPool::globalInstance()->start( task );
    }

    else {
        queue << pg;
        queuedRequests.insert( pg, task );
    }

    return (img.isNull() ? img : img.scaled( imgSz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
}


void QDocumentRenderer::reload() {
    /* The document was reloaded: Clear the page cache */
    pageCache.clear();
    pages.clear();

    /* Clear the requests */
    for ( int rq = 0; rq < requests.count(); rq++ ) {
        /* Get the render task */
        RenderTask *task = requestCache.take( rq );

        /* Invalidate */
        task->invalidate();

        /* Disconnect: Don't waste time validating it when it's complete */
        task->disconnect();

        /* Remove @pg from @requests */
        requests.removeAll( rq );
    }

    /* Clear the queue */
    for ( int q = 0; q < queue.count(); q++ ) {
        RenderTask *task = queuedRequests.take( q );
        /* Invalidate */
        task->invalidate();

        /* Disconnect: Don't waste time validating it when it's complete */
        task->disconnect();

        /* Remove @pg from @requests */
        queue.removeAll( q );
    }

    validFrom = QDateTime::currentDateTime().toSecsSinceEpoch();
}


void QDocumentRenderer::validateImage( int pg, QImage img, qint64 id ) {
    requests.removeAll( pg );
    requestCache.remove( pg );

    if ( id < validFrom ) {
        // The document has changed. All requests made before @validFrom will be invalidated
        return;
    }

    /* Add @pg to @pages if it does not exist */
    if ( not pages.contains( pg ) ) {
        /* If the cache is full, remove the oldest page */
        if ( pages.count() >= pageCacheLimit ) {
            pageCache.remove( pages.takeFirst() );
        }

        pages.append( pg );
    }

    /* Add the @img corresponding to @pg */
    pageCache.insert( pg, img );

    /* Emit the signal that the page is ready */
    emit pageRendered( pg );

    /* Check if there are any outstanding tasks */
    if ( not queue.isEmpty() ) {
        /* Take the first page */
        int        newPg = queue.takeFirst();
        RenderTask *task = queuedRequests.value( newPg );

        requests << newPg;
        requestCache.insert( newPg, task );

        QThreadPool::globalInstance()->start( task );
    }
}
