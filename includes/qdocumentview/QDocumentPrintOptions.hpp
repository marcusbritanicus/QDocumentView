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

#include <QtCore/QObject>

class QDocumentPrintOptions {
    public:
        /** The printer which will be used to print the document */
        QString printerName;

        /** Set the page layout: contains page size, orientation and margins */
        QPageLayout pageLayout;

        /** List of pages to be printed */
        QString pageRange = "all";

        /** Page set: all/odd/even */
        QString pageSet = "all";

        /** Copies? */
        int copies = 1;

        /** Color or grayscale */
        bool color = true;

        /** Collate the pages */
        bool collate = true;

        /** Reverse the print direction */
        bool reverse = false;

        /** Shrink page to fir the printer dimensions */
        bool shrinkToFit = true;

        /** Number of pages per sheet */
        int pagesPerSheet = 1;

        /** pageOrder */
        QString pageOrder = "";

        /** Printer margins */
        QMargins printMargins = QMargins();
};

Q_DECLARE_METATYPE( QDocumentPrintOptions );
