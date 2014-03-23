
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

#define VAR_VOID		0
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

enum NodeTypes : int
{
	NODE_FUNCTION,
	NODE_VARIABLE,
	NODE_ARRAY
};

struct ACS_VarRef
{
	string name;		// Name of member
	int type;			// Index of variable type
};

struct ACS_TypeDef
{
	string type;			// Name of struct / typedef
	int size;				// Size of struct / typedef
	int parent;				// Index of parent struct / typedef
	int index;				// Index of this struct / typedef

	VarRefList members;		// Defined member list
	FunctList methods;		// Index of defined method
	FunctList constructors;	// Index of defined constructor
	FunctList operators;	// Index of defined operator

	bool hasMembers;		// Does this define any members?
	bool hasParent;			// Does this have a parent?

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
		hasParent = false;
		addListItem();
	}
	// Creates a copy of a different type
	ACS_TypeDef(string type, int parent)
	{
		set(type);
		members = acs_Types[parent].members;
		methods = acs_Types[parent].methods;
		operators = acs_Types[parent].operators;
		constructors = acs_Types[parent].constructors;
		size = calcSSize();
		hasMembers = acs_Types[parent].hasMembers;
		set(parent);
		addListItem();
	}
	// Creates a new empty data type
	ACS_TypeDef(string type)
	{
		set(type);
		size = 1;

		hasMembers = false;
		hasParent = false;
	}

	static void Init(string type)
	{
		setListItem(type);
	}

	//TODO: Add method of adding objects when inheriting

protected:

	static void setListItem(string type)
	{
		acs_Types.add(ACS_TypeDef(type));
		acs_Types.lastAdded().index = acs_Types.lastIndex();
	}

	void addListItem()
	{
		acs_Types.add(*this);
		index = acs_Types.lastIndex;
		acs_Types[index].index = index;
	}

	int calcSSize()
	{
		if (!hasMembers)
			return 1;

		int ret = 0;
		for (int i = 0; i < members.size; i++)
		{
			ret += acs_Types[members[i].type].size;
		}
		return ret;
	}
};

// Base class for acs objects
struct ACS_Object
{
	string name;		// Name of object in source
	int index;			// Index in list
	int depth;			// Depth index

	bool isDeclared;	// Is it declared yet?
	bool isImported;	// Was this item imported?

protected:

	template <class ACS_ObjectType, vector<ACS_ObjectType> list>
	static void setListItem(string type)
	{
		list.add(ACS_TypeDef(type));
		list.lastAdded().index = acs_Types.lastIndex();
	}
};

// Class for all defined variables
struct ACS_Var : public ACS_Object
{
	int type;			// Type of variable / Return type

	bool isDeleted;		// Has 'delete' been called on it?
	bool isAssigned;	// Has a value been assigned to it?
	bool isStatic;		// Was this declared as static?
	bool isConst;		// Was this declared as const?
	bool isWorld;		// Is this a world var?
	bool isGlobal;		// Is this a global var?

	// Undefined Variable
	ACS_Var()
	{
		index = -1;
		type = VAR_NULL;
		isDeleted = false;
		isDeclared = false;
		isAssigned = false;
		isImported = false;
		isStatic = false;
		isConst = false;
		isWorld = isGlobal = false;
	}
	// Undefined Usage (Error, but keep going)
	ACS_Var(string name, bool assigned = false)
	{
		this->name = name;
		type = VAR_NULL;
		depth = 0;
		isStatic = false;
		isDeleted = false;
		isImported = false;
		isDeclared = false;
		isConst = false;
		isAssigned = assigned;
		isWorld = isGlobal = false;
	}
	// Standard Declaration
	ACS_Var(string name, int type, int depth, bool isAssigned = false, bool assignIndex = true)
	{
		this->name = name;
		if (assignIndex) // Arrays and structs must assign differently, allocating more space per unit.
			index = getNewIndex();
		this->type = type;
		this->depth = depth;
		isStatic = false;
		isDeleted = false;
		isImported = false;
		isDeclared = true;
		isConst = false;
		this->isAssigned = isAssigned;
		isWorld = isGlobal = false;
	}
	//TODO: Add Global and World Declarations

	int size()
	{
		return acs_Types[type].size;
	}

	// Returns true if the item was marked as deleted,
	// false if it was already deleted
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

protected: // These shouldn't be called outside of the struct

	// Gets a new index, and stores the variable in the vector
	int getNewIndex()
	{
		return 0; //TODO: Add index getter.
	}
};

// Class for preprocessor defines
struct ACS_Const : public ACS_Object
{
	string content;		// Content to replace with
	bool isLibDefine;	// Was this defined with libdefine?

	//Undefined Constant
	ACS_Const()
	{
		index = -1;
		isDeclared = false;
	}
	//Standard Declaration
	ACS_Const(string name, string content, bool isLibDefine = false)
	{
		this->name = name;
		this->content = content;
		index = 0; //TODO: Set up indexing for constants!
		this->isLibDefine = isLibDefine;
		isDeclared = true;
		isImported = false; // TODO: Check for import for constants!
	}
};

// Class for defined functions
class ACS_Function : public ACS_Var
{
protected:

	void addListItem()
	{
		acs_Functions.add(*this);
		index = acs_Functions.lastIndex();
		acs_Functions[index].index = index;
	}

public:

	vector<int> argTypes;		// List of argument types passed to the function
	int OperatorType;			// Type of operator, if isOperator is true
	int TypeAssigned;			// Type of typedef / struct that this is attached to

	pcd_t DirectCMD;			// Direct pCode to add
	pcd_t StackCMD;				// Stack pCode to add

	int optMask;				// ?
	int outMask;				// ?

	bool isOperator;			// Is this function representing an operator?
	bool isSpecial;				// Is this function an action special?
	bool isLatent;				// Does this function cause a delay or wait?
	bool isMember;				// Is this function restricted to a type?
	bool isUserDefined;			// Was this written by the user?
	bool IsInline;				// Is this function going to be placed inline?

	// Use for defining old style internal functions
	// Warning: Using this constructor will add an additional function
	ACS_Function(string name, pcd_t DirectCMD, pcd_t StackCMD, int argCount, int optMask, int outMask, bool returns, bool isLatent)
	{
		set(name);
		set(DirectCMD);
		set(StackCMD);
		argTypes.assign(argCount, VAR_INT);
		set(isLatent);
		type = (returns) ? VAR_INT : VAR_NULL;
		//TODO: Figure out 'outmask' / 'optmask'
		set(optMask);
		set(outMask);
		depth = 0;
		isUserDefined = false;
		IsInline = false;
		isSpecial = false;
		addListItem();
		setOperator(0, 0, false);
	}
	// Use for defining new style internal functions
	// Warning: Using this constructor will add an additional function
	ACS_Function(string name, pcd_t DirectCMD, pcd_t StackCMD, int type, vector<int> argTypes, int depth = 0)
	{
		set(name);
		set(depth);
		set(DirectCMD);
		set(StackCMD);
		set(type);
		set(argTypes);
		IsInline = false;
		isSpecial = false;
		isUserDefined = true;
		addListItem();
		setOperator(0, 0, false);
	}

	void setOperator(int OperatorType, int TypeAssigned, bool isOperator = true)
	{
		libset(acs_Functions, isOperator);
		isMember = isOperator;
		libset(acs_Functions, OperatorType);
		libset(acs_Functions, TypeAssigned);
	}

	void setMethod(int TypeAssigned)
	{
		lib(acs_Functions, isMember) = true;
		libset(acs_Functions, TypeAssigned);
	}

	void setInline()
	{
		lib(acs_Functions, IsInline) = true;
	}
};

struct ACS_FuncCall
{
	int index;					// Index of the function to call
	int depth;					// Depth index
	VarRefList args;			// Arguments
};

class ACS_Array : public ACS_Var
{
public:
	int size, dimAmt;
	int dimensions[MAX_ARRAY_DIMS];

	ACS_Array(string name, int type, int depth, bool isAssigned = false) : ACS_Var(name, type, depth, isAssigned, false)
	{
		dimAmt = 0;
		size = calcASize();
	}

	void setDimension(int value)
	{
		if (value < 1)
		{
			ERR_Error(ERR_INVALID_ARRAY_SIZE, false);
		}
		if (dimAmt >= 8)
		{
			ERR_Error(ERR_TOO_MANY_ARRAY_DIMS, false);
			return;
		}
		dimensions[dimAmt++] = value;
		size = calcASize();
	}

	void setIndex()
	{
		// TODO: In list, 'filler' items that hold no data,
		// but simply point to the first index.
		// Investigate how they are currently implemented.

		acs_Arrays.add(*this);
		index = acs_Arrays.lastIndex;
		lib(acs_Arrays, index);
	}

	int size()
	{
		return size;
	}

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

};

class ACS_Node
{
protected:

	ACS_Object * obj;	// Pointer to a basic version of the contents

public:

	int type;			// What this node contains
	ACS_Function * cmd;	// Pointer to the function
	ACS_Var * var;		// Pointer to the variable
	ACS_Array * arr;	// Pointer to the array

	ACS_Node(NodeTypes type, int index)
	{
		set(type);
		set(index);

		switch (type)
		{
		case NODE_FUNCTION:
			*obj = *cmd = acs_Functions[index];
			break;
		case NODE_ARRAY:
			*obj = *arr = acs_Arrays[index];
			break;
		case NODE_VARIABLE:
			*obj = *var = acs_Variables[index];
			break;
		}
	}

#define PUBLIC_GET(type, member) type member() { return obj->member; }

	PUBLIC_GET(string, name);
	PUBLIC_GET(int, index);
	PUBLIC_GET(bool, isDeclared);
	PUBLIC_GET(bool, isImported);
	PUBLIC_GET(int, depth);

#undef PUBLIC_GET
};

class ACS_DepthRoot
{
protected:

	int parent;
	int current;

	void addListItem()
	{
		acs_Depths.add(*this);
		acs_Depths.lastAdded().current = current = acs_Depths.lastIndex;
	}

	static void setListItem(int parent)
	{
		acs_Depths.add(ACS_DepthRoot());
		acs_Depths.lastAdded().parent = parent;
		acs_Depths.lastAdded().current = acs_Depths.lastIndex();
	}

public:
	ACS_DepthRoot()
	{
		parent = 0;
		current = 0;
	}
	// Warning: Using this constructor will add an additional depth index
	ACS_DepthRoot(int parent)
	{
		set(parent);
		addListItem();
	}

	static void Init(int parent)
	{
		setListItem(parent);
	}

	int Parent()
	{
		return parent;
	}

	int Current()
	{
		return current;
	}
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
void SY_ClearShared();

// PUBLIC DATA DECLARATIONS ------------------------------------------------

ConstList	acs_Constants;	// List of all constants
VarList		acs_Variables;	// List of all variables
ArrayList	acs_Arrays;		// List of all arrays
FunctList	acs_Functions;	// List of all functions
FunctList	acs_Operators;	// List of new operators
TypeList	acs_Types;		// List of new types / structs
FunCallList acs_FuncCall;	// List of commands in a script / function / method
DepthList	acs_Depths;		// List of depths within the code
NodeList	acs_Nodes;		// List of identifiers, and what they are

extern int pa_CurrentDepth;	// Current statement depth
extern int pa_FileDepth;	// Outermost level in the current file
