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

    FileData::FileData(const HashID& hash)
     : _hash(hash)
    {
        //
    }

    FileData::FileData(const HashID& hash,
        const QString& artist, const QString& title,
        const QString& album, const QString& comment,
        AudioData::FileFormat format, int trackLength)
     : _hash(hash),
       _audio(format, trackLength),
       _tags(artist, title, album, comment)
    {
        //
    }

    bool FileData::supportsExtension(QString const& extension) {
        QString lowercaseExtension = extension.toLower();

        /* TODO: support other file types as well */
        return lowercaseExtension == "mp3";
            //|| lowercaseExtension == "wma" || lowercaseExtension == "asf";
    }

    FileData FileData::analyzeFile(const QByteArray& fileContents, const QString& fileExtension) {
        TagLib::ByteVector fileContentsScratch(fileContents.data(), fileContents.length());
        TagLib::ByteVectorStream fileScratchStream(fileContentsScratch);

        QString lowercaseExtension = fileExtension.toLower();

        if (lowercaseExtension == "mp3") {
            return analyzeMp3(fileScratchStream);
        }
        else {
            /* file type not (yet) supported */
            return FileData(HashID());
        }
    }

    FileData FileData::analyzeFile(const QString& filename) {
        QFileInfo fileInfo(filename);

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) return FileData(HashID());

        QByteArray fileContents = file.readAll();
        return analyzeFile(fileContents, fileInfo.suffix());
    }

    FileData FileData::create(const HashID& hash,
        const QString& artist, const QString& title,
        AudioData::FileFormat format, int length)
    {
        return FileData(hash, artist, title, "", "", format, length);
    }

    HashID FileData::getHashFrom(TagLib::ByteVector* data) {
        uint size = data->size();

        QCryptographicHash md5Hasher(QCryptographicHash::Md5);
        md5Hasher.addData(data->data(), size);

        QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
        sha1Hasher.addData(data->data(), size);

        return HashID(size, sha1Hasher.result(), md5Hasher.result());
    }

    void FileData::getDataFromTag(const TagLib::Tag* tag,
                                  QString& artist, QString& title,
                                  QString& album, QString& comment)
    {
        if (!tag) { return; }

        artist = TStringToQString(tag->artist());
        title = TStringToQString(tag->title());
        album = TStringToQString(tag->album());
        comment = TStringToQString(tag->comment());
    }

    FileData FileData::analyzeMp3(TagLib::ByteVectorStream& fileContents)
    {
        TagLib::MPEG::File tagFile(&fileContents, TagLib::ID3v2::FrameFactory::instance());
        if (!tagFile.isValid()) { return FileData(HashID()); }

        QString artist, title, album, comment;
        getDataFromTag(tagFile.tag(), artist, title, album, comment);

        int lengthInSeconds = -1;
        TagLib::AudioProperties* audioProperties = tagFile.audioProperties();
        if (audioProperties != 0) {
           lengthInSeconds = audioProperties->length();
        }

        tagFile.strip(); /* strip all tag headers */

        TagLib::ByteVector* strippedData = fileContents.data();

        return FileData(
            getHashFrom(strippedData),
            artist, title, album, comment,
            AudioData::MP3, lengthInSeconds
        );
    }

    //FileData const* FileData::analyzeWma(TagLib::ByteVectorStream& fileContents) { }

}
