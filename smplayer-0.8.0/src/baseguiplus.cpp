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

#include "baseguiplus.h"
#include "config.h"
#include "myaction.h"
#include "global.h"
#include "images.h"
#include "playlist.h"

#ifdef Q_OS_WIN
#include "favorites.h"
#else
#include "tvlist.h"
#endif

#include "widgetactions.h"

#include <QMenu>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>

#if DOCK_PLAYLIST
#include <QDockWidget>
#include "playlistdock.h"
#include "desktopinfo.h"

#define PLAYLIST_ON_SIDES 1
#endif

using namespace Global;

BaseGuiPlus::BaseGuiPlus( QWidget * parent, Qt::WindowFlags flags)
	: BaseGui( parent, flags )
{
	// Initialize variables
	mainwindow_visible = true;
	//infowindow_visible = false;
	trayicon_playlist_was_visible = false;
	widgets_size = 0;
#if DOCK_PLAYLIST
	fullscreen_playlist_was_visible = false;
	fullscreen_playlist_was_floating = false;
	compact_playlist_was_visible = false;
	ignore_playlist_events = false;
#endif


	mainwindow_pos = pos();

	tray = new QSystemTrayIcon( Images::icon("logo", 22), this );

	tray->setToolTip( "SMPlayer" );
	connect( tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
             this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

	quitAct = new MyAction(QKeySequence("Ctrl+Q"), this, "quit");
    connect( quitAct, SIGNAL(triggered()), this, SLOT(quit()) );
	openMenu->addAction(quitAct);

	showTrayAct = new MyAction(this, "show_tray_icon" );
	showTrayAct->setCheckable(true);
	connect( showTrayAct, SIGNAL(toggled(bool)),
             tray, SLOT(setVisible(bool)) );

#ifndef Q_OS_OS2
	optionsMenu->addAction(showTrayAct);
#else
	trayAvailable();
	connect( optionsMenu, SIGNAL(aboutToShow()),
             this, SLOT(trayAvailable()) );
#endif

	showAllAct = new MyAction(this, "restore/hide");
	connect( showAllAct, SIGNAL(triggered()),
             this, SLOT(toggleShowAll()) );


	context_menu = new QMenu(this);
	context_menu->addAction(showAllAct);
	context_menu->addSeparator();
	context_menu->addAction(openFileAct);
	context_menu->addMenu(recentfiles_menu);
	context_menu->addAction(openDirectoryAct);
	context_menu->addAction(openDVDAct);
	context_menu->addAction(openURLAct);
	context_menu->addMenu(favorites);
#ifndef Q_OS_WIN
	context_menu->addMenu(tvlist);
	context_menu->addMenu(radiolist);
#endif
	context_menu->addSeparator();
	context_menu->addAction(playOrPauseAct);
	context_menu->addAction(stopAct);
	context_menu->addSeparator();
	context_menu->addAction(playPrevAct);
	context_menu->addAction(playNextAct);
	context_menu->addSeparator();
	context_menu->addAction(showPlaylistAct);
	context_menu->addAction(showPreferencesAct);
	context_menu->addSeparator();
	context_menu->addAction(quitAct);
	
	tray->setContextMenu( context_menu );

#if DOCK_PLAYLIST
	// Playlistdock
	playlistdock = new PlaylistDock(this);
	playlistdock->setObjectName("playlistdock");
	playlistdock->setFloating(false); // To avoid that the playlist is visible for a moment
	playlistdock->setWidget(playlist);
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea | 
                                  Qt::BottomDockWidgetArea
#if PLAYLIST_ON_SIDES
                                  | Qt::LeftDockWidgetArea | 
                                  Qt::RightDockWidgetArea
#endif
                                  );
	addDockWidget(Qt::BottomDockWidgetArea, playlistdock);
	playlistdock->hide();
	playlistdock->setFloating(true); // Floating by default

	connect( playlistdock, SIGNAL(closed()), this, SLOT(playlistClosed()) );
#if USE_DOCK_TOPLEVEL_EVENT
	connect( playlistdock, SIGNAL(topLevelChanged(bool)), 
             this, SLOT(dockTopLevelChanged(bool)) );
#else
	connect( playlistdock, SIGNAL(visibilityChanged(bool)), 
             this, SLOT(dockVisibilityChanged(bool)) );
#endif // USE_DOCK_TOPLEVEL_EVENT

	ignore_playlist_events = false;
#endif // DOCK_PLAYLIST

	retranslateStrings();

    loadConfig();
}

BaseGuiPlus::~BaseGuiPlus() {
	saveConfig();
}

bool BaseGuiPlus::startHidden() {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	return false;
#else
	if ( (!showTrayAct->isChecked()) || (mainwindow_visible) ) 
		return false;
	else
		return true;
#endif
}

void BaseGuiPlus::closeEvent( QCloseEvent * e ) {
	e->ignore();
	closeWindow();
}

void BaseGuiPlus::closeWindow() {

	if (tray->isVisible()) {
		//e->ignore();
		exitFullscreen();
		showAll(false); // Hide windows
		if (core->state() == Core::Playing) core->stop();

		if (pref->balloon_count > 0) {
			tray->showMessage( "SMPlayer", 
				tr("SMPlayer is still running here"), 
        	    QSystemTrayIcon::Information, 3000 );
			pref->balloon_count--;
		}

	} else {
		BaseGui::closeWindow();
	}
	//tray->hide();

}

void BaseGuiPlus::quit() {
	BaseGui::closeWindow();
}

void BaseGuiPlus::retranslateStrings() {
	BaseGui::retranslateStrings();

	quitAct->change( Images::icon("exit"), tr("&Quit") );
	showTrayAct->change( Images::icon("systray"), tr("S&how icon in system tray") );

	updateShowAllAct();

#if DOCK_PLAYLIST
    playlistdock->setWindowTitle( tr("Playlist") );
#endif
}

void BaseGuiPlus::updateShowAllAct() {
	if (isVisible()) 
		showAllAct->change( tr("&Hide") );
	else
		showAllAct->change( tr("&Restore") );
}

void BaseGuiPlus::saveConfig() {

	QSettings * set = settings;

	set->beginGroup( "base_gui_plus");

	set->setValue( "show_tray_icon", showTrayAct->isChecked() );
	set->setValue( "mainwindow_visible", isVisible() );

	set->setValue( "trayicon_playlist_was_visible", trayicon_playlist_was_visible );
	set->setValue( "widgets_size", widgets_size );
#if DOCK_PLAYLIST
	set->setValue( "fullscreen_playlist_was_visible", fullscreen_playlist_was_visible );
	set->setValue( "fullscreen_playlist_was_floating", fullscreen_playlist_was_floating );
	set->setValue( "compact_playlist_was_visible", compact_playlist_was_visible );
	set->setValue( "ignore_playlist_events", ignore_playlist_events );
#endif

/*
#if DOCK_PLAYLIST
	set->setValue( "playlist_and_toolbars_state", saveState() );
#endif
*/

	set->endGroup();
}

void BaseGuiPlus::loadConfig() {

	QSettings * set = settings;

	set->beginGroup( "base_gui_plus");

	bool show_tray_icon = set->value( "show_tray_icon", false).toBool();
	showTrayAct->setChecked( show_tray_icon );
	//tray->setVisible( show_tray_icon );

	mainwindow_visible = set->value("mainwindow_visible", true).toBool();

	trayicon_playlist_was_visible = set->value( "trayicon_playlist_was_visible", trayicon_playlist_was_visible ).toBool();
	widgets_size = set->value( "widgets_size", widgets_size ).toInt();
#if DOCK_PLAYLIST
	fullscreen_playlist_was_visible = set->value( "fullscreen_playlist_was_visible", fullscreen_playlist_was_visible ).toBool();
	fullscreen_playlist_was_floating = set->value( "fullscreen_playlist_was_floating", fullscreen_playlist_was_floating ).toBool();
	compact_playlist_was_visible = set->value( "compact_playlist_was_visible", compact_playlist_was_visible ).toBool();
	ignore_playlist_events = set->value( "ignore_playlist_events", ignore_playlist_events ).toBool();
#endif

/*
#if DOCK_PLAYLIST
	restoreState( set->value( "playlist_and_toolbars_state" ).toByteArray() );
#endif
*/

	set->endGroup();

	updateShowAllAct();
}


void BaseGuiPlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {

	updateShowAllAct();

	if (reason == QSystemTrayIcon::Trigger) {
		toggleShowAll();
	}
	else
	if (reason == QSystemTrayIcon::MiddleClick) {
		core->pause();
	}
}

void BaseGuiPlus::toggleShowAll() {
	// Ignore if tray is not visible
	if (tray->isVisible()) {
		showAll( !isVisible() );
	}
}

void BaseGuiPlus::showAll(bool b) {
	if (!b) {
		// Hide all
#if DOCK_PLAYLIST
		trayicon_playlist_was_visible = (playlistdock->isVisible() && 
                                         playlistdock->isFloating() );
		if (trayicon_playlist_was_visible)
			playlistdock->hide();
#else
		trayicon_playlist_was_visible = playlist->isVisible();
		playlist_pos = playlist->pos();
		playlist->hide();
#endif

		mainwindow_pos = pos();
		hide();
	} else {
		// Show all
		move(mainwindow_pos);
		show();

#if DOCK_PLAYLIST
		if (trayicon_playlist_was_visible) {
			playlistdock->show();
		}
#else
		if (trayicon_playlist_was_visible) {
			playlist->move(playlist_pos);
			playlist->show();
		}
#endif

	}
	updateShowAllAct();
}

void BaseGuiPlus::resizeWindow(int w, int h) {

	if ( (tray->isVisible()) && (!isVisible()) ) showAll(true);

	BaseGui::resizeWindow(w, h );
}

void BaseGuiPlus::updateMediaInfo() {
	BaseGui::updateMediaInfo();

	tray->setToolTip( windowTitle() );
}

void BaseGuiPlus::setWindowCaption(const QString & title) {
	tray->setToolTip( title );

	BaseGui::setWindowCaption( title );
}


// Playlist stuff
void BaseGuiPlus::aboutToEnterFullscreen() {

	BaseGui::aboutToEnterFullscreen();

#if DOCK_PLAYLIST
	playlistdock->setAllowedAreas(Qt::NoDockWidgetArea);

	int playlist_screen = QApplication::desktop()->screenNumber(playlistdock);
	int mainwindow_screen = QApplication::desktop()->screenNumber(this);

	fullscreen_playlist_was_visible = playlistdock->isVisible();
	fullscreen_playlist_was_floating = playlistdock->isFloating();

	ignore_playlist_events = true;

	// Hide the playlist if it's in the same screen as the main window
    if ((playlist_screen == mainwindow_screen)  )
	{
		playlistdock->setFloating(true);
		playlistdock->hide();
	}
#endif
}

void BaseGuiPlus::aboutToExitFullscreen() {

	BaseGui::aboutToExitFullscreen();

#if DOCK_PLAYLIST
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea | 
                                  Qt::BottomDockWidgetArea
                                  #if PLAYLIST_ON_SIDES
                                  | Qt::LeftDockWidgetArea | 
                                  Qt::RightDockWidgetArea
                                  #endif
                                  );

	if (fullscreen_playlist_was_visible) {
		playlistdock->show();
	}
	playlistdock->setFloating( fullscreen_playlist_was_floating );
	ignore_playlist_events = false;
#endif
}

void BaseGuiPlus::aboutToEnterCompactMode() {
#if DOCK_PLAYLIST
	compact_playlist_was_visible = (playlistdock->isVisible() && 
                                    !playlistdock->isFloating());
	if (compact_playlist_was_visible)
		playlistdock->hide();
#endif

    widgets_size = height() - panel->height();

	BaseGui::aboutToEnterCompactMode();

	if (pref->resize_method == Preferences::Always) {
		resize( width(), height() - widgets_size );
	}
}

void BaseGuiPlus::aboutToExitCompactMode() {
	BaseGui::aboutToExitCompactMode();

	if (pref->resize_method == Preferences::Always) {
		resize( width(), height() + widgets_size );
	}

#if DOCK_PLAYLIST
	if (compact_playlist_was_visible)
		playlistdock->show();
#endif
}

#if DOCK_PLAYLIST
void BaseGuiPlus::showPlaylist(bool b) {

	if ( !b ) {
		playlistdock->hide();
	} else {
		exitFullscreenIfNeeded();
		playlistdock->show();

		if (playlistdock->isFloating()) {
			if (!DesktopInfo::isInsideScreen(playlistdock)) {
				playlistdock->move(0,0);
			}
		}
	}

}

void BaseGuiPlus::playlistClosed() {
	showPlaylistAct->setChecked(false);
}

#if !USE_DOCK_TOPLEVEL_EVENT
void BaseGuiPlus::dockVisibilityChanged(bool visible) {

	if (!playlistdock->isFloating()) {
		if (!visible) shrinkWindow(); else stretchWindow();
	}
}

#else

void BaseGuiPlus::dockTopLevelChanged(bool floating) {

	if (floating) shrinkWindow(); else stretchWindow();
}
#endif

void BaseGuiPlus::stretchWindow() {
	if ((ignore_playlist_events) || (pref->resize_method!=Preferences::Always)) return;

	if ( (dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea) )
	{
		int new_height = height() + playlistdock->height();
		resize( width(), new_height );

	}

	else

	if ( (dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea) )
	{
		int new_width = width() + playlistdock->width();

		resize( new_width, height() );
	}
}

void BaseGuiPlus::shrinkWindow() {
	if ((ignore_playlist_events) || (pref->resize_method!=Preferences::Always)) return;


	if ( (dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea) )
	{
		int new_height = height() - playlistdock->height();
		resize( width(), new_height );

	}

	else

	if ( (dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea) )
	{
		int new_width = width() - playlistdock->width();

		resize( new_width, height() );
	}
}

#endif

TimeSliderAction * BaseGuiPlus::createTimeSliderAction(QWidget * parent) {
	TimeSliderAction * timeslider_action = new TimeSliderAction( parent );
	timeslider_action->setObjectName("timeslider_action");

#ifdef SEEKBAR_RESOLUTION
	connect( timeslider_action, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPosition(int)) );
	connect( core, SIGNAL(positionChanged(int)), 
             timeslider_action, SLOT(setPos(int)) );
#else
	connect( timeslider_action, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPos(int)) );
	connect( core, SIGNAL(posChanged(int)), 
             timeslider_action, SLOT(setPos(int)) );
#endif
	connect( timeslider_action, SIGNAL( draggingPos(int) ), 
             this, SLOT(displayGotoTime(int)) );
#if ENABLE_DELAYED_DRAGGING
	timeslider_action->setDragDelay( pref->time_slider_drag_delay );

	connect( timeslider_action, SIGNAL( delayedDraggingPos(int) ), 
             this, SLOT(goToPosOnDragging(int)) );
#else
	connect( timeslider_action, SIGNAL( draggingPos(int) ), 
             this, SLOT(goToPosOnDragging(int)) );
#endif
	return timeslider_action;
}

VolumeSliderAction * BaseGuiPlus::createVolumeSliderAction(QWidget * parent) {
	VolumeSliderAction * volumeslider_action = new VolumeSliderAction(parent);
	volumeslider_action->setObjectName("volumeslider_action");

	connect( volumeslider_action, SIGNAL( valueChanged(int) ), 
             core, SLOT( setVolume(int) ) );
	connect( core, SIGNAL(volumeChanged(int)),
             volumeslider_action, SLOT(setValue(int)) );

	return volumeslider_action;
}

#ifdef Q_OS_OS2
void BaseGuiPlus::trayAvailable() {
	if (!tray->isSystemTrayAvailable()) {
			optionsMenu->removeAction(showTrayAct);
	}
	else {
		optionsMenu->addAction(showTrayAct);
	}
}
#endif

#include "moc_baseguiplus.cpp"
