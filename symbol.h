
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

#define DEPTH_MAP_LEVEL	0
#define DEPTH_NONE		-1

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

template <class type>
class ACS_IndexedObject
{
public:

	int index;			// Index in list

protected:

	vector<type> &list;

	void initList(vector<type> &list)
	{
		set(list);
		index = -1;
	}

	// Places a copy in the list, sets the index on the copy and this
	void addListItem()
	{
		list.add((type)this);
		index = ((ACS_IndexedObject)list.lastAdded()).index = list.lastIndex();
	}

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
};

enum NodeTypes : int
{
	NODE_UNKNOWN = -1,
	NODE_FUNCTION,
	NODE_VARIABLE,
	NODE_ARRAY
};

struct ACS_VarRef
{
	string name;			// Name of member
	int type;				// Index of variable type
};

class ACS_TypeDef : public ACS_IndexedObject<ACS_TypeDef>
{
protected:

	void init()
	{
		size = 1;
		hasMembers = false;
		hasParent = false;
		initList(acs_Types);
	}

public:

	string type;			// Name of struct / typedef
	int size;				// Size of struct / typedef
	int parent;				// Index of parent struct / typedef

	VarRefList members;		// Defined member list
	FunctList methods;		// Index of defined method
	FunctList constructors;	// Index of defined constructor
	FunctList operators;	// Index of defined operator

	bool hasMembers;		// Does this define any members?
	bool hasParent;			// Does this have a parent?

	// Creates a new data type and defines all objects within
	ACS_TypeDef(string type, FunctList methods, FunctList constructors, FunctList operators, VarRefList members)
	{
		init();
		set(type);
		set(members);
		set(methods);
		set(constructors);
		set(operators);
		size = calcSSize();
		hasMembers = true;
		addListItem();
	}
	// Creates a copy of a different type
	ACS_TypeDef(string type, int parent)
	{
		init();
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
		init();
		set(type);
	}

	static void Init(string type)
	{
		setListItem(type);
	}

	//TODO: Add method of adding objects when inheriting

protected:

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
class ACS_Object
{
protected:

	void init()
	{
		depth = DEPTH_NONE;
		isDeclared = false;
		isImported = false;
	}

public:

	string name;		// Name of object in source
	int depth;			// Depth index

	bool isDeclared;	// Is it declared yet?
	bool isImported;	// Was this item imported?
};

// Class to provide ACS_Var, and others, with data
class ACS_VarData : public ACS_Object
{
protected:

	void init()
	{
		ACS_Object::init();
		type = VAR_VOID;
		isDeleted = isAssigned = false;
		isStatic = isConst = false;
		isWorld = isGlobal = false;
	}

public:

	int type;			// Type of variable / Return type

	bool isDeleted;		// Has 'delete' been called on it?
	bool isAssigned;	// Has a value been assigned to it?
	bool isStatic;		// Was this declared as static?
	bool isConst;		// Was this declared as const?
	bool isWorld;		// Is this a world var?
	bool isGlobal;		// Is this a global var?

	int size()
	{
		return acs_Types[type].size;
	}
};

// Class for all defined variables
class ACS_Var : public ACS_VarData, public ACS_IndexedObject<ACS_Var>
{
protected:

	void init()
	{
		ACS_VarData::init();
		initList(acs_Variables);
	}

public:

	// Undefined Variable
	ACS_Var()
	{
		init();
	}
	// Undefined Usage (Error, but keep going)
	ACS_Var(string name, bool isAssigned = false)
	{
		init();
		set(name);
		set(isAssigned);
	}
	// Standard Declaration
	ACS_Var(string name, int type, int depth, bool isAssigned = false, bool assignIndex = true)
	{
		init();
		set(name);
		set(type);
		set(depth);
		isDeclared = true;
		set(isAssigned);
		if (assignIndex)	// Arrays and structs must assign differently, allocating more space per unit.
			addListItem();
	}
	//TODO: Add Global and World Declarations

	// Returns true if the item was marked as deleted,
	// false if it was already deleted
	bool Delete()
	{
		if (isDeleted)
		{
			ERR_Error(ERR_ALREADY_DELETED, false, name, tk_Line);
			return false;
		}
		return lib(isDeleted) = true;
	}
};

// Class for preprocessor defines
class ACS_Const : public ACS_Object, public ACS_IndexedObject<ACS_Const>
{
protected:

	void init()
	{
		ACS_Object::init();
		initList(acs_Constants);
	}

public:

	string content;		// Content to replace with
	bool isLibDefine;	// Was this defined with libdefine?

	//Undefined Constant
	ACS_Const()
	{
		init();
	}
	//Standard Declaration
	ACS_Const(string name, string content, bool isLibDefine = false, bool isImported = false)
	{
		init();
		set(name);
		set(content);
		set(isLibDefine);
		set(isImported);
		isDeclared = true;
		addListItem();
	}
};

// Class for defined functions
class ACS_Function : public ACS_VarData, public ACS_IndexedObject<ACS_Function>
{
protected:

	void init()
	{
		ACS_VarData::init();
		isOperator = isSpecial = isMember = isInline = isLatent = false;
		isUserDefined = false;
		optMask = outMask = 0;
		DirectCMD = StackCMD = PCD_NOP;
		OperatorType = TypeAssigned = NULL;
		initList(acs_Functions);
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
	bool isInline;				// Is this function going to be placed inline?

	// Use for an empty function
	ACS_Function(string name)
	{
		init();
		set(name);
		isUserDefined = true;
	}
	// Use for defining old style internal functions
	// Warning: Using this constructor will add an additional function
	ACS_Function(string name, pcd_t DirectCMD, pcd_t StackCMD, int argCount, int optMask, int outMask, bool returns, bool isLatent)
	{
		init();
		set(name);
		set(DirectCMD);
		set(StackCMD);
		argTypes.assign(argCount, VAR_INT);
		set(isLatent);
		type = (returns) ? VAR_INT : VAR_VOID;
		//TODO: Figure out 'outmask' / 'optmask'
		set(optMask);
		set(outMask);
		depth = DEPTH_MAP_LEVEL;
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
		isUserDefined = true;
		addListItem();
		setOperator(0, 0, false);
	}

	void setOperator(int OperatorType, int TypeAssigned, bool isOperator = true)
	{
		libset(isOperator);
		lib(isMember) = isOperator;
		libset(OperatorType);
		libset(TypeAssigned);
	}

	void setMethod(int TypeAssigned)
	{
		lib(isMember) = true;
		libset(TypeAssigned);
	}

	void setInline()
	{
		lib(isInline) = true;
	}
};

struct ACS_FuncCall
{
	int index;					// Index of the function to call
	int depth;					// Depth index
	VarRefList args;			// Arguments
};

class ACS_Array : public ACS_VarData, public ACS_IndexedObject<ACS_Array>
{
protected:

	void init()
	{
		ACS_VarData::init();
		dimAmt = 0;
		size = calcASize();
		initList(acs_Arrays);
	}

public:
	int size, dimAmt;
	int dimensions[MAX_ARRAY_DIMS];

	// Undefined Variable
	ACS_Array()
	{
		init();
	}
	// Undefined Usage (Error, but keep going)
	ACS_Array(string name, bool isAssigned = false)
	{
		init();
		set(name);
		set(isAssigned);
	}
	ACS_Array(string name, int type, int depth, bool isAssigned = false)
	{
		init();
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
		lib(dimensions[dimAmt]) = value;
		dimAmt++;
		lib(size) = calcASize();
	}

	bool Delete()
	{
		if (isDeleted)
		{
			ERR_Error(ERR_ALREADY_DELETED, false, name, tk_Line);
			return false;
		}
		return lib(isDeleted) = true;
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

	void init()
	{
		type = NODE_UNKNOWN;
		cmd = NULL; var = NULL;
		arr = NULL;
	}

public:

	int type;			// What this node contains
	ACS_Function * cmd;	// Pointer to the function
	ACS_Var * var;		// Pointer to the variable
	ACS_Array * arr;	// Pointer to the array

	ACS_Node(NodeTypes type, int index)
	{
		set(type);

		switch (type)
		{
		case NODE_FUNCTION:
			cmd = &acs_Functions[index];
			break;
		case NODE_ARRAY:
			arr = &acs_Arrays[index];
			break;
		case NODE_VARIABLE:
			var = &acs_Variables[index];
			break;
		}
	}
	ACS_Node(int type, string name)
	{
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

#define PUBLIC_GET_SET(type, member) type member() { return obj->member; } void member(type value) { obj->member = value; }

	PUBLIC_GET_SET(string, name);
	PUBLIC_GET_SET(bool, isDeclared);
	PUBLIC_GET_SET(bool, isImported);
	PUBLIC_GET_SET(int, depth);
	PUBLIC_GET_SET(int, index);

#undef PUBLIC_GET_SET

	void clear()
	{
		init();
	}
};

class ACS_DepthRoot : public ACS_IndexedObject<ACS_DepthRoot>
{
protected:

	int parent;

	void init()
	{
		initList(acs_Depths);
		parent = DEPTH_NONE;
		index = DEPTH_NONE;
	}

public:
	ACS_DepthRoot()
	{
		init();
	}
	// Warning: Using this constructor will add an additional depth index
	ACS_DepthRoot(int parent, bool add = true)
	{
		init();
		set(parent);
		if(add) addListItem();
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
		return index;
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
