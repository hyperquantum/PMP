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

#include <taglib/id3v2framefactory.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tbytevectorstream.h>

namespace PMP {
	
	FileData::FileData(const HashID& hash, const QString& artist, const QString& title,
      const QString& album, const QString& comment)
	 : _hash(hash), _artist(artist), _title(title), _album(album), _comment(comment)
	{
		//
	}

   FileData* FileData::analyzeFile(QString fileName) {
      QFile file(fileName);
      if (!file.open(QIODevice::ReadOnly)) return 0;
       
      QByteArray fileContents = file.readAll();

      TagLib::ByteVector fileContentsScratch(fileContents.data(), fileContents.length());
      TagLib::ByteVectorStream fileScratchStream(fileContentsScratch);
      
      TagLib::MPEG::File tagFile(&fileScratchStream, TagLib::ID3v2::FrameFactory::instance());
      if (!tagFile.isValid()) { return 0; }
      
      QString artist, title, album, comment;
      
      TagLib::Tag* tag = tagFile.tag();
      if (tag != 0) {
         artist = TStringToQString(tag->artist());
         title = TStringToQString(tag->title());
         album = TStringToQString(tag->album());
         comment = TStringToQString(tag->comment());
      }
      
      tagFile.strip(); // strip all tag headers
      
      TagLib::ByteVector* stripped_data = fileScratchStream.data();
      
      QCryptographicHash md5_hasher(QCryptographicHash::Md5);
      md5_hasher.addData(stripped_data->data(), stripped_data->size());

      QCryptographicHash sha1_hasher(QCryptographicHash::Sha1);
      sha1_hasher.addData(stripped_data->data(), stripped_data->size());
      
      return new FileData(
         HashID(stripped_data->size(), sha1_hasher.result(), md5_hasher.result()),
         artist, title, album, comment
      );
    }
    
}
