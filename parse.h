
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

static const enum
{
	VT_BOOL = 0,
	VT_INT,
	VT_FIXED,
	VT_STRING,
	VT_TID,
	VT_ACTOR,
	VT_CLASS,
	VT_CUSTOM
};

// TYPES -------------------------------------------------------------------

struct ScriptTypes
{
	const char *TypeName;
	int TypeBase;
	int TypeCount;
};

//==========================================================================
// ACS_MapVar
//
// Variable defined at the map level or lower, with map level being 0.
//
//==========================================================================

struct ACS_MapVar
{
	int index;
	int type;
	char *name;
	bool isConstant;
	bool isStatic;
	bool isDeleted;
	bool isImported;
	bool isString;

	//Every set of braces for every if statement, or script, or function
	//has a unique depthindex, this is so you can call 'int c = 0' in a
	//for loop, and then call 'int c = 3' after it, without a
	//redefinition error.

	int depthIndex;

	static int nextIndex;
	static int nextDepthIndex;

	ACS_MapVar()
	{
		index = -1; // Undefined
	}

	ACS_MapVar(char *name, int type = VT_INT, int depthIndex = 0)
	{
		index = nextIndex++;
		this->type = type;
		this->name = name;
		isConstant = false;
		isStatic = false;
		isDeleted = false;
		isImported = false;
		isString = false;

		this->depthIndex = depthIndex;
	}

	bool Delete()
	{
		if (isDeleted)
		{
			ERR_Error(ERR_ALREADY_DELETED, false, name, tk_Line);
			return false;
		}
		isDeleted = true;
		return true;
	}

	bool TestInput(int type = VT_INT)
	{

	}
};

int ACS_MapVar::nextIndex = 0;
ACS_MapVar ACS_MapVarList[MAX_MAP_VARIABLES];

#pragma region ACS_LocalVar

//==========================================================================
// ACS_LocalVar
//==========================================================================

struct ACS_LocalVar
{
	int index;
	char *name;
	bool isConstant;
	bool isStatic;
	bool isDeleted;

	//Every set of braces for every if statement, or script, or function
	//has a unique depthindex, this is so you can call 'int c = 0' in a
	//for loop, and then call 'int c = 3' after it, without a
	//redefinition error.

	int depthIndex;

	static int nextIndex;
	static int nextDepthIndex;

	ACS_LocalVar()
	{
		index = -1; // Undefined
	}

	ACS_LocalVar(char *name)
	{
		this->name = name;
		index = nextIndex++;
	}


};

int ACS_LocalVar::nextIndex = 0;
int ACS_LocalVar::nextDepthIndex = 0;

ACS_LocalVar ACS_LocalVarList[MAX_MAP_VARIABLES];

#pragma endregion

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