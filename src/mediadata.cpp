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

#include "mediadata.h"
#include <QFileInfo>
#include <cmath>


MediaData::MediaData() {
	reset();
}

MediaData::~MediaData() {
}

void MediaData::reset() {
	filename="";
	dvd_id="";
	type = TYPE_UNKNOWN;
	duration=0;

	novideo = FALSE;

	video_width=0;
    video_height=0;
    video_aspect= (double) 4/3;

#if PROGRAM_SWITCH
	programs.clear();
#endif
	videos.clear();
	audios.clear();
	titles.clear();

	subs.clear();

	chapters.clear();

	n_chapters = 0;

	initialized=false;

	// Clip info;
	clip_name = "";
    clip_artist = "";
    clip_author = "";
    clip_album = "";
    clip_genre = "";
    clip_date = "";
    clip_track = "";
    clip_copyright = "";
    clip_comment = "";
    clip_software = "";

	stream_title = "";
	stream_url = "";

	// Other data
	demuxer="";
	video_format="";
	audio_format="";
	video_bitrate=0;
	video_fps="";
	audio_bitrate=0;
	audio_rate=0;
	audio_nch=0;
	video_codec="";
	audio_codec="";
}

QString MediaData::displayName(bool show_tag) {
	if (show_tag) {
		if (!clip_name.isEmpty()) return clip_name;
		else
		if (!stream_title.isEmpty()) return stream_title;
	}

	QFileInfo fi(filename);
	if (fi.exists()) 
		return fi.fileName(); // filename without path
	else
		return filename;
}


void MediaData::list() {
	subs.list();
#if PROGRAM_SWITCH
	programs.list();
#endif
	videos.list();

	audios.list();

	titles.list();

}

