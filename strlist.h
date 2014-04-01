//**************************************************************************
//**
//** strlist.h
//**
//**************************************************************************

#pragma once

// HEADER FILES ------------------------------------------------------------

#include "common.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

enum StringListType : int
{
	STRLIST_PICS,
	STRLIST_FUNCTIONS,
	STRLIST_MAPVARS,
	STRLIST_NAMEDSCRIPTS,
	STRLIST_STRUCTS,
	STRLIST_METHODS,

	NUM_STRLISTS
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void STR_Init();
int STR_Find(string name);
void STR_WriteStrings();
void STR_WriteList();
int STR_FindLanguage(string name);
int STR_FindInLanguage(int language, string name);
int STR_FindInList(StringListType list, string name);
int STR_FindInListInsensitive(StringListType list, string name);
int STR_AppendToList(StringListType list, string name);
string* STR_GetString(StringListType list, int index);
void STR_WriteChunk(int language, bool encrypt);
void STR_WriteListChunk(StringListType list, int id, bool quad);
int STR_ListSize(StringListType list);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

int NumLanguages;
int NumStringLists;