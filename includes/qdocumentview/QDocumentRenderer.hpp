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

class RenderTask;
class QDocument;

class QDocumentRenderer : public QObject {
    Q_OBJECT;

    public:
        QDocumentRenderer( QObject *parent = nullptr );

        void setDocument( QDocument * );
        QImage requestPage( int pg, QSize imgSz, QDocumentRenderOptions opts );

        void reload();

    private:
        QDocument *mDoc;
        qint64 validFrom = -1;

        void validateImage( int pg, QImage img, qint64 id );

        QHash<int, QImage> pageCache;
        QVector<int> pages;
        int pageCacheLimit = 20;

        QHash<int, RenderTask *> requestCache;
        QVector<int> requests;
        int requestLimit = 5;

        QHash<int, RenderTask *> queuedRequests;
        QVector<int> queue;

    Q_SIGNALS:
        void pageRendered( int );
};
