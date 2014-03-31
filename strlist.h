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

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void STR_Init();
int STR_Find(const string& name);
void STR_WriteStrings();
void STR_WriteList();
int STR_FindLanguage(const string& name);
int STR_FindInLanguage(int language, const string& name);
int STR_FindInList(StringListType list, const string& name);
int STR_FindInListInsensitive(StringListType list, const string& name);
int STR_AppendToList(StringListType list, const string& name);
const string& STR_GetString(StringListType list, int index);
void STR_WriteChunk(int language, bool encrypt);
void STR_WriteListChunk(StringListType list, int id, bool quad);
int STR_ListSize(StringListType list);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

int NumLanguages;
int NumStringLists;