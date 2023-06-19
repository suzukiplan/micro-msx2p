
namespace avilib
{
	class AviReader : protected LoggingObject
	{
		std::ifstream _f;

		typedef struct _dmlindex {
			FOURCC   dwChunkId;
			uint32_t u32_flags;
			int64_t  i64_offset;
			uint32_t u32_size;
		} DMLINDEX;
			
		// AVI header stuff
		avilib::AVIMAINHEADER m_avimHeader;
		avilib::AVISTREAMHEADER m_avisHeader[2];
		std::map<int32_t, avilib::AVISUPERINDEX> m_superIndex;
		avilib::BITMAPINFO m_bitmapInfo;
		avilib::WAVEFORMATEX m_waveformat;
		avilib::WAVEFORMATEXTENSIBLE m_waveformatExt;
		avilib::MPEG1WAVEFORMAT m_waveformatMpeg12;
		avilib::MPEGLAYER3WAVEFORMAT m_waveformatMP3;

		avilib::ODMLExtendedAVIHeader m_odmlExt;

		std::map<int32_t, std::deque<DMLINDEX>> m_frameIdxs;
		std::map<int32_t, std::deque<DMLINDEX>> odml_frameIdxs;
		
		std::deque<uint64_t> m_moviOffs;
		int32_t m_videoStreamIdx = 0;
		bool m_useMovieOffset = false;

	public:
		AviReader();
		virtual ~AviReader();

		bool open( const char *filename );
		bool close();
		int32_t read_frame( uint32_t idx, uint32_t stream, void *data );
		bool getSize( int32_t &width, int32_t &height );
		bool getFrameRate( double &rateHz );
		bool getFormat( uint32_t stream, avilib_BITMAPINFO &bm_info );
		bool getFormat( uint32_t stream, avilib_WAVEFORMATEX &wav_info );
		bool getFrameCount( uint32_t stream, uint32_t &count );
		bool getCodec( uint32_t stream, uint32_t &codec );
		bool getAllocSize( uint32_t stream, uint32_t &size );
		bool getStreamCount( uint32_t &count );
		bool getStreamType( uint32_t stream, avilib_streamtype_t &type );
	};
}


