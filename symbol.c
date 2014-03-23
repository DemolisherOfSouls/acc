
//**************************************************************************
//**
//** symbol.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "common.h"
#include "symbol.h"
#include "misc.h"
#include "parse.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

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
	ACS_TypeDef(string type, FunctList met, FunctList con, FunctList op, VarRefList members)
	{
		this->type = type;
		this->members = members;
		methods = met;
		constructors = con;
		operators = op;
		size = calcSSize();
		hasMembers = true;
		hasParent = false;
		AddToList();
	}
	// Creates a copy of a different type
	ACS_TypeDef(string type, int parent)
	{
		this->type = type;
		members = acs_Types[parent].members;
		methods = acs_Types[parent].methods;
		operators = acs_Types[parent].operators;
		constructors = acs_Types[parent].constructors;
		size = calcSSize();
		hasMembers = acs_Types[parent].hasMembers;
		this->parent = parent;
		AddToList();
	}
	// Creates a new empty data type
	ACS_TypeDef(string type)
	{
		this->type = type;
		size = 1;

		hasMembers = false;
		hasParent = false;
	}

	//TODO: Add method of adding objects when inheriting

protected:

	void AddToList()
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

	void AddRoot()
	{
		acs_Depths.add(*this);
		current = acs_Depths.lastIndex;
		acs_Depths[current].current = current;
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
		AddRoot();
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

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static ACS_Node *Find(string name, ACS_Node *root);
static ACS_Node *Insert(string name, int type, ACS_Node **root);
static void FreeNodes(ACS_Node *root);
static void FreeNodesAtDepth(ACS_Node **root, int depth);
static void DeleteNode(ACS_Node *node, ACS_Node **parent_p);
static void ClearShared(ACS_Node *root);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static ACS_Function InternalFunctions[] =
{
	{ "tagwait", PCD_TAGWAITDIRECT, PCD_TAGWAIT, 1, 0, 0, false, true },
	{ "polywait", PCD_POLYWAITDIRECT, PCD_POLYWAIT, 1, 0, 0, false, true },
	{ "scriptwait", PCD_SCRIPTWAITDIRECT, PCD_SCRIPTWAIT, 1, 0, 0, false, true },
	{ "namedscriptwait", PCD_NOP, PCD_SCRIPTWAITNAMED, 1, 0, 0, false, true },
	{ "delay", PCD_DELAYDIRECT, PCD_DELAY, 1, 0, 0, false, true },
	{ "random", PCD_RANDOMDIRECT, PCD_RANDOM, 2, 0, 0, true, false },
	{ "thingcount", PCD_THINGCOUNTDIRECT, PCD_THINGCOUNT, 2, 0, 0, true, false },
	{ "thingcountname", PCD_NOP, PCD_THINGCOUNTNAME, 2, 0, 0, true, false },
	{ "changefloor", PCD_CHANGEFLOORDIRECT, PCD_CHANGEFLOOR, 2, 0, 0, false, false },
	{ "changeceiling", PCD_CHANGECEILINGDIRECT, PCD_CHANGECEILING, 2, 0, 0, false, false },
	{ "lineside", PCD_NOP, PCD_LINESIDE, 0, 0, 0, true, false },
	{ "clearlinespecial", PCD_NOP, PCD_CLEARLINESPECIAL, 0, 0, 0, false, false },
	{ "playercount", PCD_NOP, PCD_PLAYERCOUNT, 0, 0, 0, true, false },
	{ "gametype", PCD_NOP, PCD_GAMETYPE, 0, 0, 0, true, false },
	{ "gameskill", PCD_NOP, PCD_GAMESKILL, 0, 0, 0, true, false },
	{ "timer", PCD_NOP, PCD_TIMER, 0, 0, 0, true, false },
	{ "sectorsound", PCD_NOP, PCD_SECTORSOUND, 2, 0, 0, false, false },
	{ "ambientsound", PCD_NOP, PCD_AMBIENTSOUND, 2, 0, 0, false, false },
	{ "soundsequence", PCD_NOP, PCD_SOUNDSEQUENCE, 1, 0, 0, false, false },
	{ "setlinetexture", PCD_NOP, PCD_SETLINETEXTURE, 4, 0, 0, false, false },
	{ "setlineblocking", PCD_NOP, PCD_SETLINEBLOCKING, 2, 0, 0, false, false },
	{ "setlinespecial", PCD_NOP, PCD_SETLINESPECIAL, 7, 4|8|16|32|64, 0, false, false },
	{ "thingsound", PCD_NOP, PCD_THINGSOUND, 3, 0, 0, false, false },
	{ "activatorsound", PCD_NOP, PCD_ACTIVATORSOUND, 2, 0, 0, false, false },
	{ "localambientsound", PCD_NOP, PCD_LOCALAMBIENTSOUND, 2, 0, 0, false, false },
	{ "setlinemonsterblocking", PCD_NOP, PCD_SETLINEMONSTERBLOCKING, 2, 0, 0, false, false },
	{ "fixedmul", PCD_NOP, PCD_FIXEDMUL, 2, 0, 0, true, false },
	{ "fixeddiv", PCD_NOP, PCD_FIXEDDIV, 2, 0, 0, true, false },
// [BC] Start of new pcodes
	{ "playerblueskull", PCD_NOP, PCD_PLAYERBLUESKULL, 0, 0, 0, true, false },
	{ "playerredskull", PCD_NOP, PCD_PLAYERREDSKULL, 0, 0, 0, true, false },
	{ "playeryellowskull", PCD_NOP, PCD_PLAYERYELLOWSKULL, 0, 0, 0, true, false },
	{ "playerbluecard", PCD_NOP, PCD_PLAYERBLUECARD, 0, 0, 0, true, false },
	{ "playerredcard", PCD_NOP, PCD_PLAYERREDCARD, 0, 0, 0, true, false },
	{ "playeryellowcard", PCD_NOP, PCD_PLAYERYELLOWCARD, 0, 0, 0, true, false },
	{ "playeronteam", PCD_NOP, PCD_PLAYERONTEAM, 0, 0, 0, true, false },
	{ "playerteam", PCD_NOP, PCD_PLAYERTEAM, 0, 0, 0, true, false },
	{ "playerfrags", PCD_NOP, PCD_PLAYERFRAGS, 0, 0, 0, true, false },
	{ "playerhealth", PCD_NOP, PCD_PLAYERHEALTH, 0, 0, 0, true, false },
	{ "playerarmorpoints", PCD_NOP, PCD_PLAYERARMORPOINTS, 0, 0, 0, true, false },
	{ "playerexpert", PCD_NOP, PCD_PLAYEREXPERT, 0, 0, 0, true, false },
	{ "bluecount", PCD_NOP, PCD_BLUETEAMCOUNT, 0, 0, 0, true, false },
	{ "redcount", PCD_NOP, PCD_REDTEAMCOUNT, 0, 0, 0, true, false },
	{ "bluescore", PCD_NOP, PCD_BLUETEAMSCORE, 0, 0, 0, true, false },
	{ "redscore", PCD_NOP, PCD_REDTEAMSCORE, 0, 0, 0, true, false },
	{ "isoneflagctf", PCD_NOP, PCD_ISONEFLAGCTF, 0, 0, 0, true, false },
	{ "getinvasionwave", PCD_NOP, PCD_GETINVASIONWAVE, 0, 0, 0, true, false },
	{ "getinvasionstate", PCD_NOP, PCD_GETINVASIONSTATE, 0, 0, 0, true, false },
	{ "music_change", PCD_NOP, PCD_MUSICCHANGE, 2, 0, 0, false, false },
	{ "consolecommand", PCD_CONSOLECOMMANDDIRECT, PCD_CONSOLECOMMAND, 3, 2|4, 0, false, false },
	{ "singleplayer", PCD_NOP, PCD_SINGLEPLAYER, 0, 0, 0, true, false },
// [RH] end of Skull Tag functions
	{ "setgravity", PCD_SETGRAVITYDIRECT, PCD_SETGRAVITY, 1, 0, 0, false, false },
	{ "setaircontrol", PCD_SETAIRCONTROLDIRECT, PCD_SETAIRCONTROL, 1, 0, 0, false, false },
	{ "clearinventory", PCD_NOP, PCD_CLEARINVENTORY, 0, 0, 0, false, false },
	{ "giveinventory", PCD_GIVEINVENTORYDIRECT, PCD_GIVEINVENTORY, 2, 0, 0, false, false },
	{ "takeinventory", PCD_TAKEINVENTORYDIRECT, PCD_TAKEINVENTORY, 2, 0, 0, false, false },
	{ "checkinventory", PCD_CHECKINVENTORYDIRECT, PCD_CHECKINVENTORY, 1, 0, 0, true, false },
	{ "clearactorinventory", PCD_NOP, PCD_CLEARACTORINVENTORY, 1, 0, 0, false, false },
	{ "giveactorinventory", PCD_NOP, PCD_GIVEACTORINVENTORY, 3, 0, 0, false, false },
	{ "takeactorinventory", PCD_NOP, PCD_TAKEACTORINVENTORY, 3, 0, 0, false, false },
	{ "checkactorinventory", PCD_NOP, PCD_CHECKACTORINVENTORY, 2, 0, 0, true, false },
	{ "spawn", PCD_SPAWNDIRECT, PCD_SPAWN, 6, 16|32, 0, true, false },
	{ "spawnspot", PCD_SPAWNSPOTDIRECT, PCD_SPAWNSPOT, 4, 4|8, 0, true, false },
	{ "spawnspotfacing", PCD_NOP, PCD_SPAWNSPOTFACING, 3, 4, 0, true, false },
	{ "setmusic", PCD_SETMUSICDIRECT, PCD_SETMUSIC, 3, 2|4, 0, false, false },
	{ "localsetmusic", PCD_LOCALSETMUSICDIRECT, PCD_LOCALSETMUSIC, 3, 2|4, 0, false, false },
	{ "setstyle", PCD_SETSTYLEDIRECT, PCD_SETSTYLE, 1, 0, 0, false, false },
	{ "setfont", PCD_SETFONTDIRECT, PCD_SETFONT, 1, 0, 0, false, false },
	{ "setthingspecial", PCD_NOP, PCD_SETTHINGSPECIAL, 7, 4|8|16|32|64, 0, false, false },
	{ "fadeto", PCD_NOP, PCD_FADETO, 5, 0, 0, false, false },
	{ "faderange", PCD_NOP, PCD_FADERANGE, 9, 0, 0, false, false },
	{ "cancelfade", PCD_NOP, PCD_CANCELFADE, 0, 0, 0, false, false },
	{ "playmovie", PCD_NOP, PCD_PLAYMOVIE, 1, 0, 0, true, false },
	{ "setfloortrigger", PCD_NOP, PCD_SETFLOORTRIGGER, 8, 8|16|32|64|128, 0, false, false },
	{ "setceilingtrigger", PCD_NOP, PCD_SETCEILINGTRIGGER, 8, 8|16|32|64|128, 0, false, false },
	{ "setactorposition", PCD_NOP, PCD_SETACTORPOSITION, 5, 0, 0, true, false },
	{ "getactorx", PCD_NOP, PCD_GETACTORX, 1, 0, 0, true, false },
	{ "getactory", PCD_NOP, PCD_GETACTORY, 1, 0, 0, true, false },
	{ "getactorz", PCD_NOP, PCD_GETACTORZ, 1, 0, 0, true, false },
	{ "getactorfloorz", PCD_NOP, PCD_GETACTORFLOORZ, 1, 0, 0, true, false },
	{ "getactorceilingz", PCD_NOP, PCD_GETACTORCEILINGZ, 1, 0, 0, true, false },
	{ "getactorangle", PCD_NOP, PCD_GETACTORANGLE, 1, 0, 0, true, false },
	{ "writetoini", PCD_NOP, PCD_WRITETOINI, 3, 0, 0, false, false },
	{ "getfromini", PCD_NOP, PCD_GETFROMINI, 3, 0, 0, true, false },
	{ "sin", PCD_NOP, PCD_SIN, 1, 0, 0, true, false },
	{ "cos", PCD_NOP, PCD_COS, 1, 0, 0, true, false },
	{ "vectorangle", PCD_NOP, PCD_VECTORANGLE, 2, 0, 0, true, false },
	{ "checkweapon", PCD_NOP, PCD_CHECKWEAPON, 1, 0, 0, true, false },
	{ "setweapon", PCD_NOP, PCD_SETWEAPON, 1, 0, 0, true, false },
	{ "setmarineweapon", PCD_NOP, PCD_SETMARINEWEAPON, 2, 0, 0, false, false },
	{ "setactorproperty", PCD_NOP, PCD_SETACTORPROPERTY, 3, 0, 0, false, false },
	{ "getactorproperty", PCD_NOP, PCD_GETACTORPROPERTY, 2, 0, 0, true, false },
	{ "playernumber", PCD_NOP, PCD_PLAYERNUMBER, 0, 0, 0, true, false },
	{ "activatortid", PCD_NOP, PCD_ACTIVATORTID, 0, 0, 0, true, false },
	{ "setmarinesprite", PCD_NOP, PCD_SETMARINESPRITE, 2, 0, 0, false, false },
	{ "getscreenwidth", PCD_NOP, PCD_GETSCREENWIDTH, 0, 0, 0, true, false },
	{ "getscreenheight", PCD_NOP, PCD_GETSCREENHEIGHT, 0, 0, 0, true, false },
	{ "thing_projectile2", PCD_NOP, PCD_THING_PROJECTILE2, 7, 0, 0, false, false },
	{ "strlen", PCD_NOP, PCD_STRLEN, 1, 0, 0, true, false },
	{ "sethudsize", PCD_NOP, PCD_SETHUDSIZE, 3, 0, 0, false, false },
	{ "getcvar", PCD_NOP, PCD_GETCVAR, 1, 0, 0, true, false },
	{ "setresultvalue", PCD_NOP, PCD_SETRESULTVALUE, 1, 0, 0, false, false },
	{ "getlinerowoffset", PCD_NOP, PCD_GETLINEROWOFFSET, 0, 0, 0, true, false },
	{ "getsectorfloorz", PCD_NOP, PCD_GETSECTORFLOORZ, 3, 0, 0, true, false },
	{ "getsectorceilingz", PCD_NOP, PCD_GETSECTORCEILINGZ, 3, 0, 0, true, false },
	{ "getsigilpieces", PCD_NOP, PCD_GETSIGILPIECES, 0, 0, 0, true, false },
	{ "getlevelinfo", PCD_NOP, PCD_GETLEVELINFO, 1, 0, 0, true, false },
	{ "changesky", PCD_NOP, PCD_CHANGESKY, 2, 0, 0, false, false },
	{ "playeringame", PCD_NOP, PCD_PLAYERINGAME, 1, 0, 0, true, false },
	{ "playerisbot", PCD_NOP, PCD_PLAYERISBOT, 1, 0, 0, true, false },
	{ "setcameratotexture", PCD_NOP, PCD_SETCAMERATOTEXTURE, 3, 0, 0, false, false },
	{ "grabinput", PCD_NOP, PCD_GRABINPUT, 2, 0, 0, false, false },
	{ "setmousepointer", PCD_NOP, PCD_SETMOUSEPOINTER, 3, 0, 0, false, false },
	{ "movemousepointer", PCD_NOP, PCD_MOVEMOUSEPOINTER, 2, 0, 0, false, false },
	{ "getammocapacity", PCD_NOP, PCD_GETAMMOCAPACITY, 1, 0, 0, true, false },
	{ "setammocapacity", PCD_NOP, PCD_SETAMMOCAPACITY, 2, 0, 0, false, false },
	{ "setactorangle", PCD_NOP, PCD_SETACTORANGLE, 2, 0, 0, false, false },
	{ "spawnprojectile", PCD_NOP, PCD_SPAWNPROJECTILE, 7, 0, 0, false, false },
	{ "getsectorlightlevel", PCD_NOP, PCD_GETSECTORLIGHTLEVEL, 1, 0, 0, true, false },
	{ "playerclass", PCD_NOP, PCD_PLAYERCLASS, 1, 0, 0, true, false },
	{ "getplayerinfo", PCD_NOP, PCD_GETPLAYERINFO, 2, 0, 0, true, false },
	{ "changelevel", PCD_NOP, PCD_CHANGELEVEL, 4, 8, 0, false, false },
	{ "sectordamage", PCD_NOP, PCD_SECTORDAMAGE, 5, 0, 0, false, false },
	{ "replacetextures", PCD_NOP, PCD_REPLACETEXTURES, 3, 4, 0, false, false },
	{ "getactorpitch", PCD_NOP, PCD_GETACTORPITCH, 1, 0, 0, true, false },
	{ "setactorpitch", PCD_NOP, PCD_SETACTORPITCH, 2, 0, 0, false, false },
	{ "setactorstate", PCD_NOP, PCD_SETACTORSTATE, 3, 4, 0, true, false },
	{ "thing_damage2", PCD_NOP, PCD_THINGDAMAGE2, 3, 0, 0, true, false },
	{ "useinventory", PCD_NOP, PCD_USEINVENTORY, 1, 0, 0, true, false },
	{ "useactorinventory", PCD_NOP, PCD_USEACTORINVENTORY, 2, 0, 0, true, false },
	{ "checkactorceilingtexture", PCD_NOP, PCD_CHECKACTORCEILINGTEXTURE, 2, 0, 0, true, false },
	{ "checkactorfloortexture", PCD_NOP, PCD_CHECKACTORFLOORTEXTURE, 2, 0, 0, true, false },
	{ "getactorlightlevel", PCD_NOP, PCD_GETACTORLIGHTLEVEL, 1, 0, 0, true, false },
	{ "setmugshotstate", PCD_NOP, PCD_SETMUGSHOTSTATE, 1, 0, 0, false, false },
	{ "thingcountsector", PCD_NOP, PCD_THINGCOUNTSECTOR, 3, 0, 0, true, false },
	{ "thingcountnamesector", PCD_NOP, PCD_THINGCOUNTNAMESECTOR, 3, 0, 0, true, false },
	{ "checkplayercamera", PCD_NOP, PCD_CHECKPLAYERCAMERA, 1, 0, 0, true, false },
	{ "morphactor", PCD_NOP, PCD_MORPHACTOR, 7, 2|4|8|16|32|64, 0, true, false },
	{ "unmorphactor", PCD_NOP, PCD_UNMORPHACTOR, 2, 2, 0, true, false },
	{ "getplayerinput", PCD_NOP, PCD_GETPLAYERINPUT, 2, 0, 0, true, false },
	{ "classifyactor", PCD_NOP, PCD_CLASSIFYACTOR, 1, 0, 0, true, false },
	
	{ "", PCD_NOP, PCD_NOP, 0, 0, 0, false, false }
};

// TODO: make _DEBUG only
static char *SymbolTypeNames[] =
{
	"SY_DUMMY",
	"SY_LABEL",
	"SY_SCRIPTVAR",
	"SY_SCRIPTALIAS",
	"SY_MAPVAR",
	"SY_WORLDVAR",
	"SY_GLOBALVAR",
	"SY_MAPARRAY",
	"SY_WORLDARRAY",
	"SY_GLOBALARRAY",
	"SY_SPECIAL",
	"SY_CONSTANT",
	"SY_INTERNFUNC",
	"SY_SCRIPTFUNC"
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// sym_Init
//
// Initializes the predefined symbols
//
//==========================================================================
void sym_Init()
{
	//Add std types
	ACS_TypeDef("void");
	ACS_TypeDef("int");
	ACS_TypeDef("bool");
	ACS_TypeDef("str");

	//Add std depth
	ACS_DepthRoot(0);

	//Add internal functions
	for each (ACS_Function def in InternalFunctions)
		acs_Nodes.add(ACS_Node(NODE_FUNCTION, def.index));
}

//==========================================================================
//
// SY_Find
//
//==========================================================================
ACS_Node *SY_Find(string name)
{
	ACS_Node *node;

	//[JRT] Shouldn't it check local first?
	if ((node = SY_FindLocal(name)) == NULL)
	{
		return SY_FindGlobal(name);
	}
	return node;
}

//==========================================================================
//
// SY_FindGlobal
//
//==========================================================================
ACS_Node *SY_FindGlobal(string name)
{
	ACS_Node *node = Find(name, 0);
	if(node)
	{
		MS_Message(MSG_DEBUG, "Symbol %s marked as used.\n", name);

		if(node->type == NODE_FUNCTION)
		{
			PC_AddFunction(node);
		}
		else if (node->type == NODE_VARIABLE)
		{
			if(pa_MapVarCount >= MAX_MAP_VARIABLES)
				ERR_Error(ERR_TOO_MANY_MAP_VARS, true);
			
			else
			{
				pa_MapVarCount++;
				PC_NameMapVariable(node->index, node);
			}
		}
		else if(node->type == NODE_ARRAY)
		{
			if(pa_MapVarCount >= MAX_MAP_VARIABLES)
				ERR_Error(ERR_TOO_MANY_MAP_VARS, true);
			
			else
			{
				pa_MapVarCount++;
				PC_NameMapVariable(node->index, node);
				PC_AddArray(node->index, node->arr->size);
			}
		}
	}
	return node;
}

//==========================================================================
//
// SY_Findlocal
//
//==========================================================================
ACS_Node *SY_FindLocal(string name)
{
	return Find(name, CurrentDepthIndex);
}

//==========================================================================
//
// Find
//
//==========================================================================
static ACS_Node *Find(string name, int depth)
{
	for each(ACS_Node &node in acs_Nodes)
	{
		if(node.name().compare(name) == 0)
			if(acs_Depths[node.depth].current == depth)
				return &node;
			else
			{
				int cur = acs_Depths[node.depth].current;
				while (cur != 0)
				{
					cur = acs_Depths[node.depth].parent;
					if (cur == depth)
						return &node;
				}
			}
	}
	return NULL;
}

//==========================================================================
//
// SY_InsertLocal
//
//==========================================================================
ACS_Node *SY_InsertLocal(string name, int type)
{
	if(Find(name, 0))
		ERR_Error(ERR_LOCAL_VAR_SHADOWED, true);
	
	MS_Message(MSG_DEBUG, "Inserting local identifier: %s (%s)\n", name, SymbolTypeNames[type]);

	return Insert(name, type, CurrentDepthIndex);
}

//==========================================================================
//
// SY_InsertGlobal
//
//==========================================================================
ACS_Node *SY_InsertGlobal(string name, int type)
{
	MS_Message(MSG_DEBUG, "Inserting global identifier: %s (%s)\n", name, SymbolTypeNames[type]);
	return Insert(name, type, 0);
}

//==========================================================================
//
// SY_InsertGlobalUnique
//
//==========================================================================
ACS_Node *SY_InsertGlobalUnique(string name, int type)
{
	if(SY_FindGlobal(name))
	{ // Redefined
		ERR_Exit(ERR_REDEFINED_IDENTIFIER, true, name);
	}
	return SY_InsertGlobal(name, type);
}

//==========================================================================
//
// Insert
//
//==========================================================================
static ACS_Node *Insert(string name, int type)
{
	int compare;
	ACS_Node *node;

	node = new ACS_Node(type, name);
	node->imported = ImportMode == IMPORT_Importing;

	acs_Nodes.add(*node);
	return(acs_Nodes.lastAdded);
}

//==========================================================================
//
// SY_FreeLocals
//
//==========================================================================
void SY_FreeLocals()
{
	for each (ACS_Node &node in acs_Nodes)
	{
		if(node.depth == CurrentDepthIndex)
			FreeNodes(&node);
		//delete node? why? It's not like an acs script is going to eat a gig of memory
	}
	MS_Message(MSG_DEBUG, "Freeing local identifiers\n");
	(LocalRoot);
	LocalRoot = NULL;
}

//==========================================================================
//
// SY_FreeGlobals
//
//==========================================================================
void SY_FreeGlobals()
{
	MS_Message(MSG_DEBUG, "Freeing global identifiers\n");
	for each (ACS_Node &node in acs_Nodes)
		FreeNodes(&node);
}

//==========================================================================
//
// FreeNodes
//
//==========================================================================
static void FreeNodes(ACS_Node *root)
{
	if(root == NULL)
		return;
	
	free(root->name);
	free(root);
}

//==========================================================================
//
// SY_FreeConstants
//
//==========================================================================
void SY_FreeConstants(int depth)
{
	MS_Message(MSG_DEBUG, "Freeing constants for depth %d\n", depth);
	FreeNodesAtDepth(depth);
}

//==========================================================================
//
// DeleteNode
//
//==========================================================================
/*
static void DeleteNode(symbolNode_t *node, symbolNode_t **parent_p)
{
	symbolNode_t **temp;
	char *nametemp;

	if(node->left == NULL)
	{
		*parent_p = node->right;
		free(node->name);
		free(node);
	}
	else if(node->right == NULL)
	{
		*parent_p = node->left;
		free(node->name);
		free(node);
	}
	else
	{
		// "Randomly" pick the in-order successor or predecessor to take
		// the place of the deleted node.
		if(rand() & 1)
		{
			// predecessor
			temp = &node->left;
			while((*temp)->right != NULL)
			{
				temp = &(*temp)->right;
			}
		}
		else
		{
			// successor
			temp = &node->right;
			while((*temp)->left != NULL)
			{
				temp = &(*temp)->left;
			}
		}
		nametemp = node->name;
		node->name = (*temp)->name;
		(*temp)->name = nametemp;
		node->type = (*temp)->type;
		node->unused = (*temp)->unused;
		node->imported = (*temp)->imported;
		node->info = (*temp)->info;
		DeleteNode(*temp, temp);
	}
}
*/
//==========================================================================
//
// SY_ClearShared
//
//==========================================================================

void SY_ClearShared()
{
	MS_Message(MSG_DEBUG, "Marking library exports as unused\n");
	ClearShared(0);
}

//==========================================================================
//
// ClearShared
//
//==========================================================================

static void ClearShared(symbolNode_t *root)
{
	while(root != NULL)
	{
		if( root->type == SY_SCRIPTFUNC ||
			root->type == SY_MAPVAR ||
			root->type == SY_MAPARRAY)
		{
			root->unused = true;
		}
		ClearShared(root->left);
		root = root->right;
	}
}
