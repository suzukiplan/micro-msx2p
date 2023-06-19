
namespace avilib
{
	int32_t gcd( int32_t nom, int32_t denom );
	bool cancel( int32_t nom, int32_t denom, int32_t &out_nom, int32_t &out_denom );
	
	class LoggingObject
	{
		std::deque<std::string> m_messages;

	protected:
		void log( const std::string & );
		void clear_log();

	public:
		bool getNextMessage( std::string &message );
	};

	avilib::FOURCC generate_fcc( std::string id, uint8_t idx );
}

