
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

struct ScriptTypes
{
	const char *TypeName;
	int TypeBase;
	int TypeCount;
};


// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void PA_Parse();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

extern int pa_ScriptCount;
extern struct ScriptTypes *pa_TypedScriptCounts;
extern int pa_MapVarCount;
extern int pa_WorldVarCount;
extern int pa_GlobalVarCount;
extern int pa_WorldArrayCount;
extern int pa_GlobalArrayCount;
extern enum ImportModes ImportMode;
extern bool ExporterFlagged;
extern bool pa_ConstExprIsString;
extern int pa_CurrentDepth;			// Current statement depth
extern int pa_FileDepth;			// Outermost level in the current file