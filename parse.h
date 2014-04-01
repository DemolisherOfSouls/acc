//**************************************************************************
//**
//** parse.h
//**
//**************************************************************************

#pragma once

// HEADER FILES ------------------------------------------------------------

#include "error.h"
#include "token.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct ScriptType
{
	const string TypeName;
	int TypeBase;
	int TypeCount;
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void PA_Parse();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

int pa_ScriptCount;
ScriptType *pa_TypedScriptCounts;
int pa_MapVarCount;
int pa_WorldVarCount;
int pa_GlobalVarCount;
int pa_WorldArrayCount;
int pa_GlobalArrayCount;
ImportModes ImportMode;
bool ExporterFlagged;
bool pa_ConstExprIsString;
ACS_File *currentFile;
DepthVal pa_CurrentDepth;			// Current statement depth
DepthVal pa_FileDepth;				// Outermost level in the current file