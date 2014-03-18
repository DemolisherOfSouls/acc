
//**************************************************************************
//**
//** symbol.h
//**
//**************************************************************************

#pragma once

// HEADER FILES ------------------------------------------------------------

#include "common.h"
#include "pcode.h"

// MACROS ------------------------------------------------------------------

#define MAX_ARRAY_DIMS 8

// TYPES -------------------------------------------------------------------

enum symbolType_t : int
{
	SY_DUMMY,
	SY_LABEL,
	SY_SCRIPTVAR,
	SY_SCRIPTALIAS,
	SY_MAPVAR,
	SY_WORLDVAR,
	SY_GLOBALVAR,
	SY_MAPARRAY,
	SY_WORLDARRAY,
	SY_GLOBALARRAY,
	SY_SPECIAL,
	SY_CONSTANT,
	SY_INTERNFUNC,
	SY_SCRIPTFUNC
};

struct symVar_t
{
	byte index;
	int vartype;
};

struct symArray_t
{
	byte index;
	int dimensions[MAX_ARRAY_DIMS];
	int ndim;
	int size;
};

struct symStruct_t
{
	byte index;
	int vartype;
};

struct symLabel_t
{
	int address;
};

struct symSpecial_t
{
	int value;
	int argCount;
};

struct symConstant_t
{
	int value;
	int fileDepth;
};

struct symInternFunc_t
{
	pcd_t directCommand;
	pcd_t stackCommand;
	int argCount;
	int optMask;
	int outMask;
	bool hasReturnValue;
	bool latent;
};

struct symScriptFunc_t
{
	int address;
	int argCount;
	int varCount;
	int funcNumber;
	bool hasReturnValue;
	int sourceLine;
	char *sourceName;
	bool predefined;
};

struct symbolNode_t
{
	struct symbolNode_t *left;
	struct symbolNode_t *right;
	char *name;
	symbolType_t type;
	bool unused;
	bool imported;
	union
	{
		symVar_t var;
		symArray_t array;
		symLabel_t label;
		symSpecial_t special;
		symConstant_t constant;
		symInternFunc_t internFunc;
		symScriptFunc_t scriptFunc;
		symStruct_t a_struct;
	} info;
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void SY_Init();
symbolNode_t *SY_Find(char *name);
symbolNode_t *SY_FindLocal(char *name);
symbolNode_t *SY_FindGlobal(char *name);
symbolNode_t *SY_InsertLocal(char *name, symbolType_t type);
symbolNode_t *SY_InsertGlobal(char *name, symbolType_t type);
symbolNode_t *SY_InsertGlobalUnique(char *name, symbolType_t type);
void SY_FreeLocals();
void SY_FreeGlobals();
void SY_FreeConstants(int depth);
void SY_ClearShared();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

