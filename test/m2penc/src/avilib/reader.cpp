
#include <map>
#include <deque>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <array>
#include <assert.h>

#include "avilib.h"
#include "avidef.h"
#include "common.h"
#include "reader.h"


#if defined(_WIN32) || defined(__clang__)
	#define LLU "%llu"
#elif defined(__gnug__)
	#define LLU "%lu"
#else
	#define LLU "%ull"
#endif

using std::string;
using std::ifstream;
using std::streampos;
using std::deque;
using std::memset;

avilib::AviReader::AviReader()
{
}

avilib::AviReader::~AviReader()
{
	_f.close();
}

// https://msdn.microsoft.com/de-de/library/ms779636.aspx
// http://www.onicos.com/staff/iz/formats/wav.html

bool avilib::AviReader::open( const char *filename )
{
	char tag[ 5 ] = {0};
	uint32_t size = 0;
	bool b_audio = false;
	int32_t i_streamHeader = 0;
	m_useMovieOffset = false;
	char message[512];
	clear_log();

	m_moviOffs.clear();
	_f.open( filename, ifstream::binary );
	if( !_f.good() ){
		log( "Cannot parse, file is bad." );
		return false;
	}

	_f.seekg(0,ifstream::beg);
	for(;_f.good();)
	{
		int64_t pos = _f.tellg();
		if( !_f.read( tag, 4 ) ) break;

		snprintf( message, 512, "%s @ " LLU, tag, (uint64_t)pos );
		log( message );

		if( *(uint32_t *)tag == (uint32_t)'KNUJ' ){
			_f.read( (char *)&size, 4 );
			_f.seekg(size,ifstream::cur);
		}
		else if( *(uint32_t *)tag == (uint32_t)'FFIR' ){
			_f.read( (char *)&size, 4 );  // file size
		}
		else if( *(uint32_t *)tag == (uint32_t)' IVA' ){
			continue;
		}
		else if( *(uint32_t *)tag == (uint32_t)'XIVA' ){
			continue; // OPENDML: AVIX, further RIFF chunk(s)
		}
		else if( *(uint32_t *)tag == (uint32_t)'TSIL' ){
			_f.read( (char *)&size, 4 );  // 1st: file size
		}
		else if( *(uint32_t *)tag == (uint32_t)'lrdh' ){
			continue;
		}
		else if( *(uint32_t *)tag == (uint32_t)'hiva' ){
			_f.read( (char *)&size, 4 );  // AVIMAINHEADER
			if( size != sizeof( avilib::AVIMAINHEADER ) - 8 )
				return false;
			m_avimHeader.fcc = *(FOURCC*)tag;
			m_avimHeader.cb = size;
			_f.read( (char *)&m_avimHeader + 8, m_avimHeader.cb );
			snprintf( message, 512, "AVIMAINHEADER (\n  dwFlags=0x%x,\n  dwWidth=%i,\n  dwHeight=%i,\n  dwStreams=%i,\n  dwInitialFrames=%i,\n  dwMaxBytesPerSec=%i\n  dwMicroSecPerFrame=%i\n  dwPaddingGranularity=%i\n  dwSuggestedBufferSize=%i\n  dwTotalFrames=%i\n)",
					m_avimHeader.dwFlags, m_avimHeader.dwWidth, m_avimHeader.dwHeight, m_avimHeader.dwStreams, m_avimHeader.dwInitialFrames, m_avimHeader.dwMaxBytesPerSec, m_avimHeader.dwMicroSecPerFrame, m_avimHeader.dwPaddingGranularity, m_avimHeader.dwSuggestedBufferSize, m_avimHeader.dwTotalFrames );
			log( message );
		}
		else if( *(uint32_t *)tag == (uint32_t)'lrts' ){
			if( i_streamHeader >= 2 ) return false;
			_f.read( (char *)&m_avisHeader[i_streamHeader], sizeof(avilib::AVISTREAMHEADER) );
			b_audio = m_avisHeader[i_streamHeader].fccType._ == (uint32_t)'sdua';
			if( !b_audio )
				m_videoStreamIdx = i_streamHeader;

			snprintf( message, 512, "AVISTREAMHEADER (\n  fccType=%c%c%c%c,\n  fccHandler=%c%c%c%c,\n  dwFlags=%i,\n  wPriority=%i,\n  wLanguage=%i,\n  dwInitialFrames=%i\n  dwScale=%i\n  dwRate=%i\n  dwStart=%i\n  dwLength=%i\n  dwSuggestedBufferSize=%i\n  dwQuality=%i\n  dwSampleSize=%i\n  rcFrame.left=%i\n  rcFrame.top=%i\n  rcFrame.right=%i\n  rcFrame.bottom=%i\n)",
					m_avisHeader[i_streamHeader].fccType.fcc[0] ? m_avisHeader[i_streamHeader].fccType.fcc[0] : '?',
					m_avisHeader[i_streamHeader].fccType.fcc[1] ? m_avisHeader[i_streamHeader].fccType.fcc[1] : '?',
					m_avisHeader[i_streamHeader].fccType.fcc[2] ? m_avisHeader[i_streamHeader].fccType.fcc[2] : '?',
					m_avisHeader[i_streamHeader].fccType.fcc[3] ? m_avisHeader[i_streamHeader].fccType.fcc[3] : '?',
					m_avisHeader[i_streamHeader].fccHandler.fcc[0] ? m_avisHeader[i_streamHeader].fccHandler.fcc[0] : '?',
					m_avisHeader[i_streamHeader].fccHandler.fcc[1] ? m_avisHeader[i_streamHeader].fccHandler.fcc[1] : '?',
					m_avisHeader[i_streamHeader].fccHandler.fcc[2] ? m_avisHeader[i_streamHeader].fccHandler.fcc[2] : '?',
					m_avisHeader[i_streamHeader].fccHandler.fcc[3] ? m_avisHeader[i_streamHeader].fccHandler.fcc[3] : '?',
					m_avisHeader[i_streamHeader].dwFlags,
					m_avisHeader[i_streamHeader].wPriority,
					m_avisHeader[i_streamHeader].wLanguage,
					m_avisHeader[i_streamHeader].dwInitialFrames,
					m_avisHeader[i_streamHeader].dwScale,
					m_avisHeader[i_streamHeader].dwRate,
					m_avisHeader[i_streamHeader].dwStart,
					m_avisHeader[i_streamHeader].dwLength,
					m_avisHeader[i_streamHeader].dwSuggestedBufferSize,
					m_avisHeader[i_streamHeader].dwQuality,
					m_avisHeader[i_streamHeader].dwSampleSize,
					m_avisHeader[i_streamHeader].rcFrame.left,
					m_avisHeader[i_streamHeader].rcFrame.top,
					m_avisHeader[i_streamHeader].rcFrame.right,
					m_avisHeader[i_streamHeader].rcFrame.bottom
					);
			log( message );
			i_streamHeader++;
		}
		else if( *(uint32_t *)tag == (uint32_t)'hrts' ){
			// int z=0;
		}
		else if( *(uint32_t *)tag == (uint32_t)'frts' ){
			_f.read( (char *)&size, 4 );
			if( b_audio ){
				_f.read( (char *)&m_waveformat, std::min( size, (uint32_t)sizeof(avilib::WAVEFORMATEX) ) );
				if( m_waveformat.wFormatTag == WAVE_FORMAT_EXTENSIBLE ){
					_f.seekg(-(std::streamoff)sizeof(avilib::WAVEFORMATEX),ifstream::cur);
					_f.read( (char *)&m_waveformatExt, (uint32_t)sizeof(avilib::WAVEFORMATEXTENSIBLE) );
					_f.seekg(size-sizeof(avilib::WAVEFORMATEXTENSIBLE),ifstream::cur);
					snprintf( message, 512, "WAVEFORMATEXTENSIBLE (\n  wFormatTag=0x%04x,\n  nChannels=%i,\n  nAvgBytesPerSec=%i,\n  nSamplesPerSec=%i,\n  wBitsPerSample=%i,\n  nBlockAlign=%i,\n  SubFormat=...,\n  wSamplesPerBlock=%i,\n  dwChannelMask=%i\n)",
							m_waveformat.wFormatTag, m_waveformat.nChannels, m_waveformat.nAvgBytesPerSec,
							m_waveformat.nSamplesPerSec, m_waveformat.wBitsPerSample, m_waveformat.nBlockAlign,
							m_waveformatExt.Samples.wSamplesPerBlock, m_waveformatExt.dwChannelMask);
				} else if( m_waveformat.wFormatTag == WAVE_FORMAT_MPEG ){
					_f.seekg(-(std::streamoff)sizeof(avilib::WAVEFORMATEX),ifstream::cur);
					_f.read( (char *)&m_waveformatMpeg12, (uint32_t)sizeof(avilib::MPEG1WAVEFORMAT) );
					_f.seekg((std::streamoff)size-sizeof(avilib::MPEG1WAVEFORMAT),ifstream::cur);
					snprintf( message, 512, "MPEG1WAVEFORMAT (\n  wFormatTag=0x%04x,\n  nChannels=%i,\n  nAvgBytesPerSec=%i,\n  nSamplesPerSec=%i,\n  wBitsPerSample=%i,\n  nBlockAlign=%i,\n  dwHeadBitrate=%i,\n  dwPTSHigh=%i,\n  dwPTSLow=%i,\n  fwHeadFlags=%i,\n  fwHeadLayer=%i,\n  fwHeadMode=%i,\n  fwHeadModeExt=%i,\n  wHeadEmphasis%i\n)",
							m_waveformat.wFormatTag, m_waveformat.nChannels, m_waveformat.nAvgBytesPerSec,
							m_waveformat.nSamplesPerSec, m_waveformat.wBitsPerSample, m_waveformat.nBlockAlign,
							m_waveformatMpeg12.dwHeadBitrate, m_waveformatMpeg12.dwPTSHigh, m_waveformatMpeg12.dwPTSLow,
							m_waveformatMpeg12.fwHeadFlags, m_waveformatMpeg12.fwHeadLayer, m_waveformatMpeg12.fwHeadMode,
							m_waveformatMpeg12.fwHeadModeExt, m_waveformatMpeg12.wHeadEmphasis );
				} else if( m_waveformat.wFormatTag == WAVE_FORMAT_MPEGLAYER3 ){
					_f.seekg(-(std::streamoff)sizeof(avilib::WAVEFORMATEX),ifstream::cur);
					_f.read( (char *)&m_waveformatMP3, (uint32_t)sizeof(avilib::MPEGLAYER3WAVEFORMAT) );
					_f.seekg(size-sizeof(avilib::MPEGLAYER3WAVEFORMAT),ifstream::cur);
					snprintf( message, 512, "MPEGLAYER3WAVEFORMAT (\n  wFormatTag=0x%04x,\n  nChannels=%i,\n  nAvgBytesPerSec=%i,\n  nSamplesPerSec=%i,\n  wBitsPerSample=%i,\n  nBlockAlign=%i\n  fdwFlags%i,\n  nBlockSize%i,\n  nCodecDelay%i,\n  nFramesPerBlock%i,\n  wID=%i\n)",
							m_waveformat.wFormatTag, m_waveformat.nChannels, m_waveformat.nAvgBytesPerSec,
							m_waveformat.nSamplesPerSec, m_waveformat.wBitsPerSample, m_waveformat.nBlockAlign,
							m_waveformatMP3.fdwFlags, m_waveformatMP3.nBlockSize, m_waveformatMP3.nCodecDelay, m_waveformatMP3.nFramesPerBlock,
							m_waveformatMP3.wID);
				} else {
					_f.seekg(size-sizeof(avilib::WAVEFORMATEX),ifstream::cur);
					snprintf( message, 512, "WAVEFORMATEX (\n  wFormatTag=0x%04x,\n  nChannels=%i,\n  nAvgBytesPerSec=%i,\n  nSamplesPerSec=%i,\n  wBitsPerSample=%i,\n  nBlockAlign=%i\n)",
							m_waveformat.wFormatTag, m_waveformat.nChannels, m_waveformat.nAvgBytesPerSec,
							m_waveformat.nSamplesPerSec, m_waveformat.wBitsPerSample, m_waveformat.nBlockAlign );
				}
				log( message );
			} else {
				_f.read( (char *)&m_bitmapInfo, std::min( size, (uint32_t)sizeof(avilib::BITMAPINFO) ) );
				if( size>sizeof(avilib::BITMAPINFO) )
					_f.seekg(size-sizeof(avilib::BITMAPINFO),ifstream::cur);
				snprintf( message, 512, "BITMAPINFO (\n  biWidth=%i,\n  biHeight=%i,\n  biPlanes=%i,\n  biBitCount=%i,\n  biCompression=%s,\n  biSizeImage=%i,\n  biXPelsPerMeter=%i,\n  biYPelsPerMeter=%i,\n  biClrUsed=%i,\n  biClrImportant=%i,\n)",
						m_bitmapInfo.biWidth, m_bitmapInfo.biHeight, m_bitmapInfo.biPlanes,
						m_bitmapInfo.biBitCount, m_bitmapInfo.biCompression.fcc, m_bitmapInfo.biSizeImage,
						m_bitmapInfo.biXPelsPerMeter, m_bitmapInfo.biYPelsPerMeter,
						m_bitmapInfo.biClrUsed, m_bitmapInfo.biClrImportant);
				log( message );
			}
		}
		else if( *(uint32_t *)tag == (uint32_t)'OFNI' ){
			continue;
		}
		else if( *(uint32_t *)tag == (uint32_t)'TFSI' ){
			_f.read( (char *)&size, 4 );
			_f.seekg(size,ifstream::cur);
		}
		else if( *(uint32_t *)tag == (uint32_t)'nrts' ){
			_f.read( (char *)&size, 4 );
			_f.seekg(size-4,ifstream::cur);
		}
		else if( *(uint32_t *)tag == (uint32_t)'ivom' ){
			// the movie list, here is the data.
			m_moviOffs.push_back((size_t)_f.tellg() - 4);
			_f.seekg(size-4, ifstream::cur);
		}
		else if( *(uint32_t *)tag == (uint32_t)'1xdi' ){
			_f.read( (char *)&size, 4 );
			uint8_t *p = new uint8_t[size], *p0 = p;
			size = (uint32_t)_f.read((char *)p,size).gcount();
			for (size_t i=0; i<size/sizeof(AVIOLDINDEX); i++) {
				AVIOLDINDEX *idx = (AVIOLDINDEX *)p;
				p += sizeof(AVIOLDINDEX);
				
				if( idx->dwChunkId._ == ' cer' )
					continue;

				DMLINDEX dml_index;
				dml_index.dwChunkId = idx->dwChunkId;
				dml_index.u32_flags = idx->dwFlags;
				dml_index.i64_offset = idx->dwOffset;
				dml_index.u32_size = idx->dwSize;
				string fcc = idx->dwChunkId.fcc;
				int32_t stream_idx = atoi(fcc.substr(0,2).c_str());
				m_frameIdxs[stream_idx].push_back(dml_index);
/*
				switch(m_videoStreamIdx){
				case 0:
					if( idx->dwChunkId._ == (uint32_t)'cd00' || idx->dwChunkId._ == (uint32_t)'bd00' )//'bw10' )
						m_frameIdxs[stream_idx].push_back(dml_index);
					break;
				case 1:
					if( idx->dwChunkId._ == (uint32_t)'cd10' || idx->dwChunkId._ == (uint32_t)'bd10' )//'bw10' )
						m_frameIdxs[stream_idx].push_back(dml_index);
					break;
				}
 */
			}
			snprintf( message, 512, "%x AVIOLDINDEX", size/( uint32_t )sizeof(AVIOLDINDEX));
			log( message );
			delete [] p0;
		}
		else if( *(uint32_t *)tag == (uint32_t)'lmdo' ){
			continue;
		}
		else if( *(uint32_t *)tag == (uint32_t)'hlmd' ){
			_f.read( (char *)&size, 4 );
			_f.read((char *)&m_odmlExt, sizeof(avilib::ODMLExtendedAVIHeader));
			_f.seekg(size - sizeof(avilib::ODMLExtendedAVIHeader),ifstream::cur);
			snprintf( message, 512, "ODMLExtendedAVIHeader ( dwTotalFrames=%i )", m_odmlExt.dwTotalFrames); log( message );
		}
		else if( *(uint32_t *)tag == (uint32_t)'lhmd' ){
			_f.read( (char *)&size, 4 );
			_f.seekg(size,ifstream::cur);
		}
		else if( *(uint32_t *)tag == (uint32_t)'xdni' ){
			_f.read( (char *)&size, 4 );  // AVISUPERINDEX
			streampos superindex_end = _f.tellg() + (streampos)size;

			avilib::AVISUPERINDEX super_index;
			super_index.fcc._ = *((uint32_t *)&tag);
			super_index.cb = size;
			_f.read( (char *)&super_index + 8, sizeof(avilib::AVISUPERINDEX)-8 );

			string fcc = super_index.dwChunkId.fcc;
			int32_t stream_idx = atoi(fcc.substr(0,2).c_str());

			assert( 0 == super_index.bIndexSubType );
			if( 0 != super_index.bIndexSubType ) log( "Warning: 0 != super_index.bIndexSubType" );
			if( AVI_INDEX_OF_INDEXES != super_index.bIndexType ) log( "Warning: AVI_INDEX_OF_INDEXES != super_index.bIndexType" );
			deque<avilib::AVISUPERINDEX_ENTRY> entries;

			for( uint32_t i=0; i<super_index.nEntriesInUse; i++ )
			{
				avilib::AVISUPERINDEX_ENTRY entry;
				_f.read( (char *)&entry, sizeof(avilib::AVISUPERINDEX_ENTRY) );
				entries.push_back(entry);
			}

			snprintf( message, 512, "%ux AVISUPERINDEX_ENTRY (%s)", super_index.nEntriesInUse, fcc.c_str()); log( message );

			for( avilib::AVISUPERINDEX_ENTRY &entry : entries ){
				_f.seekg( entry.qwOffset, ifstream::beg );
				avilib::AVISTDINDEX std_index;
				memset( &std_index, 0x0, sizeof(avilib::AVISTDINDEX));
				_f.read( (char *)&std_index, sizeof(avilib::AVISTDINDEX) );

				if( std_index.dwChunkId._ == ' cer' )
					continue;

				std::array<uint32_t,6> allowed = {
					(uint32_t)'cd00', (uint32_t)'bd00', (uint32_t)'cd10', (uint32_t)'bd10',
					(uint32_t)'bw00', (uint32_t)'bw10' };

				if( std::find( allowed.begin(),allowed.end(), std_index.dwChunkId._ ) != allowed.end() ){

					// not for audio:
					if( AVI_INDEX_OF_CHUNKS != std_index.bIndexType ) log( "Warning: AVI_INDEX_OF_CHUNKS != std_index.bIndexType" );
					if( 0 != std_index.bIndexSubType ) log( "Warning: 0 != std_index.bIndexSubType" );

					//if( std_index.fcc._ == (uint32_t)'00xi' )
					{
						uint8_t *p = new uint8_t[std_index.cb], *p0 = p;
						_f.read( (char *)p, std_index.cb );
						for( uint32_t j=0; j<std_index.nEntriesInUse; j++ )
						{
							avilib::AVISTDINDEX_ENTRY *idx = (avilib::AVISTDINDEX_ENTRY *)p;
							p += sizeof(avilib::AVISTDINDEX_ENTRY);
						
							DMLINDEX dml_index;
							dml_index.dwChunkId = std_index.dwChunkId;
							dml_index.u32_flags = 0;
							dml_index.i64_offset = std_index.qwBaseOffset + idx->dwOffset;// + 4;
							dml_index.u32_size = idx->dwSize & 0x7FFFFFFF;
							if(dml_index.u32_size > m_bitmapInfo.biSizeImage){
								// int z=0;
							}

							odml_frameIdxs[stream_idx].push_back(dml_index);
						}
						delete [] p0;
					}
				}
				_f.clear();
			} // each superindex entry
			m_superIndex[stream_idx] = super_index;
			_f.clear();
			_f.seekg(superindex_end,ifstream::beg);
		}
		else if( *(uint32_t *)tag == (uint32_t)'prpv' ){
			// this does nothing and has never been tested.
			_f.read( (char *)&size, 4 );
			VIDEO_PROP_HEADER vprop_header;
			_f.read( (char *)&vprop_header, std::min((size_t)size,sizeof(VIDEO_PROP_HEADER)) );
			VIDEO_PROP_HEADER::VIDEO_FIELD_DESC *p_field_desc = (VIDEO_PROP_HEADER::VIDEO_FIELD_DESC *)malloc(sizeof(VIDEO_PROP_HEADER::VIDEO_FIELD_DESC)*vprop_header.nbFieldPerFrame);
			_f.read((char *)p_field_desc, vprop_header.nbFieldPerFrame*sizeof(VIDEO_PROP_HEADER::VIDEO_FIELD_DESC));
			free( p_field_desc );
		}
		else if( *(uint32_t *)tag == ( uint32_t )'tadT'/* Adobe Premiere Timecode */ ){
			continue;
		}
		else
		{
			_f.read( (char *)&size, 4 );
			_f.seekg(size,ifstream::cur);
			// FIXME: check size is smaller than current parent element's size
		}
	}

	if( !odml_frameIdxs.empty() ){
		m_frameIdxs.swap(odml_frameIdxs);
		log( "using OpenDML index" );
	}

	_f.clear();
	_f.seekg(0,ifstream::beg);

	if( m_moviOffs.empty() )
		return false;
	
	return true;
}


int32_t avilib::AviReader::read_frame( uint32_t idx, uint32_t stream, void *p_data )
{
	// if( _f.bad() ) return false;
	// if( _f.eof() ) return false;
	char tag_size[8];
	int32_t i_read = 0;
	uint64_t //ui64_tmpMoviOff,
 		ui64_moviOffset = 0;
	DMLINDEX *p_aviIndex;

	if( idx >= m_frameIdxs[stream].size() )
		return false;

	// if( idx >= m_frameIdxs[m_videoStreamIdx].size() ){
	// 	idx -= m_frameIdxs[m_videoStreamIdx].size();
	// 	if( idx >= odml_frameIdxs[m_videoStreamIdx].size() )
	// 		return false;
	// 	aviIndex = &odml_frameIdxs[m_videoStreamIdx][idx];
	// }
	// else
	p_aviIndex = &m_frameIdxs[stream][idx];

	if( !p_aviIndex->u32_size )
		return true;

	if( m_useMovieOffset )
		goto skip_search;

	_f.clear();

	// try without offset
	_f.seekg(p_aviIndex->i64_offset,ifstream::beg);
	_f.read( tag_size, 8 );
	i_read += 8;
	if( p_aviIndex->dwChunkId._ != *((uint32_t *)tag_size) ){
		// try with offset
skip_search:
		//ui64_tmpMoviOff = _f.tellg();
		//for_each(m_moviOffs.begin(),m_moviOffs.end(),[&](uint64_t &off){
		//	if( ui64_tmpMoviOff >= off )
		//		ui64_moviOffset = off;
		//});
		
		ui64_moviOffset = m_moviOffs[0];
		_f.seekg(p_aviIndex->i64_offset+ui64_moviOffset,ifstream::beg);
		_f.read( (char *)tag_size, 8 );
		i_read += 8;
		m_useMovieOffset = p_aviIndex->dwChunkId._ == *((uint32_t *)tag_size);
		if( !m_useMovieOffset )
			_f.seekg(p_aviIndex->i64_offset,ifstream::beg); // probably no chunk header.
	}

	// uint64_t tell = _f.tellg().seekpos();
	//_f.read( (char *)p_data, p_aviIndex->u32_size );
	//return true;

	return (int32_t)_f.read( (char *)p_data, p_aviIndex->u32_size ).gcount();
}

bool avilib::AviReader::close()
{
	if( _f.bad() ) return false;
	
	_f.close();
	
	return true;
}

bool avilib::AviReader::getSize( int32_t &width, int32_t &height )
{
	if( _f.bad() ) return false;
	
	width = m_avimHeader.dwWidth;
	height = m_avimHeader.dwHeight;
	return true;
}

bool avilib::AviReader::getFrameRate( double &rateHz )
{
	if( _f.bad() ) return false;

	rateHz = 1000000. / m_avimHeader.dwMicroSecPerFrame;
	return true;
}

bool avilib::AviReader::getFormat( uint32_t stream, avilib_BITMAPINFO &bm_info )
{
	if( _f.bad() ) return false;

	memcpy( &bm_info, &m_bitmapInfo, sizeof(avilib_BITMAPINFO) );
	return true;
}

bool avilib::AviReader::getFormat( uint32_t stream, avilib_WAVEFORMATEX &wav_info )
{
	if( _f.bad() ) return false;

	memcpy( &wav_info, &m_waveformat, sizeof(avilib_WAVEFORMATEX) );
	return true;
}

bool avilib::AviReader::getFrameCount( uint32_t stream, uint32_t &count )
{
	if( _f.bad() ) return false;
	count = (uint32_t)m_frameIdxs[stream].size();

	return true;
}
bool avilib::AviReader::getCodec( uint32_t stream, uint32_t &codec )
{
	if( _f.bad() ) return false;

	if( stream == (uint32_t)m_videoStreamIdx )
		codec = (uint32_t)m_bitmapInfo.biCompression._;
	else
		codec = (uint32_t)m_waveformat.wFormatTag;
	return true;
}
bool avilib::AviReader::getStreamCount( uint32_t &count )
{
	if( _f.bad() ) return false;

	count = (uint32_t)m_frameIdxs.size();
	return true;
}
bool avilib::AviReader::getAllocSize( uint32_t stream, uint32_t &size )
{
	if( _f.bad() ) return false;

	size = 0;
	for( const DMLINDEX &dmlIdx : m_frameIdxs[stream] )
		size = std::max( size, dmlIdx.u32_size );
	size = std::max( size, stream == (uint32_t)m_videoStreamIdx ? m_bitmapInfo.biSizeImage
					: m_avisHeader[stream].dwSuggestedBufferSize );
	return true;
}
bool avilib::AviReader::getStreamType( uint32_t stream, avilib_streamtype_t &type )
{
	if( _f.bad() ) return false;

	type = (uint32_t)m_videoStreamIdx == stream ? avilib_Video : avilib_Audio;
	return true;
}



