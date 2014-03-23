
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

#define MAX_ARRAY_DIMS 8		// Seems reasonable
#define MAX_DEFINED_TYPES 256	// Will increase if necessary

#define VAR_NULL		0
#define VAR_INT			1
#define VAR_BOOL		2
#define VAR_STR			3

#define VAR_INTERNAL	4

// TYPES -------------------------------------------------------------------

struct ACS_Object;
struct ACS_Var;
struct ACS_Const;
struct ACS_Function;
struct ACS_Struct;
struct ACS_StructArray;
struct ACS_Array;
struct ACS_TypeDef;
struct ACS_VarRef;
struct ACS_FuncCall;
struct ACS_Node;
struct ACS_DepthRoot;

using ConstList = vector<ACS_Const>;
using VarList = vector<ACS_Var>;
using VarRefList = vector<ACS_VarRef>;
using FunctList = vector<ACS_Function>;
using TypeList = vector<ACS_TypeDef>;
using ArrayList = vector<ACS_Array>;
using FunCallList = vector<ACS_FuncCall>;
using DepthList = vector<ACS_DepthRoot>;
using NodeList = vector<ACS_Node>;

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

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void sym_Init();
ACS_Node *SY_Find(string name);
ACS_Node *SY_FindLocal(string name);
ACS_Node *SY_FindGlobal(string name);
ACS_Node *SY_InsertLocal(string name, int type);
ACS_Node *SY_InsertGlobal(string name, int type);
ACS_Node *SY_InsertGlobalUnique(string name, int type);
void SY_FreeLocals();
void SY_FreeGlobals();
void SY_FreeConstants(int depth);
void SY_ClearShared();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

ConstList	acs_Constants;	// List of all constants
VarList		acs_Variables;	// List of all variables
ArrayList	acs_Arrays;		// List of all arrays
FunctList	acs_Functions;	// List of all functions
TypeList	acs_Types;		// List of new types / structs
FunCallList acs_FuncCall;	// List of commands in a script / function / method
DepthList	acs_Depths;		// List of depths within the code
NodeList	acs_Nodes;		// List of identifiers, and what they are

int CurrentDepthIndex;		// Current statement depth