
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

// Base Class for all temporary objects
struct ACS_Var
{
	string name;	// Name of object in source
	int index;		// Index in main list
	int type;		// Type of variable / Return type
	int depth;		// Depth - 0 = map, 1 = innerscript, etc.

	bool isDeleted;	// Has 'delete' been called on it?
	bool isDeclared;// Is it declared yet?
	bool isAssigned;// Has a value been assigned to it?

	ACS_Var()
	{
		index = -1; // Undefined
	}
	ACS_Var(string name, int type, int depth)
	{
		index = nextVarIndex++;
		this->type = type;
		this->name = name;
		isConstant = false;
		isStatic = false;
		isDeleted = false;
		isImported = false;

		this->depthIndex = depthIndex;
	}
};

struct ACS_Const : public ACS_Var
{
	int value;
};

struct ACS_Special : public ACS_Const
{
	vector<int> argTypes;
};

struct ACS_Function : public ACS_Var
{
	vector<int> argTypes;
};

struct ACS_Array : public ACS_Var
{
	int dimensions[MAX_ARRAY_DIMS];
	int size;
};

struct ACS_Struct : public ACS_Var
{
	int size;
	vector<int> members;
	vector<int> methods;
	vector<int> constructors;
	vector<int> operators;
};

struct symArray_t
{
	byte index;
	int dimensions[MAX_ARRAY_DIMS];
	int ndim;
	int size;
};

struct symLabel_t
{
	int address;
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
	string name;
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
symbolNode_t *SY_Find(string name);
symbolNode_t *SY_FindLocal(string name);
symbolNode_t *SY_FindGlobal(string name);
symbolNode_t *SY_InsertLocal(string name, symbolType_t type);
symbolNode_t *SY_InsertGlobal(string name, symbolType_t type);
symbolNode_t *SY_InsertGlobalUnique(string name, symbolType_t type);
void SY_FreeLocals();
void SY_FreeGlobals();
void SY_FreeConstants(int depth);
void SY_ClearShared();

// PUBLIC DATA DECLARATIONS ------------------------------------------------
