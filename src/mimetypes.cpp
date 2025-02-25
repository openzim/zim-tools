/*
 * Copyright 2025 Kiwix <kiwix.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "mimetypes.h"

namespace {

MimeMap_t create_extMimeTypes()
{
    return MimeMap_t{
        {extHtml      , mimeTextHtml},
        {extHtm       , mimeTextHtml},
        {extPng       , mimeImagePng},
        {extTiff      , mimeImageTiff},
        {extTif       , mimeImageTiff},
        {extJpeg      , mimeImageJpeg},
        {extJpg       , mimeImageJpeg},
        {extGif       , mimeImageGif},
        {extSvg       , mimeImageSvgXml},
        {extTxt       , mimeTextPlain},
        {extXml       , mimeTextXml},
        {extEpub      , mimeAppEpubZip},
        {extPdf       , mimeAppPdf},
        {extOgg       , mimeAudioOgg},
        {extOgv       , mimeVideoOgg},
        {extJs        , mimeAppJavascript},
        {extJson      , mimeAppJson},
        {extCss       , mimeTextCss},
        {extOtf       , mimeFontOtf},
        {extSfnt      , mimeFontSfnt},
        {extEot       , mimeAppVndMSFontObj},
        {extTtf       , mimeFontTtf},
        {extCollection, mimeFontCollection},
        {extWoff      , mimeFontWoff},
        {extWoff2     , mimeFontWoff2},
        {extVtt       , mimeTextVtt},
        {extWebm      , mimeVideoWebm},
        {extWebp      , mimeImageWebp},
        {extMp4       , mimeVideoMp4},
        {extDoc       , mimeAppMSWord},
        {extDocx      , mimeAppVndXmlWord},
        {extPpt       , mimeAppVndMSPpt},
        {extOdt       , mimeVndOODoc},
        {extOdp       , mimeVndOODoc},
        {extZip       , mimeAppZip},
        {extWasm      , mimeAppWasm}
  };
}

} // namespace

const MimeMap_t& extMimeTypes() {
    static const MimeMap_t mime = create_extMimeTypes();
    return mime;
}

