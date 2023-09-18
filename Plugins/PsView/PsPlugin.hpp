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

#include <qdocumentview/QDocument.hpp>
#include <qdocumentview/QDocumentPluginInterface.hpp>

class PsDocumentPlugin : public QObject, QDocumentPluginInterface {
    Q_OBJECT;

    Q_PLUGIN_METADATA( IID "org.QDocumentView.Plugin" );
    Q_INTERFACES( QDocumentPluginInterface );

    public:
        explicit PsDocumentPlugin() {}

        ~PsDocumentPlugin() {}

        /** Name of the plugin */
        QString name();

        /** Version of the current plugin */
        QString version();

        /** Plugin description, like DjVu support for QDV */
        QString description();

        /** List of supported mimetypes */
        QStringList supportedMimeTypes();

        /** List of supported extensions */
        QStringList supportedExtensions();

        /** Instance of `QDocument` */
        QDocument * document( QString );
};
