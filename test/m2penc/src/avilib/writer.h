
namespace avilib
{
	class AviWriter : protected LoggingObject
	{
		typedef int32_t stream_id;

		std::ofstream _f;
		uint64_t pos_RIFFSize;
		uint64_t pos_moviListSize;
		std::streamoff pos_odmlExt;
		std::streamoff pos_1stMoviStart;
		std::streamoff pos_aviMainHeader;
		std::map<stream_id, std::streamoff> pos_odmlSuperIdx;
		std::map<stream_id, std::streamoff> pos_streamHeader;

		std::map<stream_id, avilib::AVISUPERINDEX> m_superIdxs;
		std::map<stream_id, std::vector<avilib::AVISTDINDEX>> m_stdIndexes;
		std::map<stream_id, std::map<int32_t, std::deque<avilib::AVISTDINDEX_ENTRY>>> m_stdIndexEntries;
		std::map<stream_id, avilib_streamtype_t> m_streamTypes;
		
		std::deque<avilib::AVIOLDINDEX> m_oldIndexEntries;

		avilib::AVIMAINHEADER m_avimHeader;
		std::map<int,avilib::AVISTREAMHEADER> m_avisHeaders;
		avilib::BITMAPINFO m_bitmapInfo;
		avilib::WAVEFORMATEXTENSIBLE m_waveFormat;


		bool m_openDML = true;
		bool m_opened = false;
		uint32_t m_videoStreamIdx = ~0u;
		uint32_t m_reservedFrames = 0;
		uint32_t m_RIFF_idx = 0;  // nth RIFF section (OpenDML)
		uint32_t m_RIFF_size = 0; // current RIFF size (OpenDML)
		uint32_t m_totalFrames = 0;
		uint64_t m_currBaseOff = 0;
		uint64_t m_currMaxRIFFSize = 1000*1000*1000;

		void finish_old();
		void finish_avix();
		void reset();

	public:
		AviWriter();
		virtual ~AviWriter();

		bool setVideoProperties( uint8_t stream_idx, int32_t width, int32_t height, uint32_t codec, uint32_t framesize, double rateHz );
		bool setAudioProperties( uint8_t stream_idx, int16_t format, int8_t channels, uint32_t samplesPerSecond, uint32_t avgBytesPerSecond, uint16_t bitsPerSample, uint16_t blockAlign );
		bool setFrameCount( uint8_t stream_idx, uint32_t count );

		bool open( const char *filename );
		bool write_frame( uint8_t stream_idx, void *data );
		bool write_data( uint8_t stream_idx, void *data, uint32_t len );
		bool close();
		bool openDML( bool yes );
	};

}


