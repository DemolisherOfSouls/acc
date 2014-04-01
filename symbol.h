
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

#define MAX_ARRAY_DIMS	8		// Seems reasonable
#define MAX_DEFINED_TYPES 256	// Will increase if necessary

// TYPES -------------------------------------------------------------------

template <class type> struct ACS_IndexedObject;
struct ACS_VarRef;
struct ACS_TypeDef;
struct ACS_Object;
struct ACS_VarData;
struct ACS_Var;
struct ACS_Const;
struct ACS_Function;
struct ACS_FuncCall;
struct ACS_Array;
struct ACS_Node;
struct ACS_DepthRoot;
struct ACS_Script;
struct ACS_File;

using ConstList = vector<ACS_Const>;
using VarList = vector<ACS_Var>;
using VarRefList = vector<ACS_VarRef>;
using FunctList = vector<ACS_Function>;
using TypeList = vector<ACS_TypeDef>;
using ArrayList = vector<ACS_Array>;
using FunCallList = vector<ACS_FuncCall>;
using DepthList = vector<ACS_DepthRoot>;
using NodeList = vector<ACS_Node>;
using ScriptList = vector<ACS_Script>;
using FileList = vector<ACS_File>;

enum DataType : int
{
	VAR_VOID,
	VAR_INT,
	VAR_BOOL,
	VAR_STR,

	VAR_COUNT
};
enum NodeType : int
{
	NODE_UNKNOWN = -1,
	NODE_FUNCTION,
	NODE_VARIABLE,
	NODE_ARRAY
};
enum DepthVal : int
{
	DEPTH_NONE = -1,	// Undefined
	DEPTH_GLOBAL,		// Map level, initial file
};

template <class type>
class ACS_DeletableObject
{
private:

	using LTRef = vector<type>&;

	LTRef list;

public:

	bool isDeleted = false;

	static void Delete(int index, LTRef list)
	{
		if (list[index].isDeleted)
			ERR_Error(ERR_ALREADY_DELETED, true, VecStr(name, tk_Line);
		else
			lib(isDeleted) = true;
	}
};

template <class type>
class ACS_IndexedObject
{
public:

	int index = INVALID_INDEX;	// Index in list

protected:

	using LTRef = vector<type>&;

	LTRef list;

	// Creates a named instance in the list, sets the index on the instance
	static void setListItem(string name)
	{
		list.add(type(name));
		list.lastAdded().index = list.lastIndex();
	}
	static void setListItem(int index)
	{
		list.add(type(index, false));
		list.lastAdded().index = list.lastIndex();
	}

public:

	// Places a copy in the given list, sets the index on the copy and this
	void addListItem(LTRef list)
	{
		set(list);
		list.add((type)this);
		index = ((ACS_IndexedObject)list.lastAdded()).index = list.lastIndex();
	}
};

struct ACS_VarRef
{
	string name = "";			// Name of member
	int type = VAR_VOID;		// Index of variable type
};

class ACS_TypeDef : public ACS_IndexedObject<ACS_TypeDef>
{
protected:

	int calcSSize()
	{
		if (!hasMembers)
			return 1;

		int ret = 0;
		for (int i = 0; i < members.size; i++)
		{
			ret += sym_Types[members[i].type].size;
		}
		return ret;
	}

public:

	string type = "";			// Name of struct / typedef
	int size = 1;				// Size of struct / typedef
	int parent = INVALID_INDEX;	// Index of parent struct / typedef

	// Defined member list
	VarRefList members = VarRefList(MAX_STRUCT_VARIABLES);
	// Index of defined methods
	FunctList methods = FunctList(MAX_STRUCT_METHODS);
	// Index of defined constructors
	FunctList constructors = FunctList(MAX_STRUCT_CONSTRUCTORS);
	// Index of defined operators
	FunctList operators = FunctList(MAX_STRUCT_OPERATORS);

	bool hasMembers = false;	// Does this define any members?
	bool hasParent = false;		// Does this have a parent?

	// Creates a new data type and defines all objects within
	ACS_TypeDef(string type, FunctList methods, FunctList constructors, FunctList operators, VarRefList members)
	{
		set(type);
		set(members);
		set(methods);
		set(constructors);
		set(operators);
		size = calcSSize();
		hasMembers = true;
	}
	// Creates a copy of a different type
	ACS_TypeDef(string type, int parent)
	{
		set(type);
		members = list[parent].members;
		methods = list[parent].methods;
		operators = list[parent].operators;
		constructors = list[parent].constructors;
		size = calcSSize();
		hasMembers = list[parent].hasMembers;
		set(parent);
	}
	// Creates a new empty data type
	ACS_TypeDef(string type)
	{
		set(type);
	}

	static void Init(string&& type)
	{
		setListItem(move(type));
	}

	//TODO: Add method of adding objects when inheriting
};

// Base class for acs objects
class ACS_Object
{
public:

	string name = "";				// Name of object in source
	DepthVal depth = DEPTH_NONE;	// Depth index

	bool isDeclared = false;		// Is it declared yet?
	bool isImported = false;		// Was this item imported?
};

// Class to provide ACS_Var, and others, with data
class ACS_VarData : public ACS_Object
{
public:

	int type = VAR_VOID;		// Type of variable / Return type
	int initializer = 0;		// Initial value of variable

	bool isAssigned = false;	// Has a value been assigned to it?
	bool isStatic = false;		// Was this declared as static?
	bool isConst = false;		// Was this declared as const?
	bool isWorld = false;		// Is this a world var?
	bool isGlobal = false;		// Is this a global var?

	int size()
	{
		return sym_Types[type].size;
	}
};

// Class for all defined variables
class ACS_Var : public ACS_VarData, public ACS_IndexedObject<ACS_Var>, public ACS_DeletableObject<ACS_Var>
{
public:

	// Undefined Variable
	ACS_Var() { }
	// Undefined Usage (Error, but keep going)
	ACS_Var(string name, bool isAssigned = false)
	{
		set(name);
		set(isAssigned);
	}
	// Standard Declaration
	ACS_Var(string name, DataType type, DepthVal depth, bool isAssigned = false)
	{
		set(name);
		set(type);
		set(depth);
		isDeclared = true;
		set(isAssigned);
	}
	//TODO: Add Global and World Declarations
};

// Class for preprocessor defines
class ACS_Const : public ACS_Object, public ACS_IndexedObject<ACS_Const>, public ACS_DeletableObject<ACS_Const>
{
public:

	string content = "";		// Content to replace with
	bool isLibDefine = false;	// Was this defined with libdefine?

	//Undefined Constant
	ACS_Const()
	{
	}
	//Undeclared Constant
	ACS_Const(string name, bool isImported = false)
	{
		set(name);
		set(isImported);
	}
	//Standard Declaration
	ACS_Const(string name, string content, int depth, bool isLibDefine = false, bool isImported = false)
	{
		set(name);
		set(content);
		set(isLibDefine);
		set(isImported);
		isDeclared = true;
	}
};

// Class for defined functions
class ACS_Function : public ACS_VarData, public ACS_IndexedObject<ACS_Function>
{
public:

	VecInt argTypes;					// List of argument types passed to the function
	int OperatorType = NULL;			// Type of operator, if isOperator is true
	int TypeAssigned = VAR_VOID;		// Type of typedef / struct that this is attached to
	int varCount = 0;					// The number of variables and arguments used by this function
	int address = NULL;					// The location of the pCode for this function

	pCode DirectCMD = PCD_NOP;			// Direct pCode to add
	pCode StackCMD = PCD_NOP;			// Stack pCode to add

	int optMask = 0;					// ?
	int outMask = 0;					// ?

	bool isOperator = false;			// Is this function representing an operator?
	bool isSpecial = false;				// Is this function an action special?
	bool isLatent = false;				// Does this function cause a delay or wait?
	bool isMember = false;				// Is this function restricted to a type?
	bool isUserDefined = false;			// Was this written by the user?
	bool isInline = false;				// Is this function going to be placed inline?

	// Use for an empty function
	ACS_Function(string name)
	{
		set(name);
		isUserDefined = true;
	}
	// Use for defining old style internal functions
	ACS_Function(string name, pCode DirectCMD, pCode StackCMD, int argCount, int optMask, int outMask, bool returns, bool isLatent)
	{
		set(name);
		set(DirectCMD);
		set(StackCMD);
		argTypes.assign(argCount, VAR_INT);
		set(isLatent);
		type = (returns) ? VAR_INT : VAR_VOID;
		//TODO: Figure out 'outmask' / 'optmask'
		set(optMask);
		set(outMask);
		depth = DEPTH_GLOBAL;
		setOperator(0, 0, false);
	}
	// Use for defining new style internal functions
	// You may specify the list that the function will be stored in
	ACS_Function(string name, int type, VecInt argTypes, DepthVal depth = DEPTH_GLOBAL)
	{
		set(name);
		set(depth);
		set(DirectCMD);
		set(StackCMD);
		set(type);
		set(argTypes);
		isUserDefined = true;
		setOperator(0, 0, false);
	}

	void setOperator(int OperatorType, int TypeAssigned, bool isOperator = true)
	{
		set(isOperator);
		isMember = isOperator;
		set(OperatorType);
		set(TypeAssigned);
	}
	void setMethod(int TypeAssigned)
	{
		isMember = true;
		set(TypeAssigned);
	}
	void setInline()
	{
		isInline = true;
	}
};

struct ACS_FuncCall
{
	int index;					// Index of the function to call
	int depth;					// Depth index
	VarRefList args;			// Arguments
};

class ACS_Array : public ACS_VarData, public ACS_IndexedObject<ACS_Array>, public ACS_DeletableObject<ACS_Var>
{
protected:

	int calcASize()
	{
		int ret = 1;
		for (int i = 0; i < dimAmt; i++)
		{
			ret *= dimensions[i];
		}

		return ret;
	}

public:
	int size = 1, dimAmt = 0;
	int dimensions[MAX_ARRAY_DIMS];

	// Undefined Variable
	ACS_Array()
	{
	}
	// Undefined Usage (Error, but keep going)
	ACS_Array(string name, bool isAssigned = false)
	{
		set(name);
		set(isAssigned);
	}
	ACS_Array(string name, int type, DepthVal depth, bool isAssigned = false)
	{
		set(name);
		set(type);
		set(depth);
		set(isAssigned);
	}

	void setDimension(int value)
	{
		if (value < 1)
		{
			ERR_Error(ERR_INVALID_ARRAY_SIZE, false);
			value = 1;
		}
		if (dimAmt >= MAX_ARRAY_DIMS)
		{
			ERR_Error(ERR_TOO_MANY_ARRAY_DIMS, false);
			return;
		}
		dimensions[dimAmt] = value;
		dimAmt++;
		size = calcASize();
	}
};

class ACS_Node
{
public:

	int type = NODE_UNKNOWN;				// What this node contains

	union					// Stores the pointer
	{
		ACS_Function * cmd;	// Pointer to the function
		ACS_Var * var;		// Pointer to the variable
		ACS_Array * arr;	// Pointer to the array
	};

	ACS_Node(NodeType type, int index)
	{
		cmd = NULL;
		set(type);

		switch (type)
		{
		case NODE_FUNCTION:
			cmd = &sym_Functions[index];
			break;
		case NODE_ARRAY:
			arr = &sym_Arrays[index];
			break;
		case NODE_VARIABLE:
			var = &sym_Variables[index];
			break;
		}
	}
	ACS_Node(NodeType type, string name)
	{
		cmd = NULL;
		set(type);

		switch (type)
		{
		case NODE_FUNCTION:
			cmd = new ACS_Function(name);
			break;
		case NODE_ARRAY:
			arr = new ACS_Array(name);
			break;
		case NODE_VARIABLE:
			var = new ACS_Var(name);
			break;
		}
	}

	get_set(string, name, cmd);
	get_set(bool, isDeclared, cmd);
	get_set(bool, isImported, cmd);
	get_set(DepthVal, depth, cmd);
	get_set(int, index, cmd);

	void clear()
	{
		//TODO: clear node
	}
};

class ACS_DepthRoot : public ACS_IndexedObject<ACS_DepthRoot>
{
protected:

	int parent = DEPTH_NONE;

public:
	ACS_DepthRoot()
	{
		
	}
	// Warning: Using this constructor can add an additional depth index
	ACS_DepthRoot(int parent, bool add = true)
	{
		set(parent);
		if(add) addListItem(sym_Depths);
	}

	static void Init(int && parent)
	{
		setListItem(move(parent));
	}

	const ACS_DepthRoot& WidenScope()
	{
		if (!isParentValid())
			return NULL;

		return list[parent];
	}

	static const ACS_DepthRoot& WidenScope(const ACS_DepthRoot& depth)
	{
		if (depth.parent == DEPTH_NONE)
			return NULL;

		return depth.list[depth.parent];
	}

	bool isParentValid() { return (this->parent != DEPTH_NONE); }
	bool isParentOuter() { return (this->parent == pa_FileDepth); }
	bool isValid() { return (this->index != DEPTH_NONE); }
	bool isOuter() { return (this->index == pa_FileDepth); }
	int Parent() { return this->parent; }
	int Current() { return this->index; }
};

class ACS_Script : public ACS_IndexedObject<ACS_Script>
{
protected:

public:

	int number = NULL;
	string name = "";
	bool hasName = false;

	int address = NULL;
	int argCount, varCount;
	int flags = NULL;
	int srcLine = NULL;

	bool isImported = false;

	ScriptActivation type = ST_CLOSED;

	// Empty Constructor
	ACS_Script()
	{
		//
	}
	// Numbered script
	ACS_Script(int number, int argCount, int flags, ScriptActivation type)
	{
		set(number);
		set(argCount);
		set(flags);
		set(type);
		srcLine = tk_Line;
		isImported = (ImportMode == IMPORT_Importing);
	}
	// String literal \ Identifier script
	ACS_Script(string name, int argCount, int flags, ScriptActivation type)
	{
		set(name);
		hasName = true;
		set(argCount);
		set(flags);
		set(type);
		srcLine = tk_Line;
		isImported = (ImportMode == IMPORT_Importing);
	}
};

class ACS_File : public ACS_IndexedObject<ACS_File>
{
public:

	int outerDepth = DEPTH_NONE;	// Lowest Depth index, and most global in this file
	string name = "";				// Name of library / ACS script

	bool isImporting = false;		// This file was imported by another
	bool isLoaded = false;			// This file was loaded
	bool isProcessed = false;		// This file was parsed

	// Basic Constructor
	ACS_File() {}
	// Full Constructor
	ACS_File(int outerDepth, string name, bool isImporting, bool isLoaded, bool isProcessed)
	{
		set(outerDepth);
		set(name);
		set(isImporting);
		set(isLoaded);
		set(isProcessed);
	}
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void sym_Init();
ACS_Node *sym_Find(string name);
ACS_Node *sym_FindLocal(string name);
ACS_Node *sym_FindGlobal(string name);
ACS_Node *sym_InsertLocal(string name, NodeType type);
ACS_Node *sym_InsertGlobal(string name, NodeType type);
ACS_Node *sym_InsertGlobalUnique(string name, NodeType type);
void sym_FreeLocals();
void sym_FreeGlobals();
void sym_ClearShared();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

ConstList	sym_Constants;	// List of all constants
VarList		sym_Variables;	// List of all variables
VarList		sym_Structs;	// List of all structs
ArrayList	sym_Arrays;		// List of all arrays
FunctList	sym_Functions;	// List of all functions
FunctList	sym_Operators;	// List of new operators
FunCallList sym_FuncCall;	// List of commands in a script / function / method

// List of new types / structs
// Contains within it the operators, members, constructors, and methods
// specific to itself
TypeList	sym_Types;

DepthList	sym_Depths;		// List of depths within the code
NodeList	sym_Nodes;		// List of identifiers, and what they are
ScriptList	sym_Scripts;	// List of all defined scripts
FileList	sym_Files;		// List of loaded and referenced acs files

extern int pa_CurrentDepth;	// Current statement depth
extern int pa_FileDepth;	// Outermost level in the current file
