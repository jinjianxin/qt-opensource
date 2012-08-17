/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2012 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "filesettings.h"
#include "mediasettings.h"
#include <QSettings>
#include <QFileInfo>

FileSettings::FileSettings(QString directory) : FileSettingsBase(directory) 
{
	my_settings = new QSettings(directory + "/smplayer_files.ini", QSettings::IniFormat);
}

FileSettings::~FileSettings() {
	delete my_settings;
}

QString FileSettings::filenameToGroupname(const QString & filename) {
	QString s = filename;
	s = s.replace('/', '_');
	s = s.replace('\\', '_');
	s = s.replace(':', '_');
	s = s.replace('.', '_');
	s = s.replace(' ', '_');

	QFileInfo fi(filename);
	if (fi.exists()) {
		s += "_" + QString::number( fi.size() );
	}

	return s;	
}

bool FileSettings::existSettingsFor(QString filename) {

	QString group_name = filenameToGroupname(filename);

	my_settings->beginGroup( group_name );
	bool saved = my_settings->value("saved", false).toBool();
	my_settings->endGroup();

	return saved;
}

void FileSettings::loadSettingsFor(QString filename, MediaSettings & mset) {


	QString group_name = filenameToGroupname(filename);

	mset.reset();
	my_settings->beginGroup( group_name );
	mset.load(my_settings);
	my_settings->endGroup();
}

void FileSettings::saveSettingsFor(QString filename, MediaSettings & mset) {


	QString group_name = filenameToGroupname(filename);

	my_settings->beginGroup( group_name );
	my_settings->setValue("saved", true);
	mset.save(my_settings);
	my_settings->endGroup();
}

