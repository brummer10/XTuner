/*
 * Copyright (C) 2020, 2010 Hermann Meyer
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
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <thread>
#include <system_error>
#include <fstream>
#include <iostream>
#include <sstream>

#include <jack/jack.h>


#include "NsmHandler.h"
#include "gx_pitch_tracker.h"
#include "gx_pitch_tracker.cpp"
#include "low_high_cut.cc"
#include "tuner.cc"

#include "xwidgets.h"

//   g++ -O2 -Wall -fstack-protector -funroll-loops -ffast-math -fomit-frame-pointer -fstrength-reduce xjack.c  -L. ../libxputty/libxputty/libxputty.a -o xjack -I../libxputty/libxputty/include/ `pkg-config --cflags --libs jack` `pkg-config --cflags --libs cairo x11 sigc++-2.0 fftw3f` -lm -lzita-resampler -lpthread

/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incomming system signals in a extra thread
 ** 
 */

class PosixSignalHandler : public sigc::trackable {
private:
    sigset_t waitset;
    std::thread *thread;
    volatile bool exit;
    void signal_helper_thread();
    void create_thread();
    
public:
    PosixSignalHandler();
    ~PosixSignalHandler();

    sigc::signal<void, int> trigger_quit_by_posix;
    sigc::signal<void, int>& signal_trigger_quit_by_posix() { return trigger_quit_by_posix; }

    sigc::signal<void, int> trigger_kill_by_posix;
    sigc::signal<void, int>& signal_trigger_kill_by_posix() { return trigger_kill_by_posix; }
};

PosixSignalHandler::PosixSignalHandler()
    : sigc::trackable(),
      waitset(),
      thread(nullptr),
      exit(false) {
    sigemptyset(&waitset);

    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGQUIT);
    sigaddset(&waitset, SIGTERM);
    sigaddset(&waitset, SIGHUP);
    sigaddset(&waitset, SIGKILL);

    sigprocmask(SIG_BLOCK, &waitset, NULL);
    create_thread();
}

PosixSignalHandler::~PosixSignalHandler() {
    if (thread) {
        exit = true;
        pthread_kill(thread->native_handle(), SIGINT);
        thread->join();
        delete thread;
    }
    sigprocmask(SIG_UNBLOCK, &waitset, NULL);
}

void PosixSignalHandler::create_thread() {
    try {
        thread = new std::thread(
            sigc::mem_fun(*this, &PosixSignalHandler::signal_helper_thread));
    } catch (std::system_error& e) {
        fprintf(stderr,"Thread create failed (signal): %s", e.what());
    }
}

void PosixSignalHandler::signal_helper_thread() {

    pthread_sigmask(SIG_BLOCK, &waitset, NULL);
    while (true) {
        int sig;
        int ret = sigwait(&waitset, &sig);
        if (exit) {
            break;
        }
        if (ret != 0) {
            assert(errno == EINTR);
            continue;
        }
        switch (sig) {
            case SIGINT:
            case SIGTERM:
            case SIGQUIT:
                trigger_quit_by_posix(sig);
            break;
            case SIGHUP:
            case SIGKILL:
                trigger_kill_by_posix(sig);
            break;
            default:
            break;
        }
    }
}

/****************************************************************
 ** class XJack
 **
 ** 
 ** 
 */

class XJack {
private:
    PosixSignalHandler xsig;
    nsmhandler::NsmSignalHandler& nsmsig;
    int main_x;
    int main_y;
    int main_h;
    int main_w;
    int visible;

    void set_config(const char *name, const char *client_id, bool op_gui);
    void nsm_show_ui();
    void nsm_hide_ui();
    void show_ui(int present);

    static void jack_shutdown (void *arg);
    static int jack_xrun_callback(void *arg);
    static int jack_srate_callback(jack_nframes_t samplerate, void* arg);
    static int jack_buffersize_callback(jack_nframes_t nframes, void* arg);
    static int jack_process(jack_nframes_t nframes, void *arg);
    static void draw_window(void *w_, void* user_data);
    static void ref_freq_changed(void *w_, void* user_data);
    static void temperament_changed(void *w_, void* user_data);
    static void win_configure_callback(void *w_, void* user_data);
    Widget_t* add_my_combobox(Widget_t *w, const char * label, const char** items,
                        size_t len, int active, int x, int y, int width, int height);
public:
    XJack(PosixSignalHandler& _xsig, nsmhandler::NsmSignalHandler& _nsmsig);
    ~XJack();

    Xputty app;
    Widget_t *w;
    Widget_t *wid[3];
    std::string client_name;
    std::string config_file;
    std::string path;

    jack_client_t *client;
    jack_port_t *in_port;
    jack_port_t *out_port;

    tuner *xtuner;
    low_high_cut::Dsp *lhc;

    void signal_handle (int sig);
    void exit_handle (int sig);
    void freq_changed_handler();
    void read_config();
    void save_config();
    void init_jack();
    void init_gui();
};

XJack::XJack(PosixSignalHandler& _xsig, nsmhandler::NsmSignalHandler& _nsmsig)
    : xsig(_xsig),
     nsmsig(_nsmsig),
     xtuner(NULL),
     lhc(NULL) {
    client_name = "XTuner";
    main_x = 0;
    main_y = 0;
    main_w = 520;
    main_h = 200;
    if (getenv("XDG_CONFIG_HOME")) {
        path = getenv("XDG_CONFIG_HOME");
        config_file = path +"/XTuner.conf";
    } else {
        path = getenv("HOME");
        config_file = path +"/.config/XTuner.conf";
    }
    
    if (!xtuner)
        xtuner = new tuner();
    if (!lhc)
        lhc = new low_high_cut::Dsp();

    xsig.signal_trigger_quit_by_posix().connect(
        sigc::mem_fun(this, &XJack::signal_handle));

    xsig.signal_trigger_kill_by_posix().connect(
        sigc::mem_fun(this, &XJack::exit_handle));

    nsmsig.signal_trigger_nsm_show_gui().connect(
        sigc::mem_fun(this, &XJack::nsm_show_ui));

    nsmsig.signal_trigger_nsm_hide_gui().connect(
        sigc::mem_fun(this, &XJack::nsm_hide_ui));

    nsmsig.signal_trigger_nsm_save_gui().connect(
        sigc::mem_fun(this, &XJack::save_config));

    nsmsig.signal_trigger_nsm_gui_open().connect(
        sigc::mem_fun(this, &XJack::set_config));

}

XJack::~XJack() {
    if (xtuner) {
        xtuner->activate(false, (*xtuner));
        delete xtuner;
    }
    if (lhc)
        delete lhc;
}

/****************************************************************
 ** 
 **    jack stuff
 */

void XJack::jack_shutdown (void *arg) {
    XJack *xjack = (XJack*)arg;
    quit(xjack->w);
    exit (1);
}

int XJack::jack_xrun_callback(void *arg) {
    fprintf (stderr, "Xrun \r");
    return 0;
}

void XJack::freq_changed_handler() { 
    //fprintf (stderr, " %f\n",xtuner->get_freq((*xtuner)));
    XLockDisplay(w->app->dpy);
    adj_set_value(wid[0]->adj, (float)xtuner->get_freq((*xtuner)));
    expose_widget(wid[0]);
    XFlush(w->app->dpy);
    XUnlockDisplay(w->app->dpy);
}

int XJack::jack_srate_callback(jack_nframes_t samplerate, void* arg) {
    fprintf (stderr, "Samplerate %iHz \n", samplerate);
    return 0;
}

int XJack::jack_buffersize_callback(jack_nframes_t nframes, void* arg) {
    fprintf (stderr, "Buffersize is %i samples \n", nframes);
    return 0;
}

int XJack::jack_process(jack_nframes_t nframes, void *arg) {
    XJack *xjack = (XJack*)arg;
    float *in = static_cast<float *>(jack_port_get_buffer (xjack->in_port, nframes));
    float *out = static_cast<float *>(jack_port_get_buffer (xjack->out_port, nframes));
    memcpy (out, in, sizeof (float) * nframes);
    float buf[nframes];
    memcpy(buf, in, nframes * sizeof(float));
    xjack->lhc->compute_static(static_cast<int>(nframes), buf, buf, xjack->lhc);
    xjack->xtuner->feed_tuner (static_cast<int>(nframes), buf, buf, (*xjack->xtuner));

    return 0;
}

void XJack::init_jack() {

    if ((client = jack_client_open (client_name.c_str(), JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        quit(w);
        exit (1);
    }

    in_port = jack_port_register(
                    client, "in_0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    out_port = jack_port_register(
                    client, "out_0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    jack_set_xrun_callback(client, jack_xrun_callback, this);
    jack_set_sample_rate_callback(client, jack_srate_callback, this);
    jack_set_buffer_size_callback(client, jack_buffersize_callback, this);
    jack_set_process_callback(client, jack_process, this);
    jack_on_shutdown (client, jack_shutdown, this);

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        quit(w);
    }

    if (!jack_is_realtime(client)) {
        fprintf (stderr, "jack isn't running with realtime priority\n");
    } else {
        fprintf (stderr, "jack running with realtime priority\n");
    }

    jack_nframes_t samplerate =jack_get_sample_rate(client);
    lhc->init_static(samplerate, lhc);
    xtuner->init(samplerate, (*xtuner));
    xtuner->signal_freq_changed().connect(sigc::mem_fun(this, &XJack::freq_changed_handler));
    
}

/****************************************************************
 ** 
 **    gui stuff
 */

// draw the window
void XJack::draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);
    set_pattern(w,&w->app->color_scheme->normal,&w->app->color_scheme->selected,BACKGROUND_);
    cairo_rectangle (w->crb,4,4,w->width-8,w->height-8);
    cairo_set_line_width(w->crb,4);
    cairo_stroke(w->crb);

    cairo_new_path (w->crb);

    cairo_text_extents_t extents;
    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb, w->app->big_font/w->scale.ascale);
    cairo_text_extents(w->crb,w->label , &extents);
    double tw = extents.width/2.0;

    widget_set_scale(w);
    cairo_move_to (w->crb, (w->scale.init_width*0.5)-tw, w->scale.init_height-10 );
    cairo_show_text(w->crb, w->label);
    cairo_new_path (w->crb);


    cairo_pattern_t* pat;

    pat = cairo_pattern_create_linear (-1.0, 53, 0, w->scale.init_height-106.0);
    cairo_pattern_add_color_stop_rgba (pat, 0,  0.25, 0.25, 0.25, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.2, 0.2, 0.2, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.1, 0.1, 0.1, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 1,  0.05, 0.05, 0.05, 1.0);

    cairo_set_source (w->crb, pat);
    cairo_rectangle(w->crb, 53.0, 53.0,w->scale.init_width*0.8,w->scale.init_height-106.0);
    cairo_set_line_width(w->crb,2);
    cairo_stroke(w->crb);
    cairo_pattern_destroy (pat);
    pat = NULL;
    pat = cairo_pattern_create_linear (-1.0, 58, 0, w->scale.init_height-116.0);
    cairo_pattern_add_color_stop_rgba (pat, 1,  0.25, 0.25, 0.25, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.2, 0.2, 0.2, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.1, 0.1, 0.1, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0,  0.05, 0.05, 0.05, 1.0);

    cairo_set_source (w->crb, pat);
    cairo_rectangle(w->crb, 58.0, 58.0,w->scale.init_width*0.78,w->scale.init_height-116.0);
    cairo_stroke(w->crb);
    cairo_pattern_destroy (pat);
    pat = NULL;

    widget_reset_scale(w);
}

void XJack::set_config(const char *name, const char *client_id, bool op_gui) {
    client_name = client_id;
    path = name;
    config_file = path + ".config";
    if (op_gui) {
        visible = 0;
    } else {
        visible = 1;
    }
}

void XJack::read_config() {
    std::ifstream infile(config_file);
    std::string line;
    std::string key;
    std::string value;
    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            std::istringstream buf(line);
            buf >> key;
            buf >> value;
            if (key.compare("[main_x]") == 0) main_x = std::stoi(value);
            else if (key.compare("[main_y]") == 0) main_y = std::stoi(value);
            else if (key.compare("[main_w]") == 0) main_w = std::stoi(value);
            else if (key.compare("[main_h]") == 0) main_h = std::stoi(value);
            else if (key.compare("[visible]") == 0) visible = std::stoi(value);
            key.clear();
            value.clear();
        }
        infile.close();
    }   
}

void XJack::save_config() {
    if(nsmsig.nsm_session_control)
        XLockDisplay(w->app->dpy);

    std::ofstream outfile(config_file);
    if (outfile.is_open()) {
         outfile << "[main_x] "<< main_x << std::endl;
         outfile << "[main_y] " << main_y << std::endl;
         outfile << "[main_w] " << main_w << std::endl;
         outfile << "[main_h] " << main_h << std::endl;
         outfile << "[visible] " << visible << std::endl;
         outfile.close();
    }

    if(nsmsig.nsm_session_control)
        XUnlockDisplay(w->app->dpy);
}

void XJack::nsm_show_ui() {
    XLockDisplay(w->app->dpy);
    widget_show_all(w);
    XFlush(w->app->dpy);
    XMoveWindow(w->app->dpy,w->widget, main_x, main_y);
    nsmsig.trigger_nsm_gui_is_shown();
    XUnlockDisplay(w->app->dpy);
}

void XJack::nsm_hide_ui() {
    XLockDisplay(w->app->dpy);
    widget_hide(w);
    XFlush(w->app->dpy);
    nsmsig.trigger_nsm_gui_is_hidden();
    XUnlockDisplay(w->app->dpy);
}

void XJack::show_ui(int present) {
    if(present) {
        widget_show_all(w);
        XMoveWindow(w->app->dpy,w->widget, main_x, main_y);
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_shown();
    }else {
        widget_hide(w);
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_hidden();
    }
}

// static
void XJack::win_configure_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XJack *xjack = (XJack*) w->parent_struct;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int width = attrs.width;
    int height = attrs.height;

    Window parent;
    Window root;
    Window * children;
    unsigned int num_children;
    XQueryTree(w->app->dpy, w->widget,
        &root, &parent, &children, &num_children);
    if (children) XFree(children);
    if (w->widget == root || parent == root) parent = w->widget;
    XGetWindowAttributes(w->app->dpy, parent, &attrs);

    int x1, y1;
    Window child;
    XTranslateCoordinates( w->app->dpy, parent, DefaultRootWindow(
                    w->app->dpy), 0, 0, &x1, &y1, &child );

    xjack->main_x = x1 - min(1,(width - attrs.width))/2;
    xjack->main_y = y1 - min(1,(height - attrs.height))/2;
    xjack->main_w = width;
    xjack->main_h = height;
}

void XJack::ref_freq_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XJack *xjack = (XJack*)w->parent_struct;
    tuner_set_ref_freq(xjack->wid[0],adj_get_value(w->adj));
}

void XJack::temperament_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XJack *xjack = (XJack*)w->parent_struct;
    tuner_set_temperament(xjack->wid[0],adj_get_value(w->adj));
}

// shortcut to create comboboxes
Widget_t* XJack::add_my_combobox(Widget_t *w, const char * label, const char** items,
                                size_t len, int active, int x, int y, int width, int height) {
    w = add_combobox(w, label, x, y, width, height);
    size_t st = 0;
    for(; st < len; st++) {
        combobox_add_entry(w,items[st]);
    }
    combobox_set_active_entry(w, active);
    return w;
}
// disable adj_callback from tuner to redraw it from freq change handler
void dummy_callback(void *w_, void* user_data) {

}

void XJack::init_gui() {
    app.color_scheme->normal.text[0] = 0.68;
    app.color_scheme->normal.text[1] = 0.44;
    app.color_scheme->normal.text[2] = 0.00;
    app.color_scheme->normal.text[3] = 1.00;
    w = create_window(&app, DefaultRootWindow(app.dpy), 0, 0, 520, 200);
    widget_set_title(w, client_name.c_str());
    w->label = "XTUNER";
    w->parent_struct = this;
    w->func.configure_notify_callback = win_configure_callback;
    w->func.expose_callback = draw_window;

    wid[0] = add_tuner(w, "Freq", 60, 60, 400, 80);
    wid[0]->func.adj_callback = dummy_callback;

    const char* model[] = {"12-TET","19-TET","24-TET", "31-TET", "53-TET"};
    size_t len = sizeof(model) / sizeof(model[0]);
    wid[1] = add_my_combobox(w, "Mode", model, len, 0, 130, 20, 90, 25);
    wid[1]->func.value_changed_callback = temperament_changed;
    wid[1]->parent_struct = this;
    tuner_set_temperament(wid[0],adj_get_value(wid[1]->adj));

    wid[2] = add_valuedisplay(w, "RefFreq", 60, 20, 50, 25);
    set_adjustment(wid[2]->adj,440.0, 440.0, 427.0, 453.0, 0.1, CL_CONTINUOS);
    wid[2]->func.value_changed_callback = ref_freq_changed;
    wid[2]->parent_struct = this;
    tuner_set_ref_freq(wid[0],adj_get_value(wid[2]->adj));
    if (!nsmsig.nsm_session_control) show_ui(1);
    
}

/****************************************************************
 ** 
 **    posix signal handle
 */

void XJack::signal_handle (int sig) {
    if(client) jack_client_close (client);
    client = NULL;
    XLockDisplay(w->app->dpy);
    quit(w);
    XFlush(w->app->dpy);
    XUnlockDisplay(w->app->dpy);
    fprintf (stderr, "\n%s: signal %i received, bye bye ...\n",client_name.c_str(), sig);
}

void XJack::exit_handle (int sig) {
    if(client) jack_client_close (client);
    client = NULL;
    fprintf (stderr, "\n%s: signal %i received, exiting ...\n",client_name.c_str(), sig);
    exit (0);
}

/****************************************************************
 ** 
 **    main
 */

int main (int argc, char *argv[]) {

    if(0 == XInitThreads()) 
        fprintf(stderr, "Warning: XInitThreads() failed\n");
    
    PosixSignalHandler xsig;
    nsmhandler::NsmSignalHandler nsmsig;
    XJack xjack (xsig, nsmsig);
    nsmhandler::NsmHandler nsmh(&nsmsig);

    nsmsig.nsm_session_control = nsmh.check_nsm(xjack.client_name.c_str(), argv);

    xjack.read_config();

    main_init(&xjack.app);

    xjack.init_gui();

    xjack.init_jack();

    main_run(&xjack.app);
   
    if(!nsmsig.nsm_session_control) xjack.save_config();

    main_quit(&xjack.app);

    if(xjack.client) jack_client_close (xjack.client);

    exit (0);
}
