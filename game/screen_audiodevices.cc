#include "screen_audiodevices.hh"

#include "configuration.hh"
#include "controllers.hh"
#include "theme.hh"
#include "audio.hh"
#include "i18n.hh"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace {
	const float yoff = 0.18; // Offset from center where to place top row
	const float xoff = 0.45; // Offset from middle where to place first column

	bool countRow(std::string needle, std::string const& haystack, int& count) {
		if (haystack.find(needle) != std::string::npos) ++count;
		if (count > 1) return false;
		return true;
	}
}

ScreenAudioDevices::ScreenAudioDevices(std::string const& name, Audio& audio): Screen(name), m_audio(audio) {
	m_selector.reset(new Surface(getThemePath("device_selector.svg")));
	m_mic_icon.reset(new Surface(getThemePath("sing_pbox.svg")));
	m_pdev_icon.reset(new Surface(getThemePath("icon_pdev.svg")));
	m_colorMap["blue"] = Color(0.2, 0.5, 0.7);
	m_colorMap["red"] = Color(0.8, 0.3, 0.3);
	m_colorMap["green"] = Color(0.2, 0.9, 0.2);
	m_colorMap["orange"] = Color(1.0, 0.6, 0.0);
	m_colorMap["OUT"] = Color(0.5, 0.5, 0.5);
}

void ScreenAudioDevices::enter() {
	m_theme.reset(new ThemeAudioDevices());
	portaudio::AudioDevices ads;
	m_devs = ads.devices;
	// FIXME: Something more elegant, like a warning box
	if (m_devs.empty()) throw std::runtime_error("No audio devices found!");
	m_selected_column = 0;
	// Detect if there is existing advanced configuration and warn that this will override
	if (!config["audio/devices"].isDefault()) {
		ConfigItem::StringList devconf = config["audio/devices"].sl();
		std::map<std::string, int> countmap;
		bool ok = true;
		for (ConfigItem::StringList::const_iterator it = devconf.begin(); it != devconf.end(); ++it) {
			if (!countRow("blue", *it, countmap["blue"])) { ok = false; break; }
			if (!countRow("red", *it, countmap["red"])) { ok = false; break; }
			if (!countRow("green", *it, countmap["green"])) { ok = false; break; }
			if (!countRow("orange", *it, countmap["orange"])) { ok = false; break; }
			if (!countRow("out=", *it, countmap["out="])) { ok = false; break; }
		}
		if (!ok)
			ScreenManager::getSingletonPtr()->dialog(
				_("It seems you have some manual configurations\nincompatible with this user interface.\nSaving these settings will override\nall existing audio device configuration.\nYour other options changes will be saved too."));
	}
	// Populate the mics vector and check open devices
	load();
	// TODO: Scrolling would be nicer than just zooming out infinitely
	float s = std::min(xoff / m_mics.size() / 1.2, yoff*2 / m_devs.size() / 1.1);
	m_mic_icon->dimensions.fixedWidth(s);
	m_pdev_icon->dimensions.fixedWidth(s);
}

void ScreenAudioDevices::exit() { m_theme.reset(); }

void ScreenAudioDevices::manageEvent(input::NavEvent const& event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav = event.button;
	if (nav == input::NAV_CANCEL) sm->activateScreen("Intro");
	else if (nav == input::NAV_PAUSE) m_audio.togglePause();
	else if (m_devs.empty()) return; // The rest work if there are any devices
	else if (nav == input::NAV_START) { if (save()) sm->activateScreen("Intro"); }
	else if (nav == input::NAV_LEFT && m_selected_column > 0) --m_selected_column;
	else if (nav == input::NAV_RIGHT && m_selected_column < m_mics.size()-1) ++m_selected_column;
	else if (nav == input::NAV_UP)
		m_mics[m_selected_column].dev = (m_mics[m_selected_column].dev + m_devs.size()) % (m_devs.size() + 1);
	else if (nav == input::NAV_DOWN)
		m_mics[m_selected_column].dev = (m_mics[m_selected_column].dev + 1) % (m_devs.size() + 1);
}

void ScreenAudioDevices::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
		if (m_devs.empty()) return; // The rest work if there are any config options
		// Reset to defaults
		else if (key == SDLK_r && modifier & KMOD_CTRL) {
			config["audio/devices"].reset(modifier & KMOD_ALT);
			save(true); // Save to disk, reload audio & reload UI to keep stuff consistent
		}
	}
}

void ScreenAudioDevices::draw() {
	m_theme->bg.draw();
	if (!m_devs.empty()) {
		// Calculate spacing between columns/rows
		const float xstep = (xoff - 0.5 + xoff) / m_mics.size();
		const float ystep = yoff*2 / m_devs.size();
		// Device text & bg
		m_theme->device_bg.dimensions.stretch(std::abs(xoff*2), m_mic_icon->dimensions.h()*0.9).middle();
		m_selector->dimensions.stretch(m_mic_icon->dimensions.w() * 1.75, m_mic_icon->dimensions.h() * 1.75);
		for (size_t i = 0; i <= m_devs.size(); ++i) {
			const float y = -yoff + i*ystep;
			float alpha = 1.0f;
			// "Grey out" devices that doesn't fit the selection
			if (m_mics[m_selected_column].name == "OUT" && !m_devs[i].out) alpha = 0.5f;
			else if (m_mics[m_selected_column].name != "OUT" && !m_devs[i].in) alpha = 0.5f;
			m_theme->device_bg.dimensions.center(y);
			m_theme->device_bg.draw();
			ColorTrans c(Color::alpha(alpha));
			m_theme->device.dimensions.middle(-xstep*0.5).center(y);
			m_theme->device.draw(i < m_devs.size() ? m_devs[i].desc() : _("- Unassigned -"));
		}
		// Icons
		for (size_t i = 0; i < m_mics.size(); ++i) {
			Surface& srf = (i < m_mics.size()-1) ? *m_mic_icon : *m_pdev_icon;
			{
				ColorTrans c(m_colorMap[m_mics[i].name]);
				srf.dimensions.middle(-xoff + xstep*0.5 + i*xstep).center(-yoff+m_mics[i].dev*ystep);
				srf.draw();
			}
			// Selection indicator
			if (m_selected_column == i)
				m_selector->dimensions.middle(srf.dimensions.xc()).center(srf.dimensions.yc());
		}
		m_selector->draw(); // Position already set in the loop
	}
	// Key help
	m_theme->comment_bg.dimensions.stretch(1.0, 0.025).middle().screenBottom(-0.054);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.067);
	m_theme->comment.draw(_("Use arrow keys to configure. Hit Enter/Start to save and test or Esc/Select to cancel. Ctrl + R to reset defaults"));
	// Additional info
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.023);
	m_theme->comment.draw(_("For advanced device configuration, use command line parameter --audio (use --audiohelp for details)."));
}

void ScreenAudioDevices::load() {
	m_mics.clear();
	// Add mics & playback device, default is unassigned
	m_mics.push_back(Mic("blue", m_devs.size()));
	m_mics.push_back(Mic("red", m_devs.size()));
	m_mics.push_back(Mic("green", m_devs.size()));
	m_mics.push_back(Mic("orange", m_devs.size()));
	m_mics.push_back(Mic("OUT", m_devs.size()));
	// Get the currently assigned device ids
	for (boost::ptr_vector<Device>::iterator it = m_audio.devices().begin();
	  it != m_audio.devices().end(); ++it) {
		for (size_t i = 0; i < m_mics.size()-1; ++i) {
			if (it->isMic(m_mics[i].name)) m_mics[i].dev = it->dev;
		}
		// Last "mic" is playback
		if (it->isOutput()) m_mics.back().dev = it->dev;
	}
}

bool ScreenAudioDevices::save(bool skip_ui_config) {
	if (!skip_ui_config) {
		ConfigItem::StringList devconf;
		// Loop through the devices and if there is mics/pdev assigned, form a config line
		for (size_t d = 0; d < m_devs.size(); ++d) {
			std::string mics = "", pdev = "";
			for (size_t m = 0; m < m_mics.size(); ++m) {
				if (m_mics[m].dev == unsigned(m_devs[d].idx)) {
					if (m_mics[m].name == "OUT") pdev = "out=2"; // Pdev, only stereo supported
					else { // Mic
						if (!mics.empty()) mics += ","; // Add separator if needed
						mics += m_mics[m].name; // Append mic color
					}
				}
			}
			if (mics.empty() && pdev.empty()) continue; // Continue looping if device is not used
			std::string dev = "dev=\"" + m_devs[d].name + "\""; // Use name instead of number for more robustness
			// Devices seem to crash less when opened if added separately
			if (!mics.empty()) devconf.push_back(dev + " mics=" + mics);
			if (!pdev.empty()) devconf.push_back(dev + " " + pdev);
		}
		config["audio/devices"].sl() = devconf;
	}
	writeConfig(); // Save the new config
	size_t unassigned_id = m_devs.size();
	// Give audio a little time to shutdown but then just quit
	boost::thread audiokiller(boost::bind(&Audio::close, boost::ref(m_audio)));
	if (!audiokiller.timed_join(boost::posix_time::milliseconds(2500)))
		ScreenManager::getSingletonPtr()->fatalError("Audio hung for some reason.\nPlease restart Performous.");
	m_audio.restart(); // Reload audio to take the new settings into use
	m_audio.playMusic(getThemePath("menu.ogg"), true); // Start music again
	// Check that all went well
	bool ret = verify(unassigned_id);
	if (!ret) ScreenManager::getSingletonPtr()->dialog(_("Some devices failed to open!"));
	// Load the new config back for UI
	load();
	return ret;
}

bool ScreenAudioDevices::verify(size_t unassigned_id) {
	for (size_t i = 0; i < m_mics.size(); ++i) {
		if (m_mics[i].dev != unassigned_id) { // Only check assigned mics
			// Find the device
			bool found = false;
			for (boost::ptr_vector<Device>::iterator it = m_audio.devices().begin();
			  it != m_audio.devices().end(); ++it) {
				if (i < m_mics.size()-1 && it->isMic(m_mics[i].name)) { found = true; break; } // Mic
				else if (i == m_mics.size()-1 && it->isOutput()) { found = true; break; } // Pdev
			}
			if (!found) return false;
		}
	}
	return true;
}
