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
int STR_Find(string name);
void STR_WriteStrings();
void STR_WriteList();
int STR_FindLanguage(string name);
int STR_FindInLanguage(int language, string name);
int STR_FindInList(int list, string name);
int STR_FindInListInsensitive(int list, string name);
int STR_AppendToList(int list, string name);
const string& STR_GetString(int list, int index);
void STR_WriteChunk(int language, bool encrypt);
void STR_WriteListChunk(int list, int id, bool quad);
int STR_ListSize(int list);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

extern int NumLanguages, NumStringLists;
