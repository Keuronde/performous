#ifdef USE_PORTMIDI

#include "controllers.hh"
#include "portmidi.hh"
#include "fs.hh"
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/regex.hpp>

namespace input {

	class Midi: public Hardware {
	public:
		Midi() {
			boost::regex re(config["game/midi_input"].s());
			for (int dev = 0; dev < Pm_CountDevices(); ++dev) {
				try {
					PmDeviceInfo const* info = Pm_GetDeviceInfo(dev);
					if (!info->input) continue;  // Not an input device
					if (info->opened) continue;  // Already opened
					std::string name = getName(dev);
					if (!re.empty() && !regex_search(name, re)) continue;
					// Now actually open the device
					m_streams.insert(dev, std::auto_ptr<pm::Input>(new pm::Input(dev)));
					std::clog << "controller-midi/info: Opened MIDI device " << name << std::endl;
				} catch (std::runtime_error& e) {
					std::clog << "controller-midi/warn: " << e.what() << std::endl;
				}
			}
		}
		std::string getName(unsigned dev) const {
			PmDeviceInfo const* info = Pm_GetDeviceInfo(dev);
			if (!info) throw std::logic_error("Invalid MIDI device requested in Midi::getName");
			std::ostringstream name;
			name << dev << ": " << info->name;
			return name.str();
		}
		bool process(Event& event) {
			PmEvent ev;
			for (auto kv: m_streams) {
				if (Pm_Read(*kv.second, &ev, 1) != 1) continue;
				unsigned char evnt = ev.message & 0xF0;
				unsigned char note = ev.message >> 8;
				unsigned char vel  = ev.message >> 16;
				unsigned chan = (ev.message & 0x0F) + 1;  // It is conventional to use one-based indexing
				if (evnt == 0x80 /* NOTE OFF */) { evnt = 0x90; vel = 0; }  // Translate NOTE OFF into NOTE ON with zero-velocity
				if (evnt != 0x90 /* NOTE ON */) continue;  // Ignore anything that isn't NOTE ON/OFF
				std::clog << "controller-midi/info: MIDI NOTE ON/OFF event: ch=" << unsigned(chan) << " note=" << unsigned(note) << " vel=" << unsigned(vel) << std::endl;
				event.source = SourceId(SOURCETYPE_MIDI, kv.first, chan);
				event.hw = note;
				event.value = vel / 127.0;
				return true;
			}
			return false;
		}
	private:
		pm::Initialize m_init;
		boost::ptr_map<unsigned, pm::Input> m_streams;
	};

	Hardware::ptr constructMidi() { return Hardware::ptr(new Midi()); }
	bool Hardware::midiEnabled() { return true; }
}

#else

#include "controllers.hh"

namespace input {
	Hardware::ptr constructMidi() { return Hardware::ptr(); }
	bool Hardware::midiEnabled() { return false; }
}

#endif

