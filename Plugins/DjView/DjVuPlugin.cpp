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

#include "DjVuDocument.hpp"

#include "DjVuPlugin.hpp"

QString DjVuDocumentPlugin::name() {
    return "DjVu Plugin";
}


QString DjVuDocumentPlugin::version() {
    return PROJECT_VERSION;
}


QString DjVuDocumentPlugin::description() {
    return "Plugin to load DjVu documents for QDocumentView";
}


QStringList DjVuDocumentPlugin::supportedMimeTypes() {
    return {
        "image/vnd.djvu",
        "image/vnd.djvu+multipage",
    };
}


QStringList DjVuDocumentPlugin::supportedExtensions() {
    return {
        "djv",
        "djvu",
    };
}


QDocument *DjVuDocumentPlugin::document( QString docPath ) {
    return new DjVuDocument( docPath );
}
