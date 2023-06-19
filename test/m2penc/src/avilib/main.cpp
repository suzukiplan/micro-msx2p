
#include "avilib.h"
//or #include <avilib.h>

#ifdef _WIN32
	#include <windows.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ostringstream;
using std::ofstream;
using std::ostream;
using std::regex;
using std::smatch;
using std::regex_search;

struct aviDump {
	aviDump() = delete;
	ostream *out_;
	aviDump( avilib_streamtype_t type, ostream &out, void *p_info )
	: out_( &out )
	{
		if( type != avilib_Audio ) {
			out_ = nullptr;
			return;
		}

		avilib_WAVEFORMATEX *p_waveInfo = (avilib_WAVEFORMATEX *)p_info;

		uint32_t N32 = 0;
		uint16_t N16 = 0;

		out.write( "RIFF", 4 );
		out.write( (const char *)&N32, 4 );
		out.write( "WAVE", 4 );

		out.write( "fmt ", 4 );
		N32 = 16;
		out.write( (const char *)&N32, 4 );

		N16 = 1; // encoding tags PCM
		out.write( (const char *)&N16, 2 );

		N16 = p_waveInfo->nChannels;
		out.write( (const char *)&N16, 2 );
		N32 = p_waveInfo->nSamplesPerSec;
		out.write( (const char *)&N32, 4 );
		N32 = p_waveInfo->nAvgBytesPerSec;
		out.write( (const char *)&N32, 4 );
		N16 = p_waveInfo->nBlockAlign;
		out.write( (const char *)&N16, 2 );
		N16 = p_waveInfo->wBitsPerSample;
		out.write( (const char *)&N16, 2 );
		out.write( "data", 4 );
		N32 = 0;
		out.write( (const char *)&N32, 4 );
	}
	~aviDump()
	{
		uint32_t N32;
		if( !out_ ) return;

		out_->seekp( 0u, ostream::end );
		auto filesize = out_->tellp();
		out_->seekp( 4u, ostream::beg );
		N32 = (uint32_t)filesize - 8u;
		out_->write( (const char *)&N32, 4 );
		N32 = (uint32_t)filesize - 44u;
		out_->seekp( 40u, ostream::beg );
		out_->write( (const char *)&N32, 4 );
	}
};




int main( int argc, char *argv[] )
{
	int32_t w = 352, h = 288;
	uint32_t codec,alloc_size,fcount,streams;
	uint8_t *buffer;

	int32_t maj, min, rev;
	const char *string_ver;
	avilib_version( &maj, &min, &rev, &string_ver );
	cerr << string_ver << endl;

	if( argc < 3 ) {
		cerr << "Usage: aviapp <<input>> <<output>>" << endl;
		exit( 1 );
	}

	string input = argv[ 1 ],
	       output = argv[ 2 ],
	       audiosrc = argc > 3 ? argv[ 3 ] : "";
	bool read = !input.compare( input.size()-3, 3, "avi" );
	
	if( read ){
		
		double rateHz;
		uint32_t read_size;
		avireader_t *p_reader = avireader_create();
		if( !avireader_open(p_reader, input.c_str() ) ) {
			cerr << "Error opening file" << endl;
			return -1;
		}

		avilib_BITMAPINFO bm_info = {0};
		avilib_WAVEFORMATEX wav_info = {0};
		avilib_streamtype_t streamtype = avilib_UnknownStreamtype;
		avilib_streamtype_t desired_streamtype = avilib_Video;

		ofstream out( output.c_str(), ofstream::binary|ofstream::trunc );
		if( !out.good() ) return -1;

		uint32_t i_stream = ~0u;
		while(streamtype != desired_streamtype)
			avireader_get_stream_type( p_reader, ++i_stream, &streamtype );
		avireader_get_stream_count( p_reader, &streams );
		avireader_get_stream_type( p_reader, i_stream, &streamtype );
		if( streamtype == avilib_Video )
			avireader_get_vformat( p_reader, i_stream, &bm_info );
		else
			avireader_get_aformat( p_reader, i_stream, &wav_info );
		avireader_get_size( p_reader, &w, &h );
		avireader_get_vrate( p_reader, &rateHz );
		avireader_get_codec( p_reader, i_stream, &codec );
		avireader_get_alloc_size( p_reader, i_stream, &alloc_size );
		avireader_get_frame_count( p_reader, i_stream, &fcount );

		aviDump _dump( streamtype, out, &wav_info /* we hope it is PCM */ );
		buffer = new uint8_t[ alloc_size ];
		uint16_t *aubuff = new uint16_t[alloc_size];
		uint32_t frame_idx = 0;

		while( (read_size = avireader_read_frame( p_reader, frame_idx++, i_stream, buffer )) )
			out.write( (char *)buffer, read_size );

		if( fcount != frame_idx-1 )
			cerr << "Said: " << fcount << ", is: " << frame_idx-1 << endl;

		avireader_destroy( p_reader );
		delete [] aubuff;

	} else {

		smatch what;
		regex reg_exWH( "([1-9][0-9]{0,5})x([1-9][0-9]{0,5})" );
		if( regex_search( input, what, reg_exWH ) ) {
			w = atoi( what[1].str().c_str() );
			h = atoi( what[2].str().c_str() );
		}
		uint32_t sz = w*h * 3 / 2u;
		uint8_t *audio_buffer = nullptr;
		uint32_t audio_chunk_size = 0;

		ifstream in( input, ifstream::binary );
		if( !in.good() ) return -1;
		ifstream audio_in( audiosrc, ifstream::binary );
		if( !audiosrc.empty() ){
			/* we hope it is PCM */
			if( !audio_in.good() ) return -2;
			audio_in.seekg( (std::streamsize)44 );
			audio_chunk_size = (uint32_t)((double)48000*2*8 / 25. + .5); // one chunk per video frame
			audio_chunk_size += audio_chunk_size & 15;
		}

		// http://www.jmcgowan.com/avicodecs.html
		aviwriter_t *p_writer = aviwriter_create();
		aviwriter_set_video_props(p_writer, 0, w, h, (uint32_t)'21VN'/*'024I'*/, sz, 25.);
		aviwriter_set_frame_count( p_writer, 0, 1500 );
		if( !audiosrc.empty() )
			aviwriter_set_audio_props( p_writer, 1, 0x0001/* PCM */, 8, 48000, 48000 * 16, 16, 16 );
		
		if( !aviwriter_open( p_writer, output.c_str() ) ) {
			cerr << "Error opening file" << endl;
			return -1;
		}

		buffer = new uint8_t[ sz ];
		if( !audiosrc.empty() )
			audio_buffer = new uint8_t[ audio_chunk_size ];

		for( int32_t i=0; i<1500; i++ ){
			if(in.eof()){
				in.clear();
				in.seekg(0);
			}
			in.read( (char *)buffer, sz );
			aviwriter_write_frame( p_writer, 0, buffer );
			if( !audiosrc.empty() ){
				if( audio_in.eof() ){
					audio_in.clear();
					audio_in.seekg( (std::streamsize)44 );
				}
				uint32_t read = (uint32_t)audio_in.read( (char *)audio_buffer, audio_chunk_size ).gcount();
				aviwriter_write_chunk( p_writer, 1, audio_buffer, read );
			}
		}
		aviwriter_destroy(p_writer);
		delete [] audio_buffer;
	}

	delete [] buffer;
	
	return 0;
}
