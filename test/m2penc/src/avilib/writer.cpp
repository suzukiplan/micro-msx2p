

#include <fstream>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

#include "avidef.h"
#include "avilib.h"
#include "common.h"
#include "writer.h"


using std::ofstream;
using std::deque;
using std::memset;


#define STDINDEXSIZE 0x4000
#ifdef _WIN32
	#define filepos() tellp().seekpos()
#else
	#define filepos() tellp()
#endif


avilib::AviWriter::AviWriter()
{
	reset();
}

void avilib::AviWriter::reset()
{
	memset(&m_avimHeader, 0x0, sizeof(avilib::AVIMAINHEADER));
	m_avimHeader.fcc = 'hiva';
	m_avimHeader.cb = sizeof( avilib::AVIMAINHEADER ) - 8;
	m_avimHeader.dwFlags = AVIF_HASINDEX/* | AVIF_MUSTUSEINDEX | AVIF_TRUSTCKTYPE*/| AVIF_ISINTERLEAVED;
	
	memset(&m_bitmapInfo, 0x0, sizeof(avilib::BITMAPINFO));
	m_bitmapInfo.biSize = sizeof( avilib::BITMAPINFO );
	m_bitmapInfo.biPlanes = 1;
	m_bitmapInfo.biBitCount = 24; // ??
	m_bitmapInfo.biCompression = BI_RGB;
	
	memset(&m_waveFormat, 0x0, sizeof(avilib::WAVEFORMATEXTENSIBLE));
	m_waveFormat.Format.cbSize = sizeof( avilib::WAVEFORMATEX ) - 4;

	uint8_t guid_pcm[] = { 0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71 };
	memcpy( m_waveFormat.SubFormat, &guid_pcm, 16 );
}

avilib::AviWriter::~AviWriter()
{
	close();
}


bool avilib::AviWriter::openDML( bool yes )
{
	if( m_opened ) return false;
	m_openDML = yes;
	return true;
}


uint32_t NULL32 = 0;
bool avilib::AviWriter::open( const char *filename )
{
	if( !m_streamTypes.size() )
		return false;

	if( !m_openDML && !m_reservedFrames )
		return false;
	
	// more than two not supported
	// must be video or video + audio

	int32_t i_tmp = 0;
	std::streamoff off_tmp;

	for( auto idx_type : m_streamTypes )
		if( i_tmp++ != idx_type.first )
			return false; // not adjacent
	
	_f.open( filename, ofstream::binary|ofstream::trunc );
	if( !_f.good() ) return false;

	_f.write( "RIFF", 4 );
	pos_RIFFSize = _f.tellp();
	_f.write( (const char *)&NULL32, 4 ); // we'll come back to this later (pos_RIFFSize)
	_f.write( "AVI ", 4 );
	_f.write( "LIST", 4 );

	std::streampos pos_hdrlSize = _f.tellp();
	_f.write( (const char *)&NULL32, 4 );
	_f.write( "hdrl", 4 );
	pos_aviMainHeader = _f.tellp();
	_f.write( (const char *)&m_avimHeader, sizeof(avilib::AVIMAINHEADER) );

	// streams begin
	int32_t i_streams = (int32_t)m_streamTypes.size();
	for (int32_t i_stream=0; i_stream<i_streams; i_stream++)
 	{
		_f.write( "LIST", 4 );
		std::streampos pos_strlSize = _f.tellp();
		_f.write( (const char *)&NULL32, 4 );

		_f.write( "strl", 4 );
		pos_streamHeader[ i_stream ] = _f.tellp();
		_f.write( (const char *)&m_avisHeaders[i_stream], sizeof(avilib::AVISTREAMHEADER) );

		_f.write( "strf", 4 );
		if( m_streamTypes[ i_stream ] == avilib_Video )
		{
			i_tmp = sizeof(avilib::BITMAPINFO);
			_f.write( (const char *)&i_tmp, 4 );
			_f.write( (const char *)&m_bitmapInfo, sizeof(avilib::BITMAPINFO) );
		}
		else if( m_streamTypes[ i_stream ] == avilib_Audio )
		{
			m_waveFormat.Format.cbSize = sizeof( avilib::WAVEFORMATEXTENSIBLE ) - sizeof( avilib::WAVEFORMATEX );
			i_tmp = sizeof(avilib::WAVEFORMATEXTENSIBLE);
			_f.write( (const char *)&i_tmp, 4 );
			_f.write( (const char *)&m_waveFormat, sizeof( avilib::WAVEFORMATEXTENSIBLE ) );
		}

		if( m_openDML )
		{
			uint32_t num_stdIndexes = 128;
			if( m_reservedFrames != 0 ){
				num_stdIndexes = (m_reservedFrames + ((STDINDEXSIZE - sizeof(avilib::AVISTDINDEX)) / sizeof(avilib::AVISTDINDEX_ENTRY)) - 1) / ((STDINDEXSIZE - sizeof(avilib::AVISTDINDEX)) / sizeof(avilib::AVISTDINDEX_ENTRY));
				if( !num_stdIndexes ){
					m_openDML = false; // just guessing
				}
			} else {
				m_reservedFrames = num_stdIndexes * ((STDINDEXSIZE - sizeof(avilib::AVISTDINDEX)) / sizeof(avilib::AVISTDINDEX_ENTRY));
			}

			if( m_openDML )
			{
				size_t clearsize = sizeof( AVISUPERINDEX ) + num_stdIndexes*sizeof(avilib::AVISUPERINDEX_ENTRY);

				// cf. http://msdn.microsoft.com/en-us/library/windows/desktop/ff625871(v=vs.85).aspx
				m_superIdxs[i_stream].fcc = 'xdni';
				m_superIdxs[i_stream].nEntriesInUse = 1; // runtime adapted
				m_superIdxs[i_stream].cb = (uint32_t)clearsize - 8;
				m_superIdxs[i_stream].wLongsPerEntry = 4;
				m_superIdxs[i_stream].bIndexSubType = 0; //[ AVI_INDEX_2FIELD | 0 ]
				m_superIdxs[i_stream].bIndexType = AVI_INDEX_OF_INDEXES;
				m_superIdxs[i_stream].dwChunkId = generate_fcc( m_streamTypes[ i_stream ] == avilib_Audio ? "wb" : "dc", i_stream );
				m_superIdxs[i_stream].dwReserved[0] = 0;
				m_superIdxs[i_stream].dwReserved[1] = 0;
				m_superIdxs[i_stream].dwReserved[2] = 0;

				pos_odmlSuperIdx[i_stream] = _f.tellp();
				uint8_t *p = new uint8_t[clearsize];
				memset(p,0x0,clearsize);
				_f.write( (const char *)p, clearsize );
				delete [] p;
				// super index later -
			}
		}

		// write strl size
		off_tmp = _f.tellp();
		uint32_t ui_strlSize = (uint32_t)(off_tmp - pos_strlSize) - 4;
		_f.seekp(pos_strlSize);
		_f.write( (const char *)&ui_strlSize, 4);
		_f.seekp(off_tmp);

		// ---

		if( m_openDML )
		{
			avilib::AVISTDINDEX std_index;
			std_index.fcc = i_stream == 1 ? '10xi' : '00xi';
			std_index.wLongsPerEntry = sizeof( avilib::AVISTDINDEX_ENTRY ) / sizeof( DWORD );
			std_index.bIndexSubType = 0; // must be 0
			std_index.dwChunkId = generate_fcc( m_streamTypes[ i_stream ] == avilib_Audio ? "wb":"dc", i_stream );
			std_index.qwBaseOffset = 0; // all dwOffsets in aIndex array are relative to this
			std_index.dwReserved = 0; // must be 0
			m_stdIndexes[i_stream].push_back( std_index );
		}
	} // streams

	if( m_openDML )
	{
		_f.write( "LIST", 4 );
		i_tmp = 248 + 12;
		_f.write( (const char *)&i_tmp, 4 );
		_f.write( "odml", 4 );
		_f.write( "dmlh", 4 );
		i_tmp = 248;
		_f.write( (const char *)&i_tmp, 4 );
		pos_odmlExt = _f.tellp();
		uint8_t *p = new uint8_t[ i_tmp ];
		avilib::ODMLExtendedAVIHeader odml_ext = { 0 };
		memset( p, 0x0, i_tmp );
		memcpy( p, &odml_ext, sizeof( avilib::ODMLExtendedAVIHeader ) );
		_f.write( (const char *)p, i_tmp );
		delete[] p;
	}


	// write hdrl size
	off_tmp = _f.tellp();
	uint32_t ui_hdrlSize = (uint32_t)(_f.tellp() - pos_hdrlSize) - 4;
	_f.seekp(pos_hdrlSize);
	_f.write( (const char *)&ui_hdrlSize, 4);
	_f.seekp(off_tmp);


	// movi LIST
	_f.write( "LIST", 4 );
	pos_moviListSize = _f.tellp();
	_f.write( (const char *)&NULL32, 4 ); // later ...
	pos_1stMoviStart = _f.tellp();
	_f.write( "movi", 4 );

	m_RIFF_size = (uint32_t)_f.tellp() + sizeof( avilib::AVIOLDINDEX );
	m_totalFrames = 0;
	m_currBaseOff = 0;


	return _f.good();
}

bool avilib::AviWriter::close()
{
	if( _f.bad() ) return false;

	_f.seekp(pos_aviMainHeader);
	_f.write( (const char *)&m_avimHeader, sizeof(avilib::AVIMAINHEADER) );

	int32_t i_streams = (int32_t)m_streamTypes.size();

	if( m_openDML )
	{
		// write odml total frame count
		_f.seekp(pos_odmlExt);
		avilib::ODMLExtendedAVIHeader odml_ext = { static_cast<DWORD>(m_totalFrames) };
		_f.write((const char *)&odml_ext, sizeof(avilib::ODMLExtendedAVIHeader));

		// write "00xi"s at the very end
		for( int32_t stream_idx = 0; stream_idx < i_streams; stream_idx++ )
		{
			deque<avilib::AVISUPERINDEX_ENTRY> deq_supIdxEntries;
			for( auto &pair : m_stdIndexEntries[stream_idx] )
			{
				avilib::AVISTDINDEX &std_index = m_stdIndexes[stream_idx][pair.first];
				const deque<avilib::AVISTDINDEX_ENTRY> &deq_entries = pair.second;

				std_index.bIndexType = AVI_INDEX_OF_CHUNKS;
				std_index.nEntriesInUse = (uint32_t)deq_entries.size();
				std_index.cb = (uint32_t)(sizeof(avilib::AVISTDINDEX)-8 + sizeof(avilib::AVISTDINDEX_ENTRY)*deq_entries.size());

				_f.seekp( 0, ofstream::end );
				uint64_t pos_stdindex = _f.filepos();
				assert(_f.good());

				_f.write( (const char *)&std_index, sizeof(avilib::AVISTDINDEX) );
			
				uint8_t *p = new uint8_t[sizeof(avilib::AVISTDINDEX_ENTRY)*deq_entries.size()], *p0 = p;
				for( const avilib::AVISTDINDEX_ENTRY &entry : deq_entries ){
					*(avilib::AVISTDINDEX_ENTRY *)p = entry;
					p += sizeof(avilib::AVISTDINDEX_ENTRY);
				};
				_f.write( (const char *)p0, sizeof(avilib::AVISTDINDEX_ENTRY)*deq_entries.size() );
				delete [] p0;

				avilib::AVISUPERINDEX_ENTRY supidx_entry;
				supidx_entry.qwOffset = pos_stdindex;
				supidx_entry.dwSize = std_index.cb+8;
				supidx_entry.dwDuration = static_cast<DWORD>(deq_entries.size()); // frame count (video)
				deq_supIdxEntries.push_back(supidx_entry);
			};

			// write that list to superindex
			_f.seekp( pos_odmlSuperIdx[stream_idx], ofstream::beg );
			_f.write( (const char *)&m_superIdxs[stream_idx], sizeof(AVISUPERINDEX) );
			for( avilib::AVISUPERINDEX_ENTRY &supidx_entry : deq_supIdxEntries ){
				_f.write( (const char *)&supidx_entry, sizeof(avilib::AVISUPERINDEX_ENTRY));
			};
		}
	}



	// write stream headers
	for( int32_t k=0; k<i_streams; k++ ){
		if( m_streamTypes[ k ] == avilib_Audio && m_videoStreamIdx != ~0u ){
			uint32_t vrate = m_avisHeaders[ m_videoStreamIdx ].dwRate;
			uint32_t vscal = m_avisHeaders[ m_videoStreamIdx ].dwScale;
			if( vrate && m_avisHeaders[ k ].dwScale && m_waveFormat.Format.nChannels ) {
				uint32_t seconds = m_avimHeader.dwTotalFrames * vscal / vrate; // fixme: no video, bad timescale
				uint32_t len = m_avisHeaders[ k ].dwRate * seconds / m_avisHeaders[ k ].dwScale / m_waveFormat.Format.nChannels;
				m_avisHeaders[ k ].dwLength = len;
			}			
		}
		_f.seekp( pos_streamHeader[ k ], ofstream::beg );
		_f.write( (const char *)&m_avisHeaders[ k ], sizeof( avilib::AVISTREAMHEADER ) );
	}

	if( m_RIFF_idx == 0 )
	{
		finish_old();
	}
	else
	{
		// write movi size without final "ixNN"s
		_f.seekp( 0, ofstream::end );
		uint32_t riffAvixMoviSize = (uint32_t)((uint64_t)_f.filepos() - pos_moviListSize - 4);
		_f.seekp( pos_moviListSize, ofstream::beg );
		_f.write( (const char *)&riffAvixMoviSize, 4 );

		_f.seekp( 0, ofstream::end );
		uint32_t riffAvixSize = (uint32_t)((uint64_t)_f.filepos() - pos_RIFFSize - 4);
		
		_f.seekp( pos_RIFFSize, ofstream::beg );
		_f.write( (const char *)&riffAvixSize, 4 );
	}

	reset();
	_f.close();
	return true;
}

void avilib::AviWriter::finish_avix()
{
	uint32_t riffAvixSize, riffAvixMoviSize;
	_f.seekp( 0, ofstream::end );
	riffAvixSize = (uint32_t)((uint64_t)_f.tellp() - pos_RIFFSize - 4);
	riffAvixMoviSize = (uint32_t)((uint64_t)_f.tellp() - pos_moviListSize - 4);

	_f.seekp( pos_RIFFSize, ofstream::beg );
	_f.write( (const char *)&riffAvixSize, 4 );

	_f.seekp( pos_moviListSize, ofstream::beg );
	_f.write( (const char *)&riffAvixMoviSize, 4 );
}


void avilib::AviWriter::finish_old()
{
	for( int32_t stream_idx = 0; stream_idx < 1; stream_idx++ )
	{
		_f.seekp( 0, ofstream::end );
		uint64_t moviSize = (uint64_t)_f.filepos() - pos_moviListSize - 4;
		_f.seekp( pos_moviListSize, ofstream::beg );
		_f.write( (const char *)&moviSize, 4 );
	}

	_f.seekp( 0, ofstream::end );
	_f.clear();
	uint32_t sz = (uint32_t)(m_oldIndexEntries.size() * sizeof( avilib::AVIOLDINDEX ));
	_f.write( "idx1", 4 );
	_f.write((const char *)&sz,4);

	for( avilib::AVIOLDINDEX &entry : m_oldIndexEntries )
		_f.write( (const char *)&entry, sizeof(avilib::AVIOLDINDEX) );
	
	// write file size right after "RIFF"
	_f.seekp( 0, ofstream::end );
	uint32_t s_ = (uint32_t)_f.tellp() - 8;
	_f.seekp( pos_RIFFSize, ofstream::beg );
	_f.write( (const char *)&s_, 4 );
}


bool avilib::AviWriter::write_frame( uint8_t stream_idx, void *data )
{
	return write_data( stream_idx, data, m_bitmapInfo.biSizeImage );
}


bool avilib::AviWriter::write_data( uint8_t stream_idx, void *data, uint32_t len )
{
	if( _f.bad() ) return false;
	char message[32];

	if( m_reservedFrames <= m_totalFrames ) return false;

	FOURCC fourcc = m_streamTypes[stream_idx] == avilib_Video ?
		generate_fcc( "dc", stream_idx ) : generate_fcc( "wb", stream_idx );

	uint32_t ui_next_RIFF_size = m_RIFF_size + len + 8;
	if( ui_next_RIFF_size >= m_currMaxRIFFSize )
	{
		if( m_RIFF_idx == 0 ){
			// write old index
			finish_old();
			m_currMaxRIFFSize = 4ULL * 1000*1000*1000;
		} else {
			finish_avix();
		}

		_f.seekp(0,ofstream::end);

		snprintf( message, 32, "#%u: RIFFSize = %u", m_totalFrames, m_RIFF_size);
		log( message );

		m_RIFF_idx++;
		m_RIFF_size = 24;
		_f.write( "RIFF", 4 );
		pos_RIFFSize = _f.filepos();
		_f.write( (const char *)&NULL32, 4 ); // we'll come back to this later (pos_RIFFSize)
		_f.write( "AVIX", 4 );

		_f.write( "LIST", 4 );
		pos_moviListSize = _f.filepos();
		_f.write( (const char *)&NULL32, 4 ); // later ...
		_f.write( "movi", 4 );
	}

	m_RIFF_size += len + 8;

	if( m_RIFF_idx == 0 )
	{
		m_RIFF_size += sizeof(avilib::AVIOLDINDEX);
		std::streamoff framePosforOldIndex = _f.tellp();
		framePosforOldIndex -= pos_1stMoviStart; // relative to first "movi"

		avilib::AVIOLDINDEX entry;
		entry.dwChunkId = fourcc;
		entry.dwFlags = AVIIF_KEYFRAME; // for PCM Audio as well
		entry.dwSize = len;
		entry.dwOffset = (DWORD)framePosforOldIndex;
		m_oldIndexEntries.push_back( entry );
	}

	if( (m_stdIndexEntries[stream_idx][(int32_t)m_stdIndexes[stream_idx].size()-1].size() + 1U) * sizeof(avilib::AVISTDINDEX_ENTRY) + sizeof(avilib::AVISTDINDEX) >= STDINDEXSIZE )
	{
		avilib::AVISTDINDEX std_index;
		std_index.fcc = stream_idx ? '10xi' : '00xi';
		std_index.wLongsPerEntry = sizeof(avilib::AVISTDINDEX_ENTRY)/sizeof(DWORD);
		std_index.bIndexSubType = 0; // must be 0
		std_index.dwChunkId = fourcc;
		std_index.qwBaseOffset = (uint64_t)_f.filepos(); // all dwOffsets in aIndex array are relative to this
		std_index.dwReserved = 0; // must be 0
		m_stdIndexes[stream_idx].push_back( std_index );
		
		m_currBaseOff = std_index.qwBaseOffset;

		m_superIdxs[stream_idx].nEntriesInUse++;
	}

	avilib::AVISTDINDEX_ENTRY std_entry = { static_cast<DWORD>((uint64_t)_f.tellp() + 8 - m_currBaseOff), len };
	m_stdIndexEntries[stream_idx][(int32_t)m_stdIndexes[stream_idx].size()-1].push_back( std_entry );

	_f.write( (const char *)&fourcc, 4 );
	_f.write( (const char *)&len, 4 );
	_f.write( (const char *)data, len );

	if( m_streamTypes[stream_idx] == avilib_Video ) {
		m_avimHeader.dwTotalFrames++;
		m_avisHeaders[ stream_idx ].dwLength++;
	}
	m_totalFrames++;
	
	return true;
}


bool avilib::AviWriter::setVideoProperties( uint8_t stream_idx, int32_t width, int32_t height, uint32_t codec, uint32_t framesize, double rateHz )
{
	if( m_opened )
		return false;
	
	uint32_t Tmicr = int32_t( .5 + 1000000. / rateHz );

	m_avimHeader.dwMicroSecPerFrame = Tmicr;
	m_avimHeader.dwMaxBytesPerSec = DWORD( .5 + framesize * rateHz );


	memset( &m_avisHeaders[ stream_idx ], 0x0, sizeof( avilib::AVISTREAMHEADER ) );
	AVISTREAMHEADER &aviVideoStreamHeader = m_avisHeaders[ stream_idx ];

	aviVideoStreamHeader.fcc = 'hrts';
	aviVideoStreamHeader.cb = sizeof( avilib::AVISTREAMHEADER ) - 8;
	aviVideoStreamHeader.fccType = 'sdiv';
	aviVideoStreamHeader.fccHandler = BI_RGB;
	aviVideoStreamHeader.dwScale = 1;
	aviVideoStreamHeader.dwQuality = -1;
	
	int32_t rate, scale;
	if( avilib::cancel( int32_t( .5 + 1000. * rateHz ), 1000, rate, scale ) )
	{
		aviVideoStreamHeader.dwScale = scale;
		aviVideoStreamHeader.dwRate = rate;
	}
	else
	{
		aviVideoStreamHeader.dwScale = 1;
		aviVideoStreamHeader.dwRate = uint32_t( .5 + rateHz );
	}

	m_avimHeader.dwMaxBytesPerSec = framesize * uint32_t( .5 + rateHz );
	m_avimHeader.dwSuggestedBufferSize = framesize;
	m_avimHeader.dwWidth = width;
	m_avimHeader.dwHeight = height;

	aviVideoStreamHeader.fccType = 'sdiv';
	aviVideoStreamHeader.fccHandler._ = codec;
	aviVideoStreamHeader.dwSuggestedBufferSize = framesize;
	aviVideoStreamHeader.rcFrame.top =
	aviVideoStreamHeader.rcFrame.left = 0;
	aviVideoStreamHeader.rcFrame.bottom = height;
	aviVideoStreamHeader.rcFrame.right = width;

	m_bitmapInfo.biWidth = width;
	m_bitmapInfo.biHeight = height;
	m_bitmapInfo.biCompression = codec;
	m_bitmapInfo.biSizeImage = framesize;
	
	m_avimHeader.dwStreams++;
	
	m_videoStreamIdx = stream_idx;
	m_streamTypes[ stream_idx ] = avilib_Video;
	return true;
}

bool avilib::AviWriter::setAudioProperties(uint8_t stream_idx, int16_t format, int8_t channels, uint32_t samplesPerSecond, uint32_t avgBytesPerSecond, uint16_t bitsPerSample, uint16_t blockAlign )
{
	if( m_opened )
		return false;

	m_waveFormat.Format.wFormatTag = format;
	m_waveFormat.Format.nChannels = channels;
	m_waveFormat.Format.nSamplesPerSec = samplesPerSecond;
	m_waveFormat.Format.nAvgBytesPerSec = avgBytesPerSecond;
	m_waveFormat.Format.nBlockAlign = blockAlign;
	m_waveFormat.Format.wBitsPerSample = bitsPerSample;
	m_waveFormat.Format.cbSize = 0;

	memset( &m_avisHeaders[ stream_idx ], 0x0, sizeof( avilib::AVISTREAMHEADER ) );
	AVISTREAMHEADER &aviAudioStreamHeader = m_avisHeaders[ stream_idx ];
	aviAudioStreamHeader.fcc = 'hrts';
	aviAudioStreamHeader.cb = sizeof( avilib::AVISTREAMHEADER ) - 8;
	aviAudioStreamHeader.fccType = 'sdua';

	aviAudioStreamHeader.fccType = 'sdua';
	aviAudioStreamHeader.fccHandler._ = 0x0000;
	aviAudioStreamHeader.dwSuggestedBufferSize = avgBytesPerSecond * channels;
	aviAudioStreamHeader.dwRate = samplesPerSecond * bitsPerSample;
	aviAudioStreamHeader.dwSampleSize = bitsPerSample;
	aviAudioStreamHeader.dwScale = bitsPerSample;

	m_waveFormat.Samples.wValidBitsPerSample = bitsPerSample;
	m_waveFormat.Samples.wReserved = 0;
	m_waveFormat.Samples.wSamplesPerBlock = blockAlign;
	m_waveFormat.dwChannelMask = 0x0;
	for( int32_t i=0; i<channels; ++i )
		m_waveFormat.dwChannelMask |= 1<<i;

	m_avimHeader.dwStreams++;

	m_streamTypes[ stream_idx ] = avilib_Audio;
	return true;
}

bool avilib::AviWriter::setFrameCount( uint8_t stream_idx, uint32_t count )
{
	if( m_opened )
		return false;

	m_reservedFrames = count;

	return true;
}


