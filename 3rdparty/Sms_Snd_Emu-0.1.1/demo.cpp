
// Use Sms_Apu to play random tones. Write output to sound file "out.aif".

#include "Sms_Apu.h"
#include "Stereo_Buffer.h"
#include "Sound_Writer.hpp"

#include <stdlib.h>

int main()
{
	long samples_per_sec = 44100;   // 44.1 kHz sample rate
	long clocks_per_sec = 3579545;  // 3.58 MHz clock rate
	
	Sms_Apu apu;

	// Set up buffer
	Stereo_Buffer buf;
	if ( !buf.sample_rate( samples_per_sec ) )
		return EXIT_FAILURE; // out of memory
	buf.clock_rate( clocks_per_sec );
	
	// Route APU output to buffer
	apu.output( buf.center(), buf.left(), buf.right() );
	
	// Synthesize sound and write to sound file
	Sound_Writer aiff( samples_per_sec );
	aiff.stereo( true );
	blip_time_t time = 0;
	int volume = 1;
	for ( int n = 30; n--; )
	{
		// Generate sound in 1/10 second chunks
		blip_time_t end_time = clocks_per_sec / 10;
		
		// Make random tones
		while ( time < end_time )
		{
			volume--;
			apu.write_data( time, 0x9f - volume );
			if ( volume == 0 )
			{
				volume = 16;
				int chan = rand() & 0x11;
				apu.write_ggstereo( time, chan ? chan : 0x11 );
				int freq = (rand() & 0x3ff) | 0x100;
				apu.write_data( time, 0x80 + (freq & 0x0f) );
				apu.write_data( time, freq >> 4 );
			}
			
			time += clocks_per_sec / 60; // 1/60 second between volume changes
		}
		
		// End time frame. The APU's frame *must* be ended first.
		time -= end_time;
		apu.end_frame( end_time );
		buf.end_frame( end_time );
		
		// Samples from the frame can now be read out of the buffer, or they
		// can be allowed to accumulate and read out later. Here they are
		// allowed to accumulate until 1/2 second of samples are available
		// before any are read out.
		if ( buf.samples_avail() >= samples_per_sec / 2 )
		{
			// Buffer to read samples into
			size_t const out_size = 1024;
			blip_sample_t out_buf [out_size];
			
			// Keep reading samples into buffer and writing to sound file until
			// no more are available.
			size_t samples_read;
			while ( (samples_read = buf.read_samples( out_buf, out_size )) != 0 )
				aiff.write( out_buf, samples_read );
		}
	}
	
	return 0;
}

