
#include <stdint.h>
#ifdef _WIN32
	#ifndef _LIB
		#ifndef AVI_IMPORT
			#define AVI_DLL_DECL __declspec(dllexport)
		#else
			#define AVI_DLL_DECL __declspec(dllimport)
		#endif
	#else
		#define AVI_DLL_DECL
	#endif
#else
	#define AVI_DLL_DECL __attribute__ ((visibility ("default")))
#endif

typedef struct
{
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;

} avilib_BITMAPINFO;


typedef struct {

	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	
} avilib_WAVEFORMATEX;

typedef enum { avilib_UnknownStreamtype, avilib_Video, avilib_Audio } avilib_streamtype_t;

typedef void avireader_t;
AVI_DLL_DECL avireader_t *avireader_create();
AVI_DLL_DECL void avireader_destroy( avireader_t *p_context );
AVI_DLL_DECL int32_t avireader_open( avireader_t *p_context, const char *filename );
AVI_DLL_DECL int32_t avireader_close( avireader_t *p_context );
AVI_DLL_DECL int32_t avireader_read_frame( avireader_t *p_context, uint32_t idx, uint32_t stream, void *data );
AVI_DLL_DECL int32_t avireader_get_size( avireader_t *p_context, int32_t *width, int32_t *height );
AVI_DLL_DECL int32_t avireader_get_vrate( avireader_t *p_context, double *rateHz );
AVI_DLL_DECL int32_t avireader_get_frame_count( avireader_t *p_context, uint32_t stream, uint32_t *count );
AVI_DLL_DECL int32_t avireader_get_codec( avireader_t *p_context, uint32_t stream, uint32_t *codec );
AVI_DLL_DECL int32_t avireader_get_alloc_size( avireader_t *p_context, uint32_t stream, uint32_t *size );
AVI_DLL_DECL int32_t avireader_get_stream_count( avireader_t *p_context, uint32_t *count );
AVI_DLL_DECL int32_t avireader_get_vformat( avireader_t *p_context, uint32_t stream, avilib_BITMAPINFO *bm_info );
AVI_DLL_DECL int32_t avireader_get_aformat( avireader_t *p_context, uint32_t stream, avilib_WAVEFORMATEX *a_info );
AVI_DLL_DECL int32_t avireader_get_stream_type( avireader_t *p_context, uint32_t stream, avilib_streamtype_t *type );


typedef void aviwriter_t;
AVI_DLL_DECL aviwriter_t *aviwriter_create();
AVI_DLL_DECL void aviwriter_destroy( aviwriter_t *p_context );
AVI_DLL_DECL int32_t aviwriter_open( aviwriter_t *p_context, const char *filename );
AVI_DLL_DECL int32_t aviwriter_close( aviwriter_t *p_context );
AVI_DLL_DECL int32_t aviwriter_write_frame( aviwriter_t *p_context, uint8_t stream_idx, void *data );
AVI_DLL_DECL int32_t aviwriter_write_chunk( aviwriter_t *p_context, uint8_t stream_idx, void *data, uint32_t len );
AVI_DLL_DECL int32_t aviwriter_set_video_props( aviwriter_t *p_context, uint8_t stream_idx, int32_t width, int32_t height, uint32_t codec, uint32_t framesize, double rateHz );
AVI_DLL_DECL int32_t aviwriter_set_audio_props( aviwriter_t *p_context, uint8_t stream_idx, int16_t format, int8_t channels, uint32_t samplesPerSecond, uint32_t avgBytesPerSecond, uint16_t bitsPerSample, uint16_t blockAlign );
AVI_DLL_DECL int32_t aviwriter_set_frame_count( aviwriter_t *p_context, uint8_t stream_idx, uint32_t count );
AVI_DLL_DECL int32_t aviwriter_openDML( aviwriter_t *p_context, int32_t openDML );


AVI_DLL_DECL void avilib_version( int32_t *p_maj, int32_t *p_min, int32_t *p_rev, const char **ppc_string );
