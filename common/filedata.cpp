/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

    This file is part of PMP (Party Music Player).

    PMP is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    PMP is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with PMP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "filedata.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

#include <taglib/id3v2framefactory.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tbytevectorstream.h>

namespace PMP {

    FileData::FileData(const HashID& hash,
        const QString& artist, const QString& title,
        const QString& album, const QString& comment, int lengthInSeconds)
     : _hash(hash), _artist(artist), _title(title),
        _album(album), _comment(comment), _lengthSeconds(lengthInSeconds)
    {
        //
    }

    bool FileData::supportsExtension(QString const& extension) {
        /* TODO: support other file types as well */
        return extension.toLower() == "mp3";
    }

    FileData const* FileData::analyzeFile(const QByteArray& fileContents, const QString& fileExtension) {
        TagLib::ByteVector fileContentsScratch(fileContents.data(), fileContents.length());
        TagLib::ByteVectorStream fileScratchStream(fileContentsScratch);

        TagLib::Tag* tag = 0;

        TagLib::MPEG::File tagFile(&fileScratchStream, TagLib::ID3v2::FrameFactory::instance());
        if (!tagFile.isValid()) { return 0; }

        tag = tagFile.tag();

        QString artist, title, album, comment;

        if (tag != 0) {
            artist = TStringToQString(tag->artist());
            title = TStringToQString(tag->title());
            album = TStringToQString(tag->album());
            comment = TStringToQString(tag->comment());
        }

        int lengthInSeconds = -1;

        TagLib::AudioProperties* audioProperties = tagFile.audioProperties();
        if (audioProperties !=0) {
           lengthInSeconds = audioProperties->length();
        }

        tagFile.strip(); // strip all tag headers

        TagLib::ByteVector* strippedData = fileScratchStream.data();

        return new FileData(
            getHashFrom(strippedData),
            artist, title, album, comment, lengthInSeconds
        );
    }

    FileData const* FileData::analyzeFile(const QString& filename) {
        QFileInfo fileInfo(filename);

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) return 0;

        QByteArray fileContents = file.readAll();
        return analyzeFile(fileContents, fileInfo.suffix());
    }

    HashID FileData::getHashFrom(TagLib::ByteVector* data) {
        QCryptographicHash md5Hasher(QCryptographicHash::Md5);
        md5Hasher.addData(data->data(), data->size());

        QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
        sha1Hasher.addData(data->data(), data->size());

        return HashID(data->size(), sha1Hasher.result(), md5Hasher.result());
    }

}
