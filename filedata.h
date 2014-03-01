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

#include "hashid.h"

namespace PMP {
	
	class FileData {
	public:
		
		static FileData* analyzeFile(QString fileName);
		
		const HashID& hash() { return _hash; }
		QString artist() const { return _artist; }
		QString title() const { return _title; }
		
	private:
		FileData(const HashID& hash, const QString& artist, const QString& title);
		
		HashID _hash;
		QString _artist;
		QString _title;
		
	};
}
#endif
