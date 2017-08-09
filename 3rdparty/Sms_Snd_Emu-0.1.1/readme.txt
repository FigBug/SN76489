Sms_Snd_Emu 0.1.1: SEGA Master System Sound Emulation Library

This is a portable SEGA Master System SN76489 PSG sound chip emulator library
for use in a SMS emulator or a VGM music file player. Licensed under the GNU
General Public License (GPL); see COPYING.TXT. Copyright (C) 2003-2004 Shay
Green.

Website: http://www.slack.net/~ant/nes-emu/
Contact: blargg@mail.com


Getting Started
---------------

This library is written in somewhat conservative C++ that should compile with
current and older compilers (ISO and ARM).

If the Boost library is installed in your environment, delete the included
"boost" compatibility directory, otherwise add the included "boost" directory
to your compiler's search paths.

Build a program consisting of the included source files and any necessary
system libraries. It should generate an AIFF sound file "out.aif" of random
tones.

See notes.txt for more information, and respective header (.h) files for
reference.


Files
-----

notes.txt               Collection of notes about the library
changes.txt             Changes made since previous releases
COPYING.txt             GNU General Public License

demo.cpp                How to use the Sms_Apu sound chip emulator

Sms_Apu.h               Master System SN76489 PSG sound chip emulator
Stereo_Buffer.h         Stereo sound synthesis buffer
Blip_Buffer.h           Sound synthesis buffer

Sound_Writer.hpp        AIFF sound file writer used for demo output
Sound_Writer.cpp
blargg_common.h         Library implementation source code
Blip_Buffer.cpp
Blip_Synth.h
Sms_Apu.cpp
Sms_Oscs.h
Stereo_Buffer.cpp
boost/                  Substitute for boost library if it's unavailable

-- 
Shay Green <blargg@mail.com>
