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

#include <qdocumentview/QDocumentRenderer.hpp>
#include <qdocumentview/QDocumentRenderOptions.hpp>

class QDocumentPage;

class RenderTask : public QObject, public QRunnable {
    Q_OBJECT;

    public:
        RenderTask( QDocumentPage *pg, QSize imgSz, QDocumentRenderOptions opts, qint64 id );

        int pageNumber();
        qint64 requestId();
        QSize imageSize();

        void invalidate();

        void run();

    private:
        QDocumentPage *mPage;
        QSize mImgSize;
        QDocumentRenderOptions mOpts;
        qint64 mId;

    Q_SIGNALS:
        void imageReady( int pageNo, QImage image, qint64 id );
};
