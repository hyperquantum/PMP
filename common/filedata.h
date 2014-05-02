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

#ifndef PMP_FILEDATA_H
#define PMP_FILEDATA_H

#include "audiodata.h"
#include "hashid.h"
#include "tagdata.h"

namespace TagLib {
    class ByteVector;
    class ByteVectorStream;
    class Tag;
}

namespace PMP {

    class FileData : public AudioData, public TagData {
    public:

        static bool supportsExtension(QString const& extension);

        static FileData const* analyzeFile(const QByteArray& fileContents, const QString& fileExtension);
        static FileData const* analyzeFile(const QString& filename);

        static FileData const* create(const HashID& hash,
                                      const QString& artist, const QString& title,
                                      FileFormat format = UnknownFormat, int trackLength = -1);

        const HashID& hash() const { return _hash; }

    private:
        FileData(const HashID& hash,
            const QString& artist, const QString& title,
            const QString& album, const QString& comment,
            FileFormat format, int trackLength);

        static FileData const* analyzeMp3(TagLib::ByteVectorStream& fileContents);
        //static FileData const* analyzeWma(TagLib::ByteVectorStream& fileContents);

        static void getDataFromTag(const TagLib::Tag* tag,
                                   QString& artist, QString& title,
                                   QString& album, QString& comment);

        static HashID getHashFrom(TagLib::ByteVector* data);

        HashID _hash;
    };
}
#endif
