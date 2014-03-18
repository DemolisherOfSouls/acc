
//**************************************************************************
//**
//** misc.h
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "error.h"

// MACROS ------------------------------------------------------------------

#ifdef _WIN32
#define strcasecmp stricmp
#endif

// TYPES -------------------------------------------------------------------

enum msg_t : int
{
	MSG_NORMAL,
	MSG_VERBOSE,
	MSG_DEBUG
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

short MS_LittleUWORD(short val);
int MS_LittleUINT(int val);
vector<char> MS_LoadFile(string name);
bool MS_FileExists(string name);
bool MS_SaveFile(string name, char *buffer, int length);
string MS_StrLwr(string s);
void MS_SuggestFileExt(string &base, string extension);
void MS_StripFileExt(string &name);
bool MS_StripFilename(string &path);
void MS_Message(msg_t type, string text, ...);
bool MS_IsPathAbsolute(string name);
bool MS_IsDirectoryDelimiter(char test);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

#ifdef _MSC_VER
// Get rid of the annoying deprecation warnings with VC++2005 and newer.
#pragma warning(disable:4996)
#endif
