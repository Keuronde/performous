#pragma once

#include "animvalue.hh"
#include "cachemap.hh"
#include "controllers.hh"
#include "screen.hh"
#include "song.hh" // for Music class
#include "textinput.hh"
#include "video.hh"

#include <boost/scoped_ptr.hpp>

class Audio;
class Database;
class Song;
class Songs;
class Surface;
class ThemeSongs;

/// song chooser screen
class ScreenSongs : public Screen {
public:
	/// constructor
	ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database);
	void enter();
	void exit();
	void reloadGL();
	void manageSharedKey(input::NavButton nav); ///< same behaviour for jukebox and normal mode
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void prepare();
	void draw();
	void drawCovers(); ///< draw the cover browser
	Surface& getCover(Song const& song); ///< get appropriate cover image for the song (incl. no cover)
	void drawJukebox(); ///< draw the songbrowser in jukebox mode (fullscreen, full previews, ...)

protected:
	void drawInstruments(Dimensions const& dim, float alpha = 1.0f) const;
	void drawMultimedia();
	void update();

	Audio& m_audio;
	Songs& m_songs;
	Database& m_database;
	boost::scoped_ptr<Surface> m_songbg, m_songbg_ground, m_songbg_default;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<ThemeSongs> theme;
	Song::Music m_playing;
	AnimValue m_idleTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_singCover;
	boost::scoped_ptr<Surface> m_instrumentCover;
	boost::scoped_ptr<Surface> m_bandCover;
	boost::scoped_ptr<Surface> m_danceCover;
	boost::scoped_ptr<Texture> m_instrumentList;
	Cachemap<std::string, Surface> m_covers;
	bool m_jukebox;
	bool show_hiscores;
	int hiscore_start_pos;
};
