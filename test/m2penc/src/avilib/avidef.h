

#ifndef _WIN32
#define GCC_PACK __attribute__((packed))
#else 
#define GCC_PACK
#pragma pack(push,1)
#endif

namespace avilib
{
	typedef uint32_t DWORD;
	typedef union fourcc_S {
		void operator = (uint32_t i){ _=i;};
		DWORD _;
		char fcc[4];
	} FOURCC;
	typedef uint16_t WORD;
	typedef int32_t LONG;
	typedef uint8_t BYTE;

	typedef struct _avimainheader {
		FOURCC fcc;
		DWORD  cb;
		DWORD  dwMicroSecPerFrame;
		DWORD  dwMaxBytesPerSec;
		DWORD  dwPaddingGranularity;
		DWORD  dwFlags;
		#define AVIF_HASINDEX        0x00000010 // Index at end of file?
		#define AVIF_MUSTUSEINDEX    0x00000020
		#define AVIF_ISINTERLEAVED   0x00000100
		#define AVIF_TRUSTCKTYPE     0x00000800 // Use CKType to find key frames
		#define AVIF_WASCAPTUREFILE  0x00010000
		#define AVIF_COPYRIGHTED     0x00020000
		DWORD  dwTotalFrames;
		DWORD  dwInitialFrames;
		DWORD  dwStreams;
		DWORD  dwSuggestedBufferSize;
		DWORD  dwWidth;
		DWORD  dwHeight;
		DWORD  dwReserved[4];
	} AVIMAINHEADER;


	typedef struct _avistreamheader {
		FOURCC fcc;
		DWORD  cb;
		FOURCC fccType;
		FOURCC fccHandler;
		DWORD  dwFlags;
		#define AVISF_DISABLED          0x00000001
		#define AVISF_VIDEO_PALCHANGES  0x00010000
		WORD   wPriority;
		WORD   wLanguage;
		DWORD  dwInitialFrames;
		DWORD  dwScale;
		DWORD  dwRate;
		DWORD  dwStart;
		DWORD  dwLength;
		DWORD  dwSuggestedBufferSize;
		DWORD  dwQuality;
		DWORD  dwSampleSize;
		struct {
			short int left;
			short int top;
			short int right;
			short int bottom;
		}  rcFrame;
	} AVISTREAMHEADER;


	typedef struct tagBITMAPINFOHEADER {
		//struct tagBITMAPINFO
		//{
			DWORD biSize;         
			LONG  biWidth;			
			LONG  biHeight;			
			WORD  biPlanes;			
			WORD  biBitCount;		
			FOURCC biCompression;	
			#define BI_RGB 0L		
			DWORD biSizeImage;		
			LONG  biXPelsPerMeter;
			LONG  biYPelsPerMeter;
			DWORD biClrUsed;		
			DWORD biClrImportant;
		//} bmInfo;
		//struct tagRGBQUAD {
		//	BYTE rgbBlue;
		//	BYTE rgbGreen;
		//	BYTE rgbRed;
		//	BYTE rgbReserved;
		//} bmiColors[1]; 
	} BITMAPINFO;


	typedef struct {
#define WAVE_FORMAT_EXTENSIBLE 0xfffe
#define WAVE_FORMAT_MPEG       0x0050 /* MPEG Layer 1,2 */
#define WAVE_FORMAT_MPEGLAYER3 0x0055 /* MPEG Layer 3 */
		WORD  wFormatTag;
		WORD  nChannels;
		DWORD nSamplesPerSec;
		DWORD nAvgBytesPerSec;
		WORD  nBlockAlign;
		WORD  wBitsPerSample;
		WORD  cbSize;
	} WAVEFORMATEX;

	typedef struct {
		WAVEFORMATEX Format;
		union {
			WORD wValidBitsPerSample;
			WORD wSamplesPerBlock;
			WORD wReserved;
		} Samples;
		DWORD        dwChannelMask;
		uint8_t  SubFormat[16];
	} WAVEFORMATEXTENSIBLE;

	typedef struct mpeg1waveformat_tag {
		WAVEFORMATEX wfx;
		WORD         fwHeadLayer;
		DWORD        dwHeadBitrate;
		WORD         fwHeadMode;
		WORD         fwHeadModeExt;
		WORD         wHeadEmphasis;
		WORD         fwHeadFlags;
		DWORD        dwPTSLow;
		DWORD        dwPTSHigh;
	} MPEG1WAVEFORMAT;

	typedef struct mpeglayer3waveformat_tag {
		WAVEFORMATEX wfx;
		WORD         wID;
		DWORD        fdwFlags;
		WORD         nBlockSize;
		WORD         nFramesPerBlock;
		WORD         nCodecDelay;
	} MPEGLAYER3WAVEFORMAT;

	typedef struct {
		DWORD dwTotalFrames;
	} ODMLExtendedAVIHeader;


	typedef struct _avioldindex {
		FOURCC  dwChunkId;
		DWORD   dwFlags;
		#define AVIIF_LIST       0x00000001
		#define AVIIF_KEYFRAME   0x00000010
		#define AVIIF_NO_TIME    0x00000100
		DWORD   dwOffset;
		DWORD   dwSize;
	} AVIOLDINDEX;

#define AVI_INDEX_OF_INDEXES       0x00
#define AVI_INDEX_OF_CHUNKS        0x01
#define AVI_INDEX_OF_TIMED_CHUNKS  0x02
#define AVI_INDEX_OF_SUB_2FIELD    0x03
#define AVI_INDEX_IS_DATA          0x80

	// index subtype codes
	//
#define AVI_INDEX_SUB_DEFAULT     0x00

	// INDEX_OF_CHUNKS subtype codes
	//
#define AVI_INDEX_SUB_2FIELD      0x01


	typedef struct _avistdindex_chunk {
		FOURCC fcc;            // "ix##"
		DWORD cb;
		WORD wLongsPerEntry;   // must be sizeof(aIndex[0])/sizeof(DWORD)
		BYTE bIndexSubType;    // must be 0
		BYTE bIndexType;       // must be AVI_INDEX_OF_CHUNKS
		DWORD nEntriesInUse;   //
		FOURCC dwChunkId;      // "##dc" or "##db" or "##wb", etc.
		uint64_t qwBaseOffset; // all dwOffsets in aIndex array are relative to this
		DWORD dwReserved;      // must be 0
	} GCC_PACK AVISTDINDEX;
	typedef struct _avistdindex_entry {
		DWORD dwOffset;        // qwBaseOffset + this is absolute file offset
		DWORD dwSize;          // bit 31 is set if this is NOT a keyframe
	} AVISTDINDEX_ENTRY;

	typedef struct _avisuperindex_chunk {
		FOURCC fcc;            // 'xdni'
		DWORD cb;              // size of this structure
		WORD wLongsPerEntry;   // must be 4 (size of each entry in aIndex array)
		BYTE bIndexSubType;    // must be 0 or AVI_INDEX_2FIELD
		BYTE bIndexType;       // must be AVI_INDEX_OF_INDEXES
		DWORD nEntriesInUse;   // number of entries in aIndex array that are used
		FOURCC dwChunkId;      // "##dc" or "##db" or "##wb", etc.
		DWORD dwReserved[3];   // must be 0
		
	} GCC_PACK AVISUPERINDEX;
	typedef struct _avisuperindex_entry {
		uint64_t qwOffset;     // absolute file offset, offset 0 is unused entry??
		DWORD dwSize;          // size of index chunk at this offset
		DWORD dwDuration;      // time span in stream ticks
	} AVISUPERINDEX_ENTRY;


	typedef struct VIDEO_PROP_HEADER_S {
		typedef enum {FORMAT_UNKNOWN, FORMAT_PAL_SQUARE, FORMAT_PAL_CCIR_601,
			FORMAT_NTSC_SQUARE, FORMAT_NTSC_CCIR_601} video_format_t;
		DWORD VideoFormatToken;
		typedef enum {STANDARD_UNKNOWN, STANDARD_PAL, STANDARD_NTSC, STANDARD_SECAM} video_standard_t;
		DWORD VideoStandard;
		DWORD dwVerticalRefreshRate;
		DWORD dwHTotalInT;
		DWORD dwVTotalInLines;
		DWORD dwFrameAspectRatio;
		DWORD dwFrameWidthInPixels;
		DWORD dwFrameHeightInLines;
		DWORD nbFieldPerFrame;
		typedef struct {
	   		DWORD CompressedBMHeight;
   			DWORD CompressedBMWidth;
  			DWORD ValidBMHeight;
  			DWORD ValidBMWidth;
			DWORD ValidBMXOffset;
			DWORD ValidBMYOffset;
			DWORD VideoXOffsetInT;
   			DWORD VideoYValidStartLine;
		} VIDEO_FIELD_DESC;
		// VIDEO_FIELD_DESC FieldInfo[nbFieldPerFrame];
	} VIDEO_PROP_HEADER;
}

#ifdef _WIN32
#pragma pack(pop)
#endif


