//**************************************************************************
//**
//** misc.h
//**
//**************************************************************************

#pragma once

// HEADER FILES ------------------------------------------------------------

#include "error.h"

// MACROS ------------------------------------------------------------------

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
void MS_LoadFile(const string &name, vector<char>& DataReference);
bool MS_FileExists(const string &name);
bool MS_SaveFile(const string &name, vector<char>& DataReference);
void MS_SuggestFileExt(string &base, string&& extension);
void MS_StripFileExt(string &name);
bool MS_StripFilename(string &path);

//TODO: MS_Message Declaration: Nasty nasty stuff...
void MS_Message(msg_t msg, const string text);
template <class type> void MS_Message(msg_t msg, const string text, const type info);
template <class type, class type2> void MS_Message(msg_t msg, const string text, const type info, const type2 info2);
template <class type, class type2, class type3> void MS_Message(msg_t msg, const string text, const type info, const type2 info2, const type3 info3);
template <class type, class type2, class type3, class type4> void MS_Message(msg_t msg, const string text, const type info, const type2 info2, const type3 info3, const type4 info4);

bool MS_IsPathAbsolute(const string & name);
bool MS_IsDirectoryDelimiter(char test);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

#ifdef _MSC_VER
// Get rid of the annoying deprecation warnings with VC++2005 and newer.
#pragma warning(disable:4996)
#endif
