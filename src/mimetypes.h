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

#ifndef OPENZIM_TOOLS_MIMETYPES_H
#define OPENZIM_TOOLS_MIMETYPES_H

#include <map>
#include <string_view>

inline constexpr std::string_view mimeTextHtml{"text/html"};
inline constexpr std::string_view mimeImagePng{"image/png"};
inline constexpr std::string_view mimeImageTiff{"image/tiff"};
inline constexpr std::string_view mimeImageJpeg{"image/jpeg"};
inline constexpr std::string_view mimeImageGif{"image/gif"};
inline constexpr std::string_view mimeImageSvgXml{"image/svg+xml"};
inline constexpr std::string_view mimeTextPlain{"text/plain"};
inline constexpr std::string_view mimeTextXml{"text/xml"};
inline constexpr std::string_view mimeAppEpubZip{"application/epub+zip"};
inline constexpr std::string_view mimeAppPdf{"application/pdf"};
inline constexpr std::string_view mimeAudioOgg{"audio/ogg"};
inline constexpr std::string_view mimeVideoOgg{"video/ogg"};
inline constexpr std::string_view mimeAppJavascript{"application/javascript"};
inline constexpr std::string_view mimeAppJson{"application/json"};
inline constexpr std::string_view mimeTextCss{"text/css"};
inline constexpr std::string_view mimeFontOtf{"font/otf"};
inline constexpr std::string_view mimeFontSfnt{"font/sfnt"};
inline constexpr std::string_view mimeAppVndMSFontObj{"application/vnd.ms-fontobject"};
inline constexpr std::string_view mimeFontTtf{"font/ttf"};
inline constexpr std::string_view mimeFontCollection{"font/collection"};
inline constexpr std::string_view mimeFontWoff{"font/woff"};
inline constexpr std::string_view mimeFontWoff2{"font/woff2"};
inline constexpr std::string_view mimeTextVtt{"text/vtt"};
inline constexpr std::string_view mimeVideoWebm{"video/webm"};
inline constexpr std::string_view mimeImageWebp{"image/webp"};
inline constexpr std::string_view mimeVideoMp4{"video/mp4"};
inline constexpr std::string_view mimeAppMSWord{"application/msword"};
inline constexpr std::string_view mimeAppVndXmlWord{"application/vnd.openxmlforMats-officedocument.wordprocessingml.document"};
inline constexpr std::string_view mimeAppVndMSPpt{"application/vnd.ms-powerpoint"};
inline constexpr std::string_view mimeVndOODoc{"application/vnd.oasis.openDocument.text"};
inline constexpr std::string_view mimeAppZip{"application/zip"};
inline constexpr std::string_view mimeAppWasm{"application/wasm"};
inline constexpr std::string_view mimeAppOctetStream{"application/octet-stream"};

inline constexpr std::string_view mimeAppFontTtf{"application/font-ttf"}; // TODO: in zimcreatorfs but not in table
inline constexpr std::string_view mimeAppVndMsOpenType{"application/vnd.ms-opentype"}; // TODO: in zimcreatorfs but not in table

inline constexpr std::string_view extHtml       { "html"};
inline constexpr std::string_view extHtm        { "htm"};
inline constexpr std::string_view extPng        { "png"};
inline constexpr std::string_view extTiff       { "tiff"};
inline constexpr std::string_view extTif        { "tif"};
inline constexpr std::string_view extJpeg       { "jpeg"};
inline constexpr std::string_view extJpg        { "jpg"};
inline constexpr std::string_view extGif        { "gif"};
inline constexpr std::string_view extSvg        { "svg"};
inline constexpr std::string_view extTxt        { "txt"};
inline constexpr std::string_view extXml        { "xml"};
inline constexpr std::string_view extEpub       { "epub"};
inline constexpr std::string_view extPdf        { "pdf"};
inline constexpr std::string_view extOgg        { "ogg"};
inline constexpr std::string_view extOgv        { "ogv"};
inline constexpr std::string_view extJs         { "js"};
inline constexpr std::string_view extJson       { "json"};
inline constexpr std::string_view extCss        { "css"};
inline constexpr std::string_view extOtf        { "otf"};
inline constexpr std::string_view extSfnt       { "sfnt"};
inline constexpr std::string_view extEot        { "eot"};
inline constexpr std::string_view extTtf        { "ttf"};
inline constexpr std::string_view extCollection { "collection"};
inline constexpr std::string_view extWoff       { "woff"};
inline constexpr std::string_view extWoff2      { "woff2"};
inline constexpr std::string_view extVtt        { "vtt"};
inline constexpr std::string_view extWebm       { "webm"};
inline constexpr std::string_view extWebp       { "webp"};
inline constexpr std::string_view extMp4        { "mp4"};
inline constexpr std::string_view extDoc        { "doc"};
inline constexpr std::string_view extDocx       { "docx"};
inline constexpr std::string_view extPpt        { "ppt"};
inline constexpr std::string_view extOdt        { "odt"};
inline constexpr std::string_view extOdp        { "odp"};
inline constexpr std::string_view extZip        { "zip"};
inline constexpr std::string_view extWasm       { "wasm"};

namespace detail {

// adapted from cppreference example implementation of lexicographical_compare
struct tolower_str_comparator {
    using UChar_t = unsigned char;

    static constexpr UChar_t toLower(const UChar_t c) noexcept {
        constexpr UChar_t offset {'a' - 'A'};
        return (c <= 'Z' && c >= 'A') ? c + offset : c;
    }

    static constexpr bool charCompare(UChar_t lhs, UChar_t rhs) noexcept {
        return toLower(lhs) < toLower(rhs);
    }

    constexpr bool operator() (const std::string_view lhs, const std::string_view rhs) const noexcept {
        auto first1 = lhs.cbegin();
        auto last1 = lhs.cend();
        auto first2 = rhs.cbegin();
        auto last2 = rhs.cend();

        for (; (first1 != last1) && (first2 != last2); ++first1, ++first2)
        {
            if (charCompare(*first1, *first2)) {
                return true;
            }
            if (charCompare(*first2, *first1)) {
                return false;
            }
        }

        return (first1 == last1) && (first2 != last2);
    }
};

} //namespace detail

using MimeMap_t = std::map<std::string_view, const std::string_view, detail::tolower_str_comparator>;

const MimeMap_t& extMimeTypes();

#endif // OPENZIM_TOOLS_MIMETYPES_H
