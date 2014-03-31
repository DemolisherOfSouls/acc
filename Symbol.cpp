//**************************************************************************
//**
//** symbol.cpp
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "common.h"
#include "symbol.h"
#include "misc.h"
#include "parse.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static ACS_Node *Find(string name, ACS_Node *root);
static ACS_Node *Insert(string name, int type, ACS_Node **root);
static void FreeNodes(ACS_Node *root);
static void FreeNodesAtDepth(int depth);
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

#ifdef _DEBUG
static string SymbolTypeNames[] =
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
#endif

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
	ACS_TypeDef::Init("void");
	ACS_TypeDef::Init("int");
	ACS_TypeDef::Init("bool");
	ACS_TypeDef::Init("str");

	//Add std depth
	ACS_DepthRoot::Init(0);

	//Add internal functions
	for (ACS_Function def : InternalFunctions)
		sym_Nodes.add(ACS_Node(NODE_FUNCTION, def.index));
}

//==========================================================================
//
// sym_Find
//
//==========================================================================
ACS_Node *sym_Find(string name)
{
	ACS_Node * node = sym_FindLocal(name);

	//[JRT] Shouldn't it check local first?
	if (!node)
		return sym_FindGlobal(name);
	
	return node;
}

//==========================================================================
//
// sym_FindGlobal
//
//==========================================================================
ACS_Node *sym_FindGlobal(string name)
{
	ACS_Node *node = Find(name, pa_FileDepth);
	if(node)
	{
		Message(MSG_DEBUG, "Symbol " + name + " marked as used.");

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
				PC_NameMapVariable(node->index(), node);
			}
		}
		else if(node->type == NODE_ARRAY)
		{
			if(pa_MapVarCount >= MAX_MAP_VARIABLES)
				ERR_Error(ERR_TOO_MANY_MAP_VARS, true);
			
			else
			{
				pa_MapVarCount++;
				PC_NameMapVariable(node->index(), node);
				PC_AddArray(node->index, node->arr->size);
			}
		}
	}
	return node;
}

//==========================================================================
//
// sym_Findlocal
//
//==========================================================================
ACS_Node *sym_FindLocal(string name)
{
	auto *depth = &sym_Depths[pa_CurrentDepth];

	while (depth->Current() != pa_FileDepth)
	{
		ACS_Node * node = Find(name, depth);

		if (!node)	// pop depth, re-check;
			depth = depth

	}
}

//==========================================================================
//
// Find
//
//==========================================================================
static ACS_Node *Find(string name, int depth)
{
	for (ACS_Node &node : acs_Nodes)
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
	ACS_Node *node = new ACS_Node(type, name);

	node->isImported(ImportMode == IMPORT_Importing);

	acs_Nodes.add(*node);
	return(&acs_Nodes.lastAdded());
}

//==========================================================================
//
// SY_FreeLocals
//
//==========================================================================
void SY_FreeLocals()
{
	for (ACS_Node &node : acs_Nodes)
	{
		if(node.depth == pa_CurrentDepth)
			FreeNodes(&node);
		//delete node? why? It's not like an acs script is going to eat a gig of memory
	}
	Message(MSG_DEBUG, "Freeing local identifiers");
}

//==========================================================================
//
// SY_FreeGlobals
//
//==========================================================================
void SY_FreeGlobals()
{
	Message(MSG_DEBUG, "Freeing global identifiers");
	for (ACS_Node &node : acs_Nodes)
		FreeNodes(&node);
}

//==========================================================================
//
// FreeNodes
//
//==========================================================================
static void FreeNodes(ACS_Node *root)
{
	delete root;
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
	Message(MSG_DEBUG, "Marking library exports as unused");
	ClearShared(0);
}

//==========================================================================
//
// ClearShared
//
//==========================================================================

static void ClearShared(ACS_Node *node)
{
	for (ACS_Node &iter : acs_Nodes)
	{
		if (node->depth == iter.depth)
			iter.clear();
	}
}
