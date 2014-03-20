
//**************************************************************************
//**
//** misc.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#ifdef __NeXT__
#include <libc.h>
#else
#include <cstdlib>
#endif
#ifdef __GNUC__
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include <sys/stat.h>
#endif
#include <iostream>
#include <fstream>
#include "common.h"
#include "misc.h"
#include "error.h"

using std::fstream;
using std::ios;

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern bool acs_BigEndianHost;
extern bool acs_VerboseMode;
extern bool acs_DebugMode;
extern fstream acs_DebugFile;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// MS_LittleUWORD
//
// Converts a host short (2 bytes) to little endian byte order.
//
//==========================================================================
short MS_LittleUWORD(short val)
{
	if (!acs_BigEndianHost)
		return val;
	
	return ((val&255u)<<8u)+((val>>8u)&255u);
}

//==========================================================================
//
// MS_LittleUINT
//
// Converts a host int (4 bytes) to little endian byte order.
//
//==========================================================================
int MS_LittleUINT(int val)
{
	if (!acs_BigEndianHost)
		return val;
	
	return ((val&255u)<<24u)+(((val>>8u)&255u)<<16u)+(((val>>16u)&255u)<<8u)
		+((val>>24u)&255u);
}

//==========================================================================
//
// MS_LoadFile
//
//==========================================================================
void MS_LoadFile(string name, vector<char>& DataReference)
{
	if (name.length() >= MAX_FILE_NAME_LENGTH)
		ERR_Exit(ERR_FILE_NAME_TOO_LONG, false, name);

	fstream file = fstream(name, ios::binary | ios::in);

	if (!file.is_open())
		ERR_Exit(ERR_CANT_OPEN_FILE, false, name);

	struct stat fileInfo;
	int size = fileInfo.st_size;

	DataReference.resize(size);

	file.read(DataReference.data(), size);

	if (file.fail())
		ERR_Exit(ERR_CANT_READ_FILE, false, name);

	file.close();
}

//==========================================================================
//
// MS_FileExists
//
// Pascal 21/11/08
//
//==========================================================================
bool MS_FileExists(string name)
{
	struct stat info;
	int ret = stat(name.c_str(), &info);

	return (ret == 0);
}

//==========================================================================
//
// MS_SaveFile
//
//==========================================================================
bool MS_SaveFile(string name, vector<char>& DataReference)
{
	fstream file = fstream(name, ios::binary | ios::out | ios::trunc);
	
	if (!file.is_open())
		return false;

	file.write(DataReference.data(), DataReference.size());
	file.flush();
	file.close();
	
	if(file.fail())
		return false;

	return true;
}

//==========================================================================
//
// MS_SuggestFileExt
//
//==========================================================================
void MS_SuggestFileExt(string &base, string &extension)
{
	int c = base.length();
	
	while(!MS_IsDirectoryDelimiter(base[--c]) && c > 0)
	{
		if(base[c] == '.')
			return;
	}
	base.append(extension);
}

//==========================================================================
//
// MS_IsDirectoryDelimiter
//
//==========================================================================
bool MS_IsDirectoryDelimiter(char foo)
{
#if defined(_WIN32) || defined(__MSDOS__)
	return foo == '/' || foo == '\\' || foo == ':';
#else
	return foo == '/';
#endif
}

//==========================================================================
//
// MS_StripFileExt
//
//==========================================================================
void MS_StripFileExt(string &name)
{
	int c = name.length();

	while(!MS_IsDirectoryDelimiter(name[--c]) && c > 0)
	{
		if(name[c] == '.')
		{
			name = name.substr(0, c + 1);
			return;
		}
	}
}

//==========================================================================
//
// MS_StripFilename
//
// [RH] This now leaves the directory delimiter in place.
//
//==========================================================================
bool MS_StripFilename(string &name)
{
	int c = name.length();

	while (!MS_IsDirectoryDelimiter(name[--c]))
	{
		if(c == 0)
		{ // No directory delimiter
			return false;
		}
	}
	name = name.substr(0, c + 1);
	return true;
}

//==========================================================================
//
// MS_Message [JRT]
//
// This mess is priceless. I'm so proud :,)
//
//==========================================================================
#define MS_MSG_INIT(msg) if (msg == MSG_VERBOSE && !acs_VerboseMode || msg == MSG_DEBUG && !acs_DebugMode) return;
#define MS_MSG_WRITE(msg,data) { if (msg == MSG_DEBUG && acs_DebugFile.is_open) { acs_DebugFile << data; } else { std::cout << data; } }
#define MS_MSG_CLOSE(msg) { if(msg == MSG_DEBUG && acs_DebugFile.is_open) { acs_DebugFile << std::endl; acs_DebugFile.flush(); acs_DebugFile.close(); } else { std::cout << std::endl; } }

void MS_Message(msg_t msg, const string text)
{
	MS_MSG_INIT(msg);
	MS_MSG_WRITE(msg, text);
	MS_MSG_CLOSE(msg);
}

template <class type>
void MS_Message(msg_t msg, const string text, const type info)
{
	MS_MSG_INIT(msg);
	MS_MSG_WRITE(msg, text);
	MS_MSG_WRITE(msg, info);
	MS_MSG_CLOSE(msg);
}

template <class type, class type2>
void MS_Message(msg_t msg, const string text, const type info, const type2 info2)
{
	MS_MSG_INIT(msg);
	MS_MSG_CHECK(msg);
	MS_MSG_WRITE(msg, text);
	MS_MSG_WRITE(msg, info);
	MS_MSG_WRITE(msg, info2);
	MS_MSG_CLOSE(msg);
}

template <class type, class type2, class type3>
void MS_Message(msg_t msg, const string text, const type info, const type2 info2, const type3 info3)
{
	MS_MSG_INIT(msg);
	MS_MSG_CHECK(msg);
	MS_MSG_WRITE(msg, text);
	MS_MSG_WRITE(msg, info);
	MS_MSG_WRITE(msg, info2);
	MS_MSG_WRITE(msg, info3);
	MS_MSG_CLOSE(msg);
}

template <class type, class type2, class type3, class type4>
void MS_Message(msg_t msg, const string text, const type info, const type2 info2, const type3 info3, const type4 info4)
{
	MS_MSG_INIT(msg);
	MS_MSG_CHECK(msg);
	MS_MSG_WRITE(msg, text);
	MS_MSG_WRITE(msg, info);
	MS_MSG_WRITE(msg, info2);
	MS_MSG_WRITE(msg, info3);
	MS_MSG_WRITE(msg, info4);
	MS_MSG_CLOSE(msg);
}

//==========================================================================
//
// MS_IsPathAbsolute
//
// Pascal 30/11/08
//
//==========================================================================
bool MS_IsPathAbsolute(string name)
{
#if defined(_WIN32) || defined(__MSDOS__)
	// In Windows, the second character must be : if it is an
	// absolute path (the first character indicates the drive)
	// or the first character is either / or \ for absolute path
	if((name[0] != '\0') && (name[1] == ':'))
		return true;
#endif
	// In Unix-land, the first character must be / for a root path
	return MS_IsDirectoryDelimiter(name[0]);
}
