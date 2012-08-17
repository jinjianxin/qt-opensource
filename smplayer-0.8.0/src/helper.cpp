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

#include "helper.h"

#include <QApplication>
#include <QFileInfo>
#include <QColor>
#include <QDir>
#include <QTextCodec>
#include <QWidget>
#include "config.h"

#ifdef Q_OS_WIN
#include <windows.h> // For the screensaver stuff
#endif

#if EXTERNAL_SLEEP
#include <unistd.h>
#else
#include <qthread.h>
#endif


#if !EXTERNAL_SLEEP
class Sleeper : public QThread
{
public:
	static void sleep(unsigned long secs) {QThread::sleep(secs);}
	static void msleep(unsigned long msecs) {
		QThread::msleep(msecs);
	}
	static void usleep(unsigned long usecs) {QThread::usleep(usecs);}
};
#endif

/*
QString Helper::dvdForPref(const QString & dvd_id, int title) {
	return  QString("DVD_%1_%2").arg(dvd_id).arg(title);
}
*/

QString Helper::formatTime(int secs) {
	int t = secs;
    int hours = (int) t / 3600;
    t -= hours*3600;
    int minutes = (int) t / 60;
    t -= minutes*60;
    int seconds = t;

    QString tf;
    return tf.sprintf("%02d:%02d:%02d",hours,minutes,seconds);
}

QString Helper::timeForJumps(int secs) {
    int minutes = (int) secs / 60;
	int seconds = secs % 60;

	if (minutes==0) {
		return QObject::tr("%n second(s)", "", seconds);
	} else {
		if (seconds==0) 
			return QObject::tr("%n minute(s)", "", minutes);
		else {
			QString m = QObject::tr("%n minute(s)", "", minutes);
			QString s = QObject::tr("%n second(s)", "", seconds);
			return QObject::tr("%1 and %2").arg(m).arg(s);
		}
	}
}

#ifdef Q_OS_WIN
// This function has been copied (and modified a little bit) from Scribus (program under GPL license):
// http://docs.scribus.net/devel/util_8cpp-source.html#l00112
QString Helper::shortPathName(QString long_path) {
	if ((QSysInfo::WindowsVersion >= QSysInfo::WV_NT) && (QFile::exists(long_path))) {
		QString short_path = long_path;

		const int max_path = 4096;
		WCHAR shortName[max_path];

		QString nativePath = QDir::convertSeparators(long_path);
		int ret = GetShortPathNameW((LPCWSTR) nativePath.utf16(), shortName, max_path);
		if (ret != ERROR_INVALID_PARAMETER && ret < MAX_PATH)
			short_path = QString::fromUtf16((const ushort*) shortName);

		return short_path;
	} else {
		return long_path;
	}
}


#endif

void Helper::msleep(int ms) {
#if EXTERNAL_SLEEP
	//qDebug("Helper::msleep: %d (using usleep)", ms);
	usleep(ms*1000);
#else
	//qDebug("Helper::msleep: %d (using QThread::msleep)", ms);
	Sleeper::msleep( ms );
#endif
}

QString Helper::changeSlashes(QString filename) {
	// Only change if file exists (it's a local file)
	if (QFileInfo(filename).exists())
		return filename.replace('/', '\\');
	else
		return filename;
}

bool Helper::directoryContainsDVD(QString directory) {
	//qDebug("Helper::directoryContainsDVD: '%s'", directory.latin1());

	QDir dir(directory);
	QStringList l = dir.entryList();
	bool valid = FALSE;
	for (int n=0; n < l.count(); n++) {
		//qDebug("  * entry %d: '%s'", n, l[n].toUtf8().data());
		if (l[n].toLower() == "video_ts") valid = TRUE;
	}

	return valid;
}

int Helper::qtVersion() {
	QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
	QString v(qVersion());

	int r = 0;

	if (rx.indexIn(v) > -1) {
		int n1 = rx.cap(1).toInt();
		int n2 = rx.cap(2).toInt();
		int n3 = rx.cap(3).toInt();
		r = n1 * 1000 + n2 * 100 + n3;
	}

	qDebug("Helper::qtVersion: %d", r);

	return r;
}

QString Helper::equalizerListToString(AudioEqualizerList values) {
	double v0 = (double) values[0].toInt() / 10;
	double v1 = (double) values[1].toInt() / 10;
	double v2 = (double) values[2].toInt() / 10;
	double v3 = (double) values[3].toInt() / 10;
	double v4 = (double) values[4].toInt() / 10;
	double v5 = (double) values[5].toInt() / 10;
	double v6 = (double) values[6].toInt() / 10;
	double v7 = (double) values[7].toInt() / 10;
	double v8 = (double) values[8].toInt() / 10;
	double v9 = (double) values[9].toInt() / 10;
	QString s = QString::number(v0) + ":" + QString::number(v1) + ":" +
				QString::number(v2) + ":" + QString::number(v3) + ":" +
				QString::number(v4) + ":" + QString::number(v5) + ":" +
				QString::number(v6) + ":" + QString::number(v7) + ":" +
				QString::number(v8) + ":" + QString::number(v9);

	return s;
}

QStringList Helper::searchForConsecutiveFiles(const QString & initial_file) {

	QStringList files_to_add;
	QStringList matching_files;

	QFileInfo fi(initial_file);
	QString basename = fi.completeBaseName();
	QString extension = fi.suffix();
	QString path = fi.absolutePath();

	QDir dir(path);
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Name);

	QRegExp rx("(\\d+)");

	int digits;
	int current_number;
	int pos = 0;
	QString next_name;
	bool next_found = false;
	while  ( ( pos = rx.indexIn(basename, pos) ) != -1 ) {
		digits = rx.cap(1).length();
		current_number = rx.cap(1).toInt() + 1;
		next_name = basename.left(pos) + QString("%1").arg(current_number, digits, 10, QLatin1Char('0'));
		next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
		next_name += "*." + extension;
		matching_files = dir.entryList((QStringList)next_name);

		if ( !matching_files.isEmpty() ) {
			next_found = true;
			break;
		}
		pos  += digits;
	}

	if (next_found) {
		while ( !matching_files.isEmpty() ) {
			files_to_add << path  + "/" + matching_files[0];
			current_number++;
			next_name = basename.left(pos) + QString("%1").arg(current_number, digits, 10, QLatin1Char('0'));
			next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
			next_name += "*." + extension;
			matching_files = dir.entryList((QStringList)next_name);
        }
	}

	return files_to_add;
}
