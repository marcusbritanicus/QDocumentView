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
#include <QtPlugin>

#include <QDocument.hpp>

class QDocumentPluginInterface {
    public:
        explicit QDocumentPluginInterface() {}

        virtual ~QDocumentPluginInterface() {}

        /** Name of the plugin */
        virtual QString name() = 0;

        /** Version of the current plugin */
        virtual QString version() = 0;

        /** Plugin description, like DjVu support for QDV */
        virtual QString description() = 0;

        /** List of supported mimetypes */
        virtual QStringList supportedMimeTypes() = 0;

        /** List of supported extensions */
        virtual QStringList supportedExtensions() = 0;

        /** Instance of `QDocument` */
        virtual QDocument * document( QString ) = 0;
};

Q_DECLARE_INTERFACE( QDocumentPluginInterface, "org.QDocumentView.Plugin" );
