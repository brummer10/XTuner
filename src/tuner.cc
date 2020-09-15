/*
 * Copyright (C) 2009, 2010 Hermann Meyer, James Warden, Andreas Degert
 * Copyright (C) 2011 Pete Shorthose
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 *
 *
 *    This is part of the Guitarix Audio Engine
 *
 *
 *
 * --------------------------------------------------------------------------
 */

#include "gx_pitch_tracker.h"

/****************************************************************
 ** class tuner
 */

class tuner {
private:
    PitchTracker pitch_tracker;
    int state;
    enum { tuner_use = 0x01, livetuner_use = 0x02, switcher_use = 0x04, midi_use = 0x08 };
    void set_and_check(int use, bool on);
public:
    sigc::signal<void >& signal_freq_changed() { return pitch_tracker.new_freq; }
   // Glib::Dispatcher& signal_freq_changed() { return pitch_tracker.new_freq; }
    static void feed_tuner(int count, float *input, float *output, tuner&);
    static int activate(bool start, tuner& self);
    static void init(unsigned int samplingFreq, tuner& self);
    static void del_instance(tuner& self);
    static float get_freq(tuner& self) { return self.pitch_tracker.get_estimated_freq(); }
    static float get_note(tuner& self) { return self.pitch_tracker.get_estimated_note(); }
    static inline float db2power(float db) {return pow(10.,db*0.05);}
    static void set_threshold_level(tuner& self,float v) {self.pitch_tracker.set_threshold(db2power(v)); }
    static void set_fast_note(tuner& self,bool v) {self.pitch_tracker.set_fast_note_detection(v); }
    tuner();
    ~tuner() {};
};

tuner::tuner()
    : // trackable(),
      pitch_tracker(),
      state() {}

void tuner::init(unsigned int samplingFreq, tuner& self) {
    int priority = 0, policy = 0;
#ifdef _POSIX_PRIORITY_SCHEDULING
    int priomax = sched_get_priority_max(SCHED_FIFO);
    if ((priomax/2.2) > 0) {
        priority = priomax/2.2;
        policy = SCHED_FIFO;
    }
#endif
    self.pitch_tracker.init(policy, priority, samplingFreq);
}

void tuner::set_and_check(int use, bool on) {
    if (on) {
        state |= use;
    } else {
        state &= ~use;
    }
    if (use == switcher_use) {
        pitch_tracker.set_fast_note_detection(on);
    }
}

int tuner::activate(bool start, tuner& self) {
    if (!start) {
        self.pitch_tracker.reset();
    }
    return 0;
}

void tuner::feed_tuner(int count, float* input, float*, tuner& self) {
    self.pitch_tracker.add(count, input);
}

void tuner::del_instance(tuner& self)
{
    delete &self;
}
