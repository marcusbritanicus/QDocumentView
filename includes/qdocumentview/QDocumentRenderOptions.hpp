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

class QDocumentRenderOptions {
    public:
        enum Rotation {
            Rotate0,
            Rotate90,
            Rotate180,
            Rotate270
        };

        enum RenderFlag {
            NoRenderFlags         = 0x000,
            RenderAnnotations     = 0x001,
            RenderOptimizedForLcd = 0x002,
            RenderGrayscale       = 0x004,
            RenderForceHalftone   = 0x008,
            RenderTextAliased     = 0x010,
            RenderImageAliased    = 0x020,
            RenderPathAliased     = 0x040
        };
        Q_DECLARE_FLAGS( RenderFlags, RenderFlag );

        QDocumentRenderOptions() : data( 0 ) {}

        Rotation rotation() const {
            return static_cast<Rotation>(bits.rotation);
        }

        void setRotation( Rotation _rotation ) {
            bits.rotation = _rotation;
        }

        RenderFlags renderFlags() const {
            return static_cast<RenderFlags>(bits.renderFlags);
        }

        void setRenderFlags( RenderFlags _renderFlags ) {
            bits.renderFlags = _renderFlags;
        }

    private:
        friend inline bool operator==( QDocumentRenderOptions lhs, QDocumentRenderOptions rhs );

        struct Bits {
            quint32 renderFlags : 8;
            quint32 rotation    : 3;
            quint32 reserved    : 21;
            quint32 reserved2   : 32;
        };

        union {
            Bits    bits;
            quint64 data;
        };
};

inline bool operator==( QDocumentRenderOptions lhs, QDocumentRenderOptions rhs ) {
    return lhs.data == rhs.data;
}


inline bool operator!=( QDocumentRenderOptions lhs, QDocumentRenderOptions rhs ) {
    return !operator==( lhs, rhs );
}


Q_DECLARE_METATYPE( QDocumentRenderOptions );
