
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

	bool isDeleted;

	void Delete()
	{
		if (isDeleted)
			ERR_Error(ERR_ALREADY_DELETED, false, name, tk_Line);
		else
			lib(isDeleted) = true;
	}
};

template <class type>
class ACS_IndexedObject
{
public:

	int index;			// Index in list

protected:

	using LTRef = vector<type>&;

	LTRef list;

	void initList(LTRef list)
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
		initList(sym_Types);
	}

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
		members = list[parent].members;
		methods = list[parent].methods;
		operators = list[parent].operators;
		constructors = list[parent].constructors;
		size = calcSSize();
		hasMembers = list[parent].hasMembers;
		set(parent);
		addListItem();
	}
	// Creates a new empty data type
	ACS_TypeDef(string type)
	{
		init();
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
		initializer = 0;
		isAssigned = false;
		isStatic = isConst = false;
		isWorld = isGlobal = false;
	}

public:

	int type;			// Type of variable / Return type
	int initializer;	// Initial value of variable

	bool isAssigned;	// Has a value been assigned to it?
	bool isStatic;		// Was this declared as static?
	bool isConst;		// Was this declared as const?
	bool isWorld;		// Is this a world var?
	bool isGlobal;		// Is this a global var?

	int size()
	{
		return sym_Types[type].size;
	}
};

// Class for all defined variables
class ACS_Var : public ACS_VarData, public ACS_IndexedObject<ACS_Var>, public ACS_DeletableObject<ACS_Var>
{
protected:

	void init()
	{
		ACS_VarData::init();
		initList(sym_Variables);
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
};

// Class for preprocessor defines
class ACS_Const : public ACS_Object, public ACS_IndexedObject<ACS_Const>, public ACS_DeletableObject<ACS_Const>
{
protected:

	void init()
	{
		ACS_Object::init();
		content = "";
		isLibDefine = false;
		initList(sym_Constants);
	}

public:

	string content;		// Content to replace with
	bool isLibDefine;	// Was this defined with libdefine?

	//Undefined Constant
	ACS_Const()
	{
		init();
	}
	//Undeclared Constant
	ACS_Const(string name, bool isImported = false)
	{
		init();
		set(name);
		set(isImported);
	}
	//Standard Declaration
	ACS_Const(string name, string content, int depth, bool isLibDefine = false, bool isImported = false)
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

	void init(LTRef fl = sym_Functions)
	{
		ACS_VarData::init();
		isOperator = isSpecial = isMember = isInline = isLatent = false;
		isUserDefined = false;
		optMask = outMask = varCount = 0;
		DirectCMD = StackCMD = PCD_NOP;
		OperatorType = TypeAssigned = NULL;
		initList(fl);
	}

public:

	vector<int> argTypes;		// List of argument types passed to the function
	int OperatorType;			// Type of operator, if isOperator is true
	int TypeAssigned;			// Type of typedef / struct that this is attached to
	int varCount;				// The number of variables and arguments used by this function
	int address;				// ?

	pCode DirectCMD;			// Direct pCode to add
	pCode StackCMD;				// Stack pCode to add

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
	ACS_Function(string name, pCode DirectCMD, pCode StackCMD, int argCount, int optMask, int outMask, bool returns, bool isLatent)
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
		depth = DEPTH_GLOBAL;
		addListItem();
		setOperator(0, 0, false);
	}
	// Use for defining new style internal functions
	// Warning: Using this constructor will add an additional function
	// You may specify the list that the function will be stored in
	ACS_Function(string name, pCode DirectCMD, pCode StackCMD, int type, vector<int> argTypes, DepthVal depth = DEPTH_GLOBAL, LTRef vec = sym_Functions)
	{
		init(vec);
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

class ACS_Array : public ACS_VarData, public ACS_IndexedObject<ACS_Array>, public ACS_DeletableObject<ACS_Var>
{
protected:

	void init()
	{
		ACS_VarData::init();
		dimAmt = 0;
		size = calcASize();
		initList(sym_Arrays);
	}

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
		lib(dimensions[dimAmt]) = value;
		dimAmt++;
		lib(size) = calcASize();
	}
};

class ACS_Node
{
protected:

	void init()
	{
		type = NODE_UNKNOWN;
		cmd = NULL;
	}

public:

	int type;				// What this node contains

	union					// Stores the pointer
	{
		ACS_Function * cmd;	// Pointer to the function
		ACS_Var * var;		// Pointer to the variable
		ACS_Array * arr;	// Pointer to the array
	};

	ACS_Node(NodeType type, int index)
	{
		init();
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
		default:
			this->type = NODE_VARIABLE;
			var = &sym_Variables[index];
			break;
		}
	}
	ACS_Node(NodeType type, string name)
	{
		init();
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
		default:
			this->type = NODE_VARIABLE;
			var = new ACS_Var(name);
			break;
		}
	}

#define PUBLIC_GET_SET(type, member) \
	type member() { return cmd->member; } \
	void member(type value) { cmd->member = value; }

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
		initList(sym_Depths);
		parent = DEPTH_NONE;
		index = DEPTH_NONE;
	}

public:
	ACS_DepthRoot()
	{
		init();
	}
	// Warning: Using this constructor can add an additional depth index
	ACS_DepthRoot(int parent, bool add = true)
	{
		init();
		set(parent);
		if(add) addListItem();
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

	void init()
	{
		initList(sym_Scripts);
		number = address = argCount = varCount = flags = srcLine = NULL;
		name = "";
		hasName = false;
		isImported = false;
		type = ST_CLOSED;
	}

public:

	int number;
	string name;
	bool hasName;

	int address;
	int argCount, varCount;
	int flags;
	int srcLine;

	bool isImported;

	ScriptType type;

	// Empty Constructor
	ACS_Script()
	{
		init();
	}
	// Numbered script
	ACS_Script(int number, int argCount, int flags, ScriptType type)
	{
		set(number);
		set(argCount);
		set(flags);
		set(type);
		srcLine = tk_Line;
		isImported = (ImportMode == IMPORT_Importing);
	}
	// String literal \ Identifier script
	ACS_Script(string name, int argCount, int flags, ScriptType type)
	{
		set(name);
		hasName = true;
		set(argCount);
		set(flags);
		set(type);
		srcLine = tk_Line;
		isImported = (ImportMode == IMPORT_Importing);
	}

	void Archive()
	{
		addListItem();
	}
};

class ACS_File : public ACS_IndexedObject<ACS_File>
{
protected:

	void init()
	{
		initList(sym_Files);
		outerDepth = DEPTH_NONE;
		isImporting = isLoaded = isProcessed = false;
	}

public:

	int outerDepth;		// Lowest Depth index, and most global in this file
	string name;		// Name of library / ACS script

	bool isImporting;	// This file was imported by another
	bool isLoaded;		// This file was loaded
	bool isProcessed;	// This file was parsed

	// Basic Constructor
	ACS_File()
	{
		init();
	}
	// Full Constructor - Automatically adds to list
	ACS_File(int outerDepth, string name, bool isImporting, bool isLoaded, bool isProcessed)
	{
		init();
		set(outerDepth);
		set(name);
		set(isImporting);
		set(isLoaded);
		set(isProcessed);
		addListItem();
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
