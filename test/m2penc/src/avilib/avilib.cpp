
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <map>

#include "avilib.h"
#include "avidef.h"
#include "common.h"
#include "writer.h"
#include "reader.h"



#define AVILIB_VERSION_Maj 0
#define AVILIB_VERSION_Min 2
#define AVILIB_VERSION_Rev 0
#define STR_(x) #x
#define STR(x) STR_(x)
#define AVILIB_VERSION_Str "avilib ver" STR(AVILIB_VERSION_Maj) "." STR(AVILIB_VERSION_Min) "." STR(AVILIB_VERSION_Rev) " 2018"
AVI_DLL_DECL void avilib_version( int32_t *p_maj, int32_t *p_min, int32_t *p_rev, const char **ppc_string ){
	if( p_maj ) *p_maj = AVILIB_VERSION_Maj;
	if( p_min ) *p_min = AVILIB_VERSION_Min;
	if( p_rev ) *p_rev = AVILIB_VERSION_Rev;
	if( ppc_string ) *ppc_string = AVILIB_VERSION_Str;
}

AVI_DLL_DECL avireader_t *avireader_create(){
	return (avireader_t *)new avilib::AviReader;
}
AVI_DLL_DECL void avireader_destroy( avireader_t *p_context ){
	delete (avilib::AviReader *)p_context;
}
AVI_DLL_DECL int32_t avireader_open( avireader_t *p_context, const char *filename ){
	return ((avilib::AviReader *)p_context)->open( filename ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_close( avireader_t *p_context ){
	return ((avilib::AviReader *)p_context)->close() ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_stream_count( avireader_t *p_context, uint32_t *count ){
	return ((avilib::AviReader *)p_context)->getStreamCount( *count ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_read_frame( avireader_t *p_context, uint32_t idx, uint32_t stream, void *data ){
	return ((avilib::AviReader *)p_context)->read_frame( idx, stream, (uint8_t *)data );
}
AVI_DLL_DECL int32_t avireader_get_size( avireader_t *p_context, int32_t *width, int32_t *height ){
	return ((avilib::AviReader *)p_context)->getSize( *width, *height ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_vrate( avireader_t *p_context, double *rateHz ){
	return ((avilib::AviReader *)p_context)->getFrameRate( *rateHz ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_frame_count( avireader_t *p_context, uint32_t stream, uint32_t *count ){
	return ((avilib::AviReader *)p_context)->getFrameCount( stream, *count ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_codec( avireader_t *p_context, uint32_t stream, uint32_t *codec ){
	return ((avilib::AviReader *)p_context)->getCodec( stream, *codec ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_alloc_size( avireader_t *p_context, uint32_t stream, uint32_t *size ){
	return ((avilib::AviReader *)p_context)->getAllocSize( stream, *size ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_vformat( avireader_t *p_context, uint32_t stream, avilib_BITMAPINFO *bm_info ){
	return ((avilib::AviReader *)p_context)->getFormat( stream, *bm_info ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_aformat( avireader_t *p_context, uint32_t stream, avilib_WAVEFORMATEX *a_info ){
	return ((avilib::AviReader *)p_context)->getFormat( stream, *a_info ) ? 1 : 0;
}
AVI_DLL_DECL int32_t avireader_get_stream_type( avireader_t *p_context, uint32_t stream, avilib_streamtype_t *type ){
	return ((avilib::AviReader *)p_context)->getStreamType( stream, *type ) ? 1 : 0;
}









AVI_DLL_DECL aviwriter_t *aviwriter_create(){
	return (aviwriter_t *)new avilib::AviWriter;
}
AVI_DLL_DECL void aviwriter_destroy( aviwriter_t *p_context ){
	delete (avilib::AviWriter *)p_context;
}
AVI_DLL_DECL int32_t aviwriter_open( aviwriter_t *p_context, const char *filename ){
	return ((avilib::AviWriter *)p_context)->open( filename ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_close( aviwriter_t *p_context ){
	return ((avilib::AviWriter *)p_context)->close() ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_write_frame( aviwriter_t *p_context, uint8_t stream_idx, void *data ){
	return ((avilib::AviWriter *)p_context)->write_frame( stream_idx, data ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_write_chunk( aviwriter_t *p_context, uint8_t stream_idx, void *data, uint32_t len ){
	return ((avilib::AviWriter *)p_context)->write_data( stream_idx, data, len ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_set_video_props( aviwriter_t *p_context, uint8_t stream_idx, int32_t width, int32_t height, uint32_t codec, uint32_t framesize, double rateHz ){
	return ((avilib::AviWriter *)p_context)->setVideoProperties( stream_idx, width, height, codec, framesize, rateHz ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_set_audio_props( aviwriter_t *p_context, uint8_t stream_idx, int16_t format, int8_t channels, uint32_t samplesPerSecond, uint32_t avgBytesPerSecond, uint16_t bitsPerSample, uint16_t blockAlign ){
	return ((avilib::AviWriter *)p_context)->setAudioProperties( stream_idx, format, channels, samplesPerSecond, avgBytesPerSecond, bitsPerSample, blockAlign ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_set_frame_count( aviwriter_t *p_context, uint8_t stream_idx, uint32_t count ){
	return ((avilib::AviWriter *)p_context)->setFrameCount( stream_idx, count ) ? 1 : 0;
}
AVI_DLL_DECL int32_t aviwriter_openDML( aviwriter_t *p_context, int32_t openDML ){
	return ( ( avilib::AviWriter * )p_context )->openDML( !!openDML ) ? 1 : 0;
}







int32_t avilib::gcd( int32_t nom, int32_t denom )
{
	if( nom < denom ) {
		int32_t tmp = nom;
		nom = denom;
		denom = tmp;
	}
	if( !(nom % denom) )
		return denom;
	return gcd( denom, nom % denom );
}

bool avilib::cancel( int32_t nom, int32_t denom, int32_t &out_nom, int32_t &out_denom )
{
	if( !denom || !nom ) return false;
	int32_t _gcd = gcd( nom, denom );

	if( _gcd ) {
		out_nom = nom / _gcd;
		out_denom = denom / _gcd;
		return true;
	}

	return false;
}




using std::string;
using std::cout;
using std::endl;

void avilib::LoggingObject::log( const string &message ){
	#ifdef DEBUG
	cout << message << endl;
	#endif
	m_messages.push_back(message);
}
bool avilib::LoggingObject::getNextMessage( string &message ){
	if(m_messages.empty()) return false;
	message = m_messages.front();
	m_messages.pop_front();
	return true;
}
void avilib::LoggingObject::clear_log(){
	m_messages.clear();
}




avilib::FOURCC avilib::generate_fcc( std::string id, uint8_t idx )
{
	char buff[3] = {0};
	avilib::FOURCC ret = {0};
	snprintf( buff, 3, "%02i", idx > 99 ? 99 : idx < 0 ? 0 : idx);
	ret.fcc[0] = buff[0];
	ret.fcc[1] = buff[1];
	ret.fcc[2] = id.c_str()[0];
	ret.fcc[3] = id.c_str()[1];
	return ret;
}
