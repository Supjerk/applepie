/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-2015  The Bochs Project
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

// Common code for sound lowlevel modules

#ifndef WIN32
#include <pthread.h>
#endif

// This is the maximum size of a wave data packet.
// It should be large enough for 0.1 seconds of playback or recording.
#define BX_SOUNDLOW_WAVEPACKETSIZE  19200

// Definitions for the output functions
#define BX_SOUNDLOW_OK   0
#define BX_SOUNDLOW_ERR  1

#define BX_MAX_WAVE_CALLBACKS 3

typedef struct {
  Bit16u samplerate;
  Bit8u  bits;
  Bit8u  channels;
  Bit8u  format;
  Bit16u volume;
} bx_pcm_param_t;

const bx_pcm_param_t default_pcm_param = {44100, 16, 2, 1};

typedef Bit32u (*sound_record_handler_t)(void *arg, Bit32u len);
typedef Bit32u (*get_wave_cb_t)(void *arg, Bit16u rate, Bit8u *buffer, Bit32u len);

// audio buffer support

typedef struct _audio_buffer_t
{
  Bit32u size, pos;
  Bit8u *data;
  bx_pcm_param_t param;
  struct _audio_buffer_t *next;
} audio_buffer_t;

class bx_audio_buffer_c {
public:
  bx_audio_buffer_c(void);
  ~bx_audio_buffer_c();

  audio_buffer_t *new_buffer(Bit32u size);
  audio_buffer_t *get_buffer();
  void delete_buffer();
private:
  audio_buffer_t *root;
};

extern bx_audio_buffer_c *audio_buffers[2];
Bit32u pcm_callback(void *dev, Bit16u rate, Bit8u *buffer, Bit32u len);

extern int conversion_control;
extern int mixer_control;
extern BX_MUTEX(conversion_mutex);
extern BX_MUTEX(mixer_mutex);

// the waveout class

class bx_soundlow_waveout_c : public logfunctions {
public:
  bx_soundlow_waveout_c();
  virtual ~bx_soundlow_waveout_c();

  virtual int openwaveoutput(const char *wavedev);
  virtual int set_pcm_params(bx_pcm_param_t *param);
  virtual int sendwavepacket(int length, Bit8u data[], bx_pcm_param_t *src_param);
  virtual int convert_pcm_data(void);
  virtual int get_packetsize();
  virtual int output(int length, Bit8u data[]);
  virtual int closewaveoutput();

  virtual int register_wave_callback(void *, get_wave_cb_t wd_cb);
  virtual void unregister_wave_callback(int callback_id);

  virtual bx_bool mixer_common(Bit8u *buffer, int len);
protected:
  void start_conversion_thread(void);
  void start_mixer_thread(void);
  void convert_common(Bit8u *src, int srcsize, Bit8u *dst, int dstsize, bx_pcm_param_t *param);

  bx_pcm_param_t emu_pcm_param, real_pcm_param;
  int cvt_mult;

  int cb_count;
  struct {
    void *device;
    get_wave_cb_t cb;
  } get_wave[BX_MAX_WAVE_CALLBACKS];
  int pcm_callback_id;
};

// the wavein class

class bx_soundlow_wavein_c : public logfunctions {
public:
  bx_soundlow_wavein_c();
  virtual ~bx_soundlow_wavein_c();

  virtual int openwaveinput(const char *wavedev, sound_record_handler_t rh);
  virtual int startwaverecord(bx_pcm_param_t *param);
  virtual int getwavepacket(int length, Bit8u data[]);
  virtual int stopwaverecord();

  static void record_timer_handler(void *);
  void record_timer(void);
protected:
  int record_timer_index;
  int record_packet_size;
  sound_record_handler_t record_handler;
};

// the midiout class

class bx_soundlow_midiout_c : public logfunctions {
public:
  bx_soundlow_midiout_c();
  virtual ~bx_soundlow_midiout_c();

  virtual int openmidioutput(const char *mididev);
  virtual int midiready();
  virtual int sendmidicommand(int delta, int command, int length, Bit8u data[]);
  virtual int closemidioutput();
};

// This is the base class of the sound lowlevel support.
// the lowlevel sound driver class returns pointers to the child objects

class bx_sound_lowlevel_c : public logfunctions {
public:
  bx_sound_lowlevel_c();
  virtual ~bx_sound_lowlevel_c();

  virtual bx_soundlow_waveout_c* get_waveout() {return NULL;}
  virtual bx_soundlow_wavein_c* get_wavein() {return NULL;}
  virtual bx_soundlow_midiout_c* get_midiout() {return NULL;}

protected:
  bx_soundlow_waveout_c *waveout;
  bx_soundlow_wavein_c *wavein;
  bx_soundlow_midiout_c *midiout;
};

// the dummy sound driver contains stubs for all features

class bx_sound_dummy_c : public bx_sound_lowlevel_c {
public:
  bx_sound_dummy_c() {}
  virtual ~bx_sound_dummy_c() {}

  virtual bx_soundlow_waveout_c* get_waveout();
  virtual bx_soundlow_wavein_c* get_wavein();
  virtual bx_soundlow_midiout_c* get_midiout();
};
