// SWF file parser.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define Abs(v)			((v)<0?(-(v)):(v))
#define Max(a,b)		((a)>(b)?(a):(b))
#define Min(a,b)		((a)<(b)?(a):(b))
#define	fix1 			0x00010000L	// fixed 1.0

// Global Types
typedef unsigned long U32, *P_U32, **PP_U32;
typedef signed long S32, *P_S32, **PP_S32;
typedef unsigned short U16, *P_U16, **PP_U16;
typedef signed short S16, *P_S16, **PP_S16;
typedef unsigned char U8, *P_U8, **PP_U8;
typedef signed char S8, *P_S8, **PP_S8;
typedef signed long SFIXED, *P_SFIXED;
typedef signed long SCOORD, *P_SCOORD;
typedef unsigned long BOOL;

typedef struct SPOINT
{
	SCOORD x;
	SCOORD y;
} SPOINT, *P_SPOINT;

typedef struct SRECT 
{
	SCOORD xmin;
	SCOORD xmax;
	SCOORD ymin;
	SCOORD ymax;
} SRECT, *P_SRECT;

typedef struct MATRIX
{
	SFIXED a;
	SFIXED b;
	SFIXED c;
	SFIXED d;
	SCOORD tx;
	SCOORD ty;
} MATRIX, *P_MATRIX;

typedef struct CXFORM
{
	S32 flags;
	enum
	{ 
		needA=0x1,	// Set if we need the multiply terms.
		needB=0x2	// Set if we need the constant terms.
	};
	S16 aa, ab;	// a is multiply factor, b is addition factor
	S16 ra, rb;
	S16 ga, gb;
	S16 ba, bb;
} CXFORM, *P_CXFORM;

#ifndef NULL
#define NULL 0
#endif

// Tag values that may contain URL's.
enum
{ 
	stagEnd 				= 0,
	stagDefineButton 		= 7,
	stagDoAction			= 12,
	stagDefineButton2		= 34,
	stagDefineSprite		= 39
};

// Action codes
enum 
{
	sactionHasLength	= 0x80,
	sactionNone			= 0x00,
	sactionGotoFrame	= 0x81,
	sactionGetURL		= 0x83,
	sactionNextFrame	= 0x04,
	sactionPrevFrame	= 0x05,
	sactionPlay			= 0x06,
	sactionStop			= 0x07,
	sactionToggleQuality= 0x08,
	sactionStopSounds	= 0x09,
	sactionWaitForFrame	= 0x8A,
	sactionSetTarget	= 0x8B,
	sactionGotoLabel	= 0x8C
};

//////////////////////////////////////////////////////////////////////
// Input script object definition.
//////////////////////////////////////////////////////////////////////

struct SValueRecord
{
	char *m_key;
	char *m_value;
	SValueRecord *m_prev;
	SValueRecord *m_next;
};

// An object than substitutes URL information in a Flash file.
struct CUrlSubstitute  
{
	// Input and output file names.
	char *m_inputName;
	char *m_outputName;
	
	// Input and output file pointers.
	FILE *m_inputFile;
	FILE *m_outputFile;

	// State for reading bits.
	S32 m_putBitBuf;
	S32 m_putBitPos;

	// State for writing bits
	S32 m_getBitBuf;
	U32 m_getBitPos;

	// Tag parsing information.
	U32 m_tagStart;
	U32 m_tagEnd;
	U32 m_tagLength;
	U16 m_tagCode;

	// Value list information.
	SValueRecord *m_valueList;
	SValueRecord *m_valueLast;

	// Constructor/destructor.
	CUrlSubstitute();
	~CUrlSubstitute();

	// Input file methods.
	U8 GetByte(void);
	U16 GetWord(void);
	U32 GetDWord(void);
	void GetRect(SRECT *rect);
	void GetMatrix(MATRIX *matrix);
	void GetCxform(CXFORM *cxform, BOOL hasAlpha);
	char *GetString(void);
	void GetData(void *data, U32 len);
	U16 GetTag(void);

	// Output file methods.
	void PutByte(U8 b);
	void PutWord(U16 w);
	void PutDWord(U32 w);
	void PutString(char *str);
	void PutData(void *data, U32 len);
	void PutRect(SRECT *rect);
	void PutMatrix(MATRIX *matrix);
	void PutCxform(CXFORM *cxform, BOOL hasAlpha);

	// File position methods.
	U32 GetGetPos(void);
	void SetGetPos(U32 pos);
	U32 GetPutPos(void);
	void SetPutPos(U32 pos);

	// Bit packing methods.
	S32 CountBits(U32 v);
	S32 CheckMag(S32 v, S32 mag);
	void InitGetBits(void);
	S32 GetSBits(S32 n);
	U32 GetBits(S32 n);
	void InitPutBits(void);
	void FlushPutBits(void);
	void PutBits(S32 v, S32 n);

	// Value list methods.
	void SetValue(char *key, char *value);
	char *GetValue(char *key);
	void FreeAllValues(void);

	// Modification methods.
	void ModifyDoAction(void);
	void ModifyDefineButton(void);
	void ModifyDefineButton2(void);
	void ModifyDefineSprite(void);
	void ModifyNone(void);

	// Application methods.
	void Error(char *errStr, char *argStr);
	void Usage(BOOL verbose);
	BOOL ParseArgs(int argc, char *argv[]);
	BOOL ProcessFile(void);
};


//////////////////////////////////////////////////////////////////////
// Static helper functions.
//////////////////////////////////////////////////////////////////////

static char *AllocStr(char *str)
// Allocates a string on the local heap.
{
	char *heapStr = NULL;
	U32 len = ::strlen(str) + 1;
	
	// Allocate the string.
	heapStr = new char[len];

	// Verify the string was allocated.
	if (heapStr != NULL)
	{
		// Copy the string.
		::strcpy(heapStr, str);
	}

	return heapStr;
}

//////////////////////////////////////////////////////////////////////
// Object methods.
//////////////////////////////////////////////////////////////////////

CUrlSubstitute::CUrlSubstitute(void)
// Class constructor.
{
	// Zero out this object.
	::memset(this, 0, sizeof(CUrlSubstitute));
}


CUrlSubstitute::~CUrlSubstitute(void)
// Class destructor.
{
	// Free all context values.
	FreeAllValues();

	// Do we have an input file handle?
	if (m_inputFile != NULL)
	{
		// Close the file.
		fclose(m_inputFile);
		m_inputFile = NULL;
	}

	// Do we have an output file handle?
	if (m_outputFile != NULL)
	{
		// Close the file.
		fclose(m_outputFile);
		m_outputFile = NULL;
	}
}


U8 CUrlSubstitute::GetByte(void) 
{
	U8 buffer[1];

	// Read the buffer.
	fread(&buffer, sizeof(buffer), 1, m_inputFile);

	// Return the value in the buffer.
	return buffer[0];
}


U16 CUrlSubstitute::GetWord(void)
{
	U8 buffer[2];

	// Read the buffer.
	fread(&buffer, sizeof(buffer), 1, m_inputFile);

	// Return the value in the buffer.
	return (U16) buffer[0] | ((U16) buffer[1] << 8);
}


U32 CUrlSubstitute::GetDWord(void)
{
	U8 buffer[4];

	// Read the buffer.
	fread(&buffer, sizeof(buffer), 1, m_inputFile);

	// Return the value in the buffer.
	return (U32) buffer[0] | ((U32) buffer[1] << 8) | ((U32) buffer[2] << 16) | ((U32) buffer[3] << 24);
}


void CUrlSubstitute::GetRect (SRECT * r)
{
	InitGetBits();
	int nBits = (int) GetBits(5);
	r->xmin = GetSBits(nBits);
	r->xmax = GetSBits(nBits);
	r->ymin = GetSBits(nBits);
	r->ymax = GetSBits(nBits);
}


void CUrlSubstitute::GetMatrix(MATRIX* mat)
{
	InitGetBits();

	// Scale terms
	if (GetBits(1))
	{
		int nBits = (int) GetBits(5);
		mat->a = GetSBits(nBits);
		mat->d = GetSBits(nBits);
	}
	else
	{
	 	mat->a = mat->d = 0x00010000L;
	}

	// Rotate/skew terms
	if (GetBits(1))
	{
		int nBits = (int)GetBits(5);
		mat->b = GetSBits(nBits);
		mat->c = GetSBits(nBits);
	}
	else
	{
	 	mat->b = mat->c = 0;
	}

	// Translate terms
	int nBits = (int) GetBits(5);
	mat->tx = GetSBits(nBits);
	mat->ty = GetSBits(nBits);
}


void CUrlSubstitute::GetCxform(CXFORM* cx, BOOL hasAlpha)
{
	InitGetBits();

	cx->flags = (int) GetBits(2);
	int nBits = (int) GetBits(4);
	cx->aa = 256; cx->ab = 0;
	if (cx->flags & CXFORM::needA)
	{
		cx->ra = (S16) GetSBits(nBits);
		cx->ga = (S16) GetSBits(nBits);
		cx->ba = (S16) GetSBits(nBits);
		if (hasAlpha) cx->aa = (S16) GetSBits(nBits);
	}
	else
	{
		cx->ra = cx->ga = cx->ba = 256;
	}
	if (cx->flags & CXFORM::needB)
	{
		cx->rb = (S16) GetSBits(nBits);
		cx->gb = (S16) GetSBits(nBits);
		cx->bb = (S16) GetSBits(nBits);
		if (hasAlpha) cx->ab = (S16) GetSBits(nBits);
	}
	else
	{
		cx->rb = cx->gb = cx->bb = 0;
	}
}


char *CUrlSubstitute::GetString(void)
// Gets a string and performs any variable substitution.
{
	char ch;
	char var[256];
	char buf[4096];

	int varIndex = 0;
	int bufIndex = 0;

	// Initialize the found var flag.
	BOOL foundVar = false;

	// Read character into the buffer.
	while (ch = (char) GetByte())
	{
		// Are we parsing the a variable.
		if (foundVar)
		{
			// Have we found the end of the variable?
			if (ch != '$')
			{
				// No. Place it into the var buffer.
				if (varIndex < (256 - 1))
				{
					// Place the character into the buffer.
					var[varIndex++] = ch;
				}
			}
			else
			{
				// Null terminate the var index.
				var[varIndex] = 0;

				// Find the value of the variable.
				char *value = GetValue(var);

				// Place the value into the buffer.
				while (*value && (bufIndex < (4096 - 1)))
				{
					// Copy the value into the buffer.
					buf[bufIndex++] = *(value++);
				}
			}
		}
		else
		{
			// Have we found a variable?
			if (ch != '$')
			{
				// No. Make sure we are not going to overflow the buffer.
				if (bufIndex < (4096 - 1))
				{
					// Place the character into the buffer.
					buf[bufIndex++] = ch;
				}
			}
			else
			{
				// Yes.  Set the flag.
				foundVar = true;

				// Reset the var index.
				varIndex = 0;
			}
		}
	}

	// Terminate the buffer.
	buf[bufIndex] = 0;

	// Allocate a string for the buffer on the stack.
	return AllocStr(buf);
}


void CUrlSubstitute::InitGetBits(void)
{
	// Reset the bit position and buffer.
	m_getBitPos = 0;
	m_getBitBuf = 0;
}


S32 CUrlSubstitute::GetSBits (S32 n)
// Get n bits from the string with sign extension.
{
	// Get the number as an unsigned value.
	S32 v = (S32) GetBits(n);

	// Is the number negative?
	if (v & (1L << (n - 1)))
	{
		// Yes. Extend the sign.
		v |= -1L << n;
	}

	return v;
}


U32 CUrlSubstitute::GetBits (S32 n)
// Get n bits from the stream.
{
	U32 v = 0;

	for (;;)
	{
		S32 s = n - m_getBitPos;
		if (s > 0)
		{
			// Consume the entire buffer
			v |= m_getBitBuf << s;
			n -= m_getBitPos;

			// Get the next buffer
			m_getBitBuf = GetByte();
			m_getBitPos = 8;
		}
		else
		{
		 	// Consume a portion of the buffer
			v |= m_getBitBuf >> -s;
			m_getBitPos -= n;
			m_getBitBuf &= 0xff >> (8 - m_getBitPos);	// mask off the consumed bits
			return v;
		}
	}
}


void CUrlSubstitute::GetData(void *data, U32 len)
{
	// Read in the data.
	fread(data, (size_t) len, 1, m_inputFile);
}


U16 CUrlSubstitute::GetTag(void)
{
	// Save the start of the tag.
	m_tagStart = GetGetPos();
	
	// Get the combined code and length of the tag.
	m_tagCode = GetWord();

	// The length is encoded in the tag.
	m_tagLength = (U32) m_tagCode & 0x3f;

	// Remove the length from the code.
	m_tagCode = m_tagCode >> 6;

	// Determine if another long word must be read to get the length.
	if (m_tagLength == 0x3f) m_tagLength = (U32) GetDWord();

	// Determine the end position of the tag.
	m_tagEnd = GetGetPos() + m_tagLength;

	return m_tagCode;
}


void CUrlSubstitute::PutByte(U8 b)
{
	U8 buffer[1];
	
	// Set the data in the buffer.
	buffer[0] = b;

	// Write out the buffer.
	fwrite(&buffer, sizeof(buffer), 1, m_outputFile);
}


void CUrlSubstitute::PutWord(U16 w)
{
	U8 buffer[2];

	// Fill in the local buffer.
	buffer[0] = (U8) (w);
	buffer[1] = (U8) (w >>= 8);
	
	// Write out the buffer.
	fwrite(&buffer, sizeof(buffer), 1, m_outputFile);
}


void CUrlSubstitute::PutDWord(U32 w)
{
	U8 buffer[4];

	// Fill in the local buffer.
	buffer[0] = (U8) (w);
	buffer[1] = (U8) (w >>= 8);
	buffer[2] = (U8) (w >>= 8);
	buffer[3] = (U8) (w >>= 8);
	
	// Write out the word.
	fwrite(&buffer, sizeof(buffer), 1, m_outputFile);
}


void CUrlSubstitute::PutString(char *str)
{
	PutData(str, ::strlen(str) + 1);
}


void CUrlSubstitute::PutData(void *data, U32 len)
{
	// Write out the data.
	fwrite(data, (size_t) len, 1, m_outputFile);
}


void CUrlSubstitute::PutRect(SRECT* r)
{
	InitPutBits();

	S32 mag = CheckMag(r->xmin, CheckMag(r->xmax, CheckMag(r->ymin, Abs(r->ymax))));
	S32 nBits = CountBits(mag) + 1;	// Include a sign bit.

	PutBits(nBits, 5);
	PutBits(r->xmin, nBits);
	PutBits(r->xmax, nBits);
	PutBits(r->ymin, nBits);
	PutBits(r->ymax, nBits);

	FlushPutBits();
}


void CUrlSubstitute::PutMatrix(MATRIX* mat)
// Output the matrix structure.
{
	InitPutBits();

	if (!mat)
	{
		// An Identity matrix
		PutBits(0, 1);	// no scale
		PutBits(0, 1);	// no rot
		PutBits(0, 5);	// no trans
	}
	else
	{
		// Handle scale terms
		BOOL scale = mat->a != fix1 || mat->d != fix1;
		PutBits(scale, 1);
		if (scale)
		{
			S32 nBits = CountBits(CheckMag(mat->a, Abs(mat->d)))+1;	// include a sign bit
			PutBits(nBits, 5);
			PutBits(mat->a, nBits);
			PutBits(mat->d, nBits);
		}

		// Handle rotation/skew terms
		BOOL rot = mat->c != 0 || mat->b != 0;
		PutBits(rot, 1);
		if (rot)
		{
			S32 nBits = CountBits(CheckMag(mat->b, Abs(mat->c)))+1;	// include a sign bit
			PutBits(nBits, 5);
			PutBits(mat->b, nBits);
			PutBits(mat->c, nBits);
		}

		{
			// Handle translation terms
			S32 nBits = CountBits(CheckMag(mat->tx, Abs(mat->ty)));
			if (nBits > 0 ) nBits++;	// include a sign bit if not zero
			PutBits(nBits, 5);
			PutBits(mat->tx, nBits);
			PutBits(mat->ty, nBits);
		}
	}
	FlushPutBits();
}


void CUrlSubstitute::PutCxform(CXFORM* cx, BOOL hasAlpha)
// Output the color transform structure.
{
	S32 mag = 0;
	if (cx->flags & CXFORM::needA)
	{
		mag = CheckMag(cx->ra, mag);
		mag = CheckMag(cx->ga, mag);
		mag = CheckMag(cx->ba, mag);
		if (hasAlpha) mag = CheckMag(cx->aa, mag);
	}
	if (cx->flags & CXFORM::needB )
	{
		mag = CheckMag(cx->rb, mag);
		mag = CheckMag(cx->gb, mag);
		mag = CheckMag(cx->bb, mag);
		if (hasAlpha) mag = CheckMag(cx->ab, mag);
	}
	S32 nBits = CountBits(mag) + 1; // include a sign bit

	InitPutBits();
	PutBits(cx->flags, 2);
	PutBits(nBits, 4);
	if (cx->flags & CXFORM::needA)
	{
		PutBits(cx->ra, nBits);
		PutBits(cx->ga, nBits);
		PutBits(cx->ba, nBits);
		if (hasAlpha) PutBits(cx->aa, nBits);
	}
	if (cx->flags & CXFORM::needB)
	{
		PutBits(cx->rb, nBits);
		PutBits(cx->gb, nBits);
		PutBits(cx->bb, nBits);
		if (hasAlpha) PutBits(cx->ab, nBits);
	}
	FlushPutBits();
}


U32 CUrlSubstitute::GetGetPos(void)
{
	// Get the current position within the input file.
	return (U32) ftell(m_inputFile);
}


void CUrlSubstitute::SetGetPos(U32 pos)
{
	// Set the current position within the input file.
	fseek(m_inputFile, (long) pos, SEEK_SET);
}


U32 CUrlSubstitute::GetPutPos(void)
{
	// Get the current position within the output file.
	return (U32) ftell(m_outputFile);
}


void CUrlSubstitute::SetPutPos(U32 pos)
{
	// Set the current position within the output file.
	fseek(m_outputFile, (long) pos, SEEK_SET);
}


S32 CUrlSubstitute::CountBits(U32 v)
{
	S32 n = 0;
	while (v & ~0xF)
	{
		v >>= 4;
	 	n += 4;
	}
	while (v)
	{
		v >>= 1;
		n++;
	}
	return n;
}


S32 CUrlSubstitute::CheckMag(S32 v, S32 mag)
{
	if (v < 0) v = -v;
	return v > mag ? v : mag;
}


void CUrlSubstitute::InitPutBits(void)
{
	m_putBitPos = 8;
	m_putBitBuf = 0;
}


void CUrlSubstitute::PutBits(S32 v, S32 n)
{		 
	for (;;)
	{
		// Mask off any extra bits.
		v &= (U32) 0xffffffff >> (32 - n);

		// The number of bits more than the buffer will hold
		S32 s = n - m_putBitPos;
		if (s <= 0)
		{
			// This fits in the buffer, add it and finish.
			m_putBitBuf |= v << -s;
			m_putBitPos -= n;	// We used x bits in the buffer.
			return;
		}
		else
		{
			// This fills the buffer, fill the remaining space and try again.
			m_putBitBuf |= v >> s;
			n -= m_putBitPos;	// We placed x bits in the buffer.
			PutByte((U8) m_putBitBuf);
			m_putBitBuf = 0;
			m_putBitPos = 8;
		}
	}
}


void CUrlSubstitute::FlushPutBits(void)
{
	// Put any remaining bits.
	if (m_putBitPos < 8) PutByte((U8) m_putBitBuf);
}


void CUrlSubstitute::SetValue(char *key, char *value)
// Set the context value.
{
	// Point to the start of the list.
	SValueRecord *record = m_valueList;

	// Search for the key in the value list.
	for (BOOL found = false; record && !found; record = found ? record : record->m_next)
	{
		// Determine if the key matches.
		found = ::stricmp(key, record->m_key) == 0;
	}

	// Did we find a record?
	if (found)
	{
		// Yes.  Should we delete the key?
		if (value && ::strlen(value))
		{
			// Allocate a value string.
			char *newValue = AllocStr(value);

			// Did we allocate the string?
			if (newValue)
			{
				// Yes. Update the value.
				if (record->m_value) delete record->m_value;

				// Set the new value.
				record->m_value = newValue;
			}
		}
		else
		{
			// Yes. Remove the record from the list.
			if (record->m_prev) record->m_prev->m_next = record->m_next;
			if (record->m_next) record->m_next->m_prev = record->m_prev;
			if (!record->m_prev) m_valueList = record->m_next;
			if (!record->m_next) m_valueLast = record->m_prev;

			// Delete the key and value strings.
			if (record->m_key) delete record->m_key;
			if (record->m_value) delete record->m_value;

			// Free the record in the chunk allocator.
			delete record;
		}
	}
	else
	{
		// Do we have a value to set?
		if (value && ::strlen(value))
		{
			// Allocate a record.
			SValueRecord *record = new SValueRecord;

			// Do we have a record?
			if (record)
			{
				// Allocate a key and value string.
				record->m_key = AllocStr(key);
				record->m_value = AllocStr(value);
	
				// Did we allocate the strings?
				if (record->m_key && record->m_value)
				{
					// Append the record to the value list.
					record->m_next = m_valueList;
					record->m_prev = NULL;
					if (record->m_next) record->m_next->m_prev = record;
					if (record->m_prev) record->m_prev->m_next = record;
					if (!record->m_prev) m_valueList = record;
					if (!record->m_next) m_valueLast = record;
				}
				else
				{
					// Delete the strings if present.
					if (record->m_key) delete record->m_key;
					if (record->m_value) delete record->m_value;

					// Delete the record.
					delete record;
				}
			}
		}
	}
}


char *CUrlSubstitute::GetValue(char *key)
// Get a pointer to the context value.
{
	// Set the default return value.
	char *value = "";
	
	// Point to the start of the list.
	SValueRecord *record = m_valueList;

	// Search for the key in the value list.
	for (BOOL found = false; record && !found; record = found ? record : record->m_next)
	{
		// Determine if the key matches.
		if (found = (::stricmp(key, record->m_key) == 0))
		{
			// Set the pointer to the value.
			value = record->m_value;
		}
	}

	return value;
}


void CUrlSubstitute::FreeAllValues(void)
// Free all the context values.
{
	// Free all nodes in the text node list.
	SValueRecord *record = m_valueList;
	while (record)
	{
		// Get the next record.
		SValueRecord *nextRecord = record->m_next;
		
		// Delete the key and value strings.
		if (record->m_key) delete record->m_key;
		if (record->m_value) delete record->m_value;

		// Delete the record.
		delete record;

		// Move to the next record.
		record = nextRecord;
	}

	// Reset the value list.
	m_valueList = NULL;
	m_valueLast = NULL;
}

void CUrlSubstitute::ModifyDoAction(void)
// Modify any URL strings within the action.
{
	// Save the tag code.
	U16 tagCode = m_tagCode;
	
	// Get the start of the tag.
	U32 tagStart = GetPutPos();
	
	// Write the code and length seperately.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(0);

	// Get the start of the tag data.
	U32 tagData = GetPutPos();

	// Handle each action.
	U8 action = GetByte();

	// Handle the action.
	while (action)
	{
		// Put the action.
		PutByte(action);

		// Get the length of the action.
		U16 actionLength = (action & 0x80) ? GetWord() : 0;

		// Is this a url action?
		if (action == sactionGetURL)
		{
			// Get the URL string.
			char *url = GetString();

			// Get the window string.
			char *win = GetString();

			// Make sure we have strings.
			if (url && win)
			{
				// Get the length of the URL and window strings.
				actionLength = ::strlen(url) + ::strlen(win) + 2;

				// Put the action length.
				PutWord(actionLength);

				// Put the url and window strings.
				PutString(url);
				PutString(win);
			}
			else
			{
				// Put the action length.
				PutWord(2);

				// Put empty url and window strings.
				PutByte(0);
				PutByte(0);
			}

			// Delete the url and window strings.
			if (url) delete url;
			if (win) delete win;
		}
		else
		{
			// Put the action length.
			if (action & 0x80) PutWord(actionLength);

			// Pass the action unmodified.
			while (actionLength--) PutByte(GetByte());
		}

		// Get the next action.
		action = GetByte();
	}

	// Put the end action.
	PutByte(0);

	// Get the actual end of the tag.
	U32 tagEnd = GetPutPos();

	// Determine the tag length.
	U32 tagLength = tagEnd - tagData;

	// Restore the start tag position.
	SetPutPos(tagStart);

	// Write the tag and length.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(tagLength);

	// Restore the end of tag position.
	SetPutPos(tagEnd);
}


void CUrlSubstitute::ModifyDefineButton(void)
// Modify any URL strings within the button.
{
	// Save the tag code.
	U16 tagCode = m_tagCode;
	
	// Get the start of the tag.
	U32 tagStart = GetPutPos();
	
	// Write the code and length seperately.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(0);

	// Get the start of the tag data.
	U32 tagData = GetPutPos();

	// Handle the button id.
	PutWord(GetWord());

	// Get the first state.
	U8 state = GetByte();

	// Loop until all children parsed.
	while (state)
	{
		MATRIX matrix;

		// Put the state.
		PutByte(state);

		// Handle the id and depth.
		PutWord(GetWord());
		PutWord(GetWord());

		// Handle the matrix.
		GetMatrix(&matrix);
		PutMatrix(&matrix);

		// Get the next state.
		state = GetByte();
	}

	// Put the final state.
	PutByte(0);

	// Handle each action.
	U8 action = GetByte();

	// Handle the action.
	while (action)
	{
		// Put the action.
		PutByte(action);

		// Get the length of the action.
		U16 actionLength = (action & 0x80) ? GetWord() : 0;

		// Is this a url action?
		if (action == sactionGetURL)
		{
			// Get the URL string.
			char *url = GetString();

			// Get the window string.
			char *win = GetString();

			// Make sure we have strings.
			if (url && win)
			{
				// Get the length of the URL and window strings.
				actionLength = ::strlen(url) + ::strlen(win) + 2;

				// Put the action length.
				PutWord(actionLength);

				// Put the url and window strings.
				PutString(url);
				PutString(win);
			}
			else
			{
				// Put the action length.
				PutWord(2);

				// Put empty url and window strings.
				PutByte(0);
				PutByte(0);
			}

			// Delete the url and window strings.
			if (url) delete url;
			if (win) delete win;
		}
		else
		{
			// Put the action length.
			if (action & 0x80) PutWord(actionLength);

			// Pass the action unmodified.
			while (actionLength--) PutByte(GetByte());
		}

		// Get the next action.
		action = GetByte();
	}

	// Put the end action.
	PutByte(0);

	// Get the actual end of the tag.
	U32 tagEnd = GetPutPos();

	// Determine the tag length.
	U32 tagLength = tagEnd - tagData;

	// Restore the start tag position.
	SetPutPos(tagStart);

	// Write the tag and length.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(tagLength);

	// Restore the end of tag position.
	SetPutPos(tagEnd);
}


void CUrlSubstitute::ModifyDefineButton2(void)
// Modify any URL strings within the button.
{
	// Save the tag code.
	U16 tagCode = m_tagCode;
	
	// Get the start of the tag.
	U32 tagStart = GetPutPos();
	
	// Write the code and length seperately.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(0);

	// Get the start of the tag data.
	U32 tagData = GetPutPos();

	// Handle the button id.
	PutWord(GetWord());

	// Handle the track as menu flag.
	PutByte(GetByte());

	// Get the position of the action link.
	U32 actionLink = GetPutPos();
	
	// Get the condition/action pair offset.
	U16 actionOffset = GetWord();
	
	// Put a dummy condition/action pair offset.
	PutWord(0);

	// Get the first state.
	U8 state = GetByte();

	// Loop until all children parsed.
	while (state)
	{
		MATRIX matrix;
		CXFORM cxform;

		// Put the state.
		PutByte(state);

		// Handle the id and depth.
		PutWord(GetWord());
		PutWord(GetWord());

		// Handle the matrix.
		GetMatrix(&matrix);
		PutMatrix(&matrix);

		// Handle the color transform.
		GetCxform(&cxform, true);
		PutCxform(&cxform, true);

		// Get the next state.
		state = GetByte();
	}

	// Put the final state.
	PutByte(0);

	// Loop while we have an action offset.
	while (actionOffset)
	{
		// Get the next action offset.
		actionOffset = GetWord();

		// Save the current position.
		U32 saveLink = GetPutPos();

		// Seek to the link position.
		SetPutPos(actionLink);

		// Write the link out.
		PutWord((U16) (saveLink - actionLink));
				
		// Seek back to the saved position.
		SetPutPos(saveLink);

		// Update the new link position.
		actionLink = saveLink;
		
		// Put a dummy condition/action pair offset.
		PutWord(0);

		// Handle the condition code.
		PutWord(GetWord());

		// Handle each action.
		U8 action = GetByte();

		// Handle the action.
		while (action)
		{
			// Put the action.
			PutByte(action);

			// Get the length of the action.
			U16 actionLength = (action & 0x80) ? GetWord() : 0;

			// Is this a url action?
			if (action == sactionGetURL)
			{
				// Get the URL string.
				char *url = GetString();

				// Get the window string.
				char *win = GetString();

				// Make sure we have strings.
				if (url && win)
				{
					// Get the length of the URL and window strings.
					actionLength = ::strlen(url) + ::strlen(win) + 2;

					// Put the action length.
					PutWord(actionLength);

					// Put the url and window strings.
					PutString(url);
					PutString(win);
				}
				else
				{
					// Put the action length.
					PutWord(2);

					// Put empty url and window strings.
					PutByte(0);
					PutByte(0);
				}

				// Delete the url and window strings.
				if (url) delete url;
				if (win) delete win;
			}
			else
			{
				// Put the action length.
				if (action & 0x80) PutWord(actionLength);

				// Pass the action unmodified.
				while (actionLength--) PutByte(GetByte());
			}

			// Get the next action.
			action = GetByte();
		}

		// Put the end action.
		PutByte(0);
	}

	// Get the actual end of the tag.
	U32 tagEnd = GetPutPos();

	// Determine the tag length.
	U32 tagLength = tagEnd - tagData;

	// Restore the start tag position.
	SetPutPos(tagStart);

	// Write the tag and length.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(tagLength);

	// Restore the end of tag position.
	SetPutPos(tagEnd);
}


void CUrlSubstitute::ModifyDefineSprite(void)
// Modifies actions within a sprite.
{
	// Save the tag code.
	U16 tagCode = m_tagCode;
	
	// Get the start of the tag.
	U32 tagStart = GetPutPos();
	
	// Write the code and length seperately.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(0);

	// Get the start of the tag data.
	U32 tagData = GetPutPos();
	
	// Handle the tag id and frame count.
	PutWord(GetWord());
	PutWord(GetWord());

	// Initialize the end of sprite flag.
	BOOL atEnd = false;

	// Loop through each tag.
	while (!atEnd)
	{
		// Get the current tag information.
		GetTag();

		switch (m_tagCode)
		{
			case stagEnd:
				
				// Pass the tag unmodified.
				ModifyNone();

				// Set the end flag.
				atEnd = true;
				
				break;
		
			case stagDoAction:

				// Modify the do action.
				ModifyDoAction();

				break;

			default:

				// Don't modify this tag.
				ModifyNone();

				break;
		}
	}

	// Get the actual end of the tag.
	U32 tagEnd = GetPutPos();

	// Determine the tag length.
	U32 tagLength = tagEnd - tagData;

	// Restore the start tag position.
	SetPutPos(tagStart);

	// Write the tag and length.
	PutWord((tagCode << 6) | 0x3f);
	PutDWord(tagLength);

	// Restore the end of tag position.
	SetPutPos(tagEnd);
}


void CUrlSubstitute::ModifyNone(void)
// Writes out the tag unmodified.
{
	// Is this tag long?
	if (m_tagLength < 0x3f)
	{
		// No. Write the code and length in the same word.
		PutWord((m_tagCode << 6) | (U16) m_tagLength);
	}
	else
	{
		// Yes. Write the code and length seperately.
		PutWord((m_tagCode << 6) | 0x3f);
		PutDWord(m_tagLength);
	}

	// Get the length of data to copy.
	U32 dataLeft = m_tagLength;

	// Copy the input file data to the output file.
	while (dataLeft)
	{
		U8 data[4096];
	
		// Determine how much data to read.
		U32 count = dataLeft > 4096 ? 4096 : dataLeft;
		
		// Read the input data.
		GetData(data, count);

		// Write the output data.
		PutData(data, count);

		// Adjust the data length.
		dataLeft -= count;
	}
}


BOOL CUrlSubstitute::ProcessFile(void)
// Processes the input file specified for the object.
{
	U8 fileHdr[8];
	BOOL sts = true;

	// Should we open a file or stdin?
	if (::strcmp(m_inputName, "-") != 0)
	{
		// Open the file for reading.
		m_inputFile = fopen(m_inputName, "rb");
	}
	else
	{
		// Open stdin for reading.
		m_inputFile = _fdopen(_fileno(stdin), "rb");
	}

	// Did we open the file?
	if (m_inputFile == NULL) 
	{
		// Error opening file.
		Error("Unable to open input file:", m_inputName);
		Usage(false);
		sts = false;
	}

	// Are we OK?
	if (sts)
	{
		// Read the file header.
		if (fread(fileHdr, 1, 8, m_inputFile) == 8)
		{
			// Verify the header and get the file size.
			if (fileHdr[0] != 'F' || fileHdr[1] != 'W' || fileHdr[2] != 'S' )
			{
				// Bad header in the file.
				Error("Invalid input file:", m_inputName);
				Usage(false);
				sts = false;
			}
		}
		else
		{
			// Unable to read the input file.
			Error("Unable to read input file:", m_inputName);
			Usage(false);
			sts = false;
		}
	}

	// Are we OK?
	if (sts)
	{
		// Should we open a file or stdout?
		if (::strcmp(m_outputName, "-") != 0)
		{
			// Open the output file for writing.
			m_outputFile = fopen(m_outputName, "wb");
		}
		else
		{
			// Open stdout for writing.
			m_outputFile = _fdopen(_fileno(stdout), "wb");
		}

		// Did we open the file?
		if (m_outputFile == NULL) 
		{
			// Error opening file.
			Error("Unable to open output file:", m_outputName);
			Usage(false);
			sts = false;
		}
	}

	// Are we OK?
	if (sts)
	{
		// Write the file header.
		if (fwrite(fileHdr, 1, 8, m_outputFile) != 8)
		{
			// Unable to write the output file.
			Error("Unable to write output file:", m_outputName);
			Usage(false);
			sts = false;
		}
	}

	// Are we OK?
	if (sts)
	{
		SRECT rect;
		
		// Handle the frame rect information.
		GetRect(&rect);
		PutRect(&rect);

		// Handle the frame rate information.
		PutWord(GetWord());

		// Handle the frame count information.
		PutWord(GetWord());

		// Initialize the end of frame flag.
		BOOL atEnd = false;

		// Loop through each tag.
		while (!atEnd)
		{
			// Get the current tag information. 
			GetTag();

			switch (m_tagCode)
			{
				case stagEnd:
					
					// Pass the tag unmodified.
					ModifyNone();

					// Set the end flag.
					atEnd = true;
					
					break;
			
				case stagDoAction:

					// Modify the do action.
					ModifyDoAction();

					break;

				case stagDefineButton:

					// Modify the button.
					ModifyDefineButton();

					break;

				case stagDefineButton2:

					// Modify the button.
					ModifyDefineButton2();

					break;

				case stagDefineSprite:

					// Modify actions within the sprite.
					ModifyDefineSprite();

					break;

				default:

					// Pass the tag unmodified.
					ModifyNone();

					break;
			}
		}
	}

	// Are we OK?
	if (sts)
	{
		// Get the length of the output file.
		U32 fileLen = GetPutPos();

		// Seek to the file length.
		SetPutPos(4);

		// Write the file length.
		PutDWord(fileLen);
	}

	// Do we have an input file handle?
	if (m_inputFile != NULL)
	{
		// Close the file.
		fclose(m_inputFile);
		m_inputFile = NULL;
	}

	// Do we have an output file handle?
	if (m_outputFile != NULL)
	{
		// Close the file.
		fclose(m_outputFile);
		m_outputFile = NULL;
	}

	return sts;
}


void CUrlSubstitute::Error(char *errStr, char *argStr)
// Print error information.
{
	// Is there a argument?
	if (argStr)
	{
		// Print the error and the argument.
		fprintf(stderr, "%s %s\n", errStr, argStr);
	}
	else
	{
		// Print the just the error.
		fprintf(stderr, "%s\n", errStr);
	}
}


void CUrlSubstitute::Usage(BOOL verbose)
// Print usage information.
{
	if (verbose)
	{
		// Output verbose usage.
		fprintf(stderr, "Macromedia Flash URL Substitution Program\n");
		fprintf(stderr, "Copyright (C) Macromedia 1998. All rights reserved.\n");
		fprintf(stderr, "Usage: urlsubst -i <filepath> -o <filepath> [-h] [-v <name> <value>]\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
   		fprintf(stderr, "   -h                  displays usage text\n");
		fprintf(stderr, "   -i <filepath>       specifies input file path ( '-' for stdin)\n");
		fprintf(stderr, "   -o <filepath>       specifies output file path ( '-' for stdout)\n");
		fprintf(stderr, "   -v <name> <value>   specifies a variable for substitution\n");
	}
	else
	{
		// Output condensed usage.
		fprintf(stderr, "urlsubst -i <filepath> -o <filepath> [-h] [-v <name> <value>]\n");
	}
}


BOOL CUrlSubstitute::ParseArgs(int argc, char *argv[])
// Configure the object based on the arguments passed in.
{
	// Are there arguments to process?
	if (argc < 2)
	{
		// Just print the usage.
		Usage(true);
		return false;
	}
	
	// Loop over each argument.
	for (int i = 1; i < argc; ++i)
	{
		if (!::stricmp("-h", argv[i]))
		{
			// Print the verbose usage.
			Usage(true);
			return false;
		}
		else if (!::stricmp("-i", argv[i]))
		{
			// Is there an input argument?
			if (++i < argc)
			{
				// Set the name of the input file.
				m_inputName = argv[i];
			}
			else
			{
				// Print the error, usage and return.
				Error("-i requires an argument", NULL);
				Usage(false);
				return false;
			}
		}
		else if (!::stricmp("-o", argv[i]))
		{
			// Is there an output argument?
			if (++i < argc)
			{
				// Set the name of the input file.
				m_outputName = argv[i];
			}
			else
			{
				// Print the error, usage and return.
				Error("-o requires an argument", NULL);
				Usage(false);
				return false;
			}
		}
		else if (!::stricmp("-v", argv[i]))
		{
			// Is there an output argument?
			if ((i + 2) < argc)
			{
				// Add the variable name and value.
				SetValue(argv[i + 1], argv[i + 2]);

				// Skip the parameters.
				i += 2;
			}
			else
			{
				// Print the error, usage and return.
				Error("-v requires two arguments", NULL);
				Usage(false);
				return false;
			}
		}
		else
		{
			// Unknown argument.
			Error("unknown argument:", argv[i]);
			Usage(false);
			return false;
		}
	}

	// Do we have an input and output name?
	if (!m_inputName || !m_outputName)
	{
		Error("input and output path must be specified", NULL);
		Usage(false);
	}

	return true;
}


int main (int argc, char *argv[])
// Main program.
{
	CUrlSubstitute urlSubst;

	// Parse the arguments.
	BOOL sts = urlSubst.ParseArgs(argc, argv);
	
	// Are we OK?
	if (sts)
	{
		// Process the input.
		sts = urlSubst.ProcessFile();
	}

	return sts ? 0 : 1;
}
