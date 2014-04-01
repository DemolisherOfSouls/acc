//**************************************************************************
//**
//** pcode.cpp
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "pcode.h"
#include "common.h"
#include "error.h"
#include "misc.h"
#include "strlist.h"
#include "token.h"
#include "symbol.h"
#include "parse.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void pCode_CommandLog(int location, int code, string prefix);
static void CloseOld();
static void CloseNew();
static void CreateDummyScripts();
static void RecordDummyScripts();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static bool ObjectOpened = false;
static ACS_Script ScriptInfo[MAX_SCRIPT_COUNT];
static ACS_Function FunctionInfo[MAX_FUNCTION_COUNT];
static int ArraySizes[MAX_MAP_VARIABLES];					// TODO: Remove ArraySizes[]
static int *ArrayInits[MAX_MAP_VARIABLES];					// TODO: Remove *ArrayInits[]
static bool ArrayOfStrings[MAX_MAP_VARIABLES];				// TODO: Remove ArrayOfStrings[]
static int NumArrays;
static bool MapVariablesInit = false;
static string ObjectName;
static int ObjectFlags;
static int PushByteAddr;
static auto Imports = vector<string>(MAX_IMPORTS);
static bool HaveExtendedScripts;


//This is so that ACC doesn't have to carry around these strings
//unless someone is actually developing it, and requires them

#if _DEBUG
static string pCode_Names[PCODE_COMMAND_COUNT]
{
	"PCD_NOP",
	"PCD_TERMINATE",
	"PCD_SUSPEND",
	"PCD_PUSHNUMBER",
	"PCD_LSPEC1",
	"PCD_LSPEC2",
	"PCD_LSPEC3",
	"PCD_LSPEC4",
	"PCD_LSPEC5",
	"PCD_LSPEC1DIRECT",
	"PCD_LSPEC2DIRECT",
	"PCD_LSPEC3DIRECT",
	"PCD_LSPEC4DIRECT",
	"PCD_LSPEC5DIRECT",
	"PCD_ADD",
	"PCD_SUBTRACT",
	"PCD_MULTIPLY",
	"PCD_DIVIDE",
	"PCD_MODULUS",
	"PCD_EQ",
	"PCD_NE",
	"PCD_LT",
	"PCD_GT",
	"PCD_LE",
	"PCD_GE",
	"PCD_ASSIGNSCRIPTVAR",
	"PCD_ASSIGNMAPVAR",
	"PCD_ASSIGNWORLDVAR",
	"PCD_PUSHSCRIPTVAR",
	"PCD_PUSHMAPVAR",
	"PCD_PUSHWORLDVAR",
	"PCD_ADDSCRIPTVAR",
	"PCD_ADDMAPVAR",
	"PCD_ADDWORLDVAR",
	"PCD_SUBSCRIPTVAR",
	"PCD_SUBMAPVAR",
	"PCD_SUBWORLDVAR",
	"PCD_MULSCRIPTVAR",
	"PCD_MULMAPVAR",
	"PCD_MULWORLDVAR",
	"PCD_DIVSCRIPTVAR",
	"PCD_DIVMAPVAR",
	"PCD_DIVWORLDVAR",
	"PCD_MODSCRIPTVAR",
	"PCD_MODMAPVAR",
	"PCD_MODWORLDVAR",
	"PCD_INCSCRIPTVAR",
	"PCD_INCMAPVAR",
	"PCD_INCWORLDVAR",
	"PCD_DECSCRIPTVAR",
	"PCD_DECMAPVAR",
	"PCD_DECWORLDVAR",
	"PCD_GOTO",
	"PCD_IFGOTO",
	"PCD_DROP",
	"PCD_DELAY",
	"PCD_DELAYDIRECT",
	"PCD_RANDOM",
	"PCD_RANDOMDIRECT",
	"PCD_THINGCOUNT",
	"PCD_THINGCOUNTDIRECT",
	"PCD_TAGWAIT",
	"PCD_TAGWAITDIRECT",
	"PCD_POLYWAIT",
	"PCD_POLYWAITDIRECT",
	"PCD_CHANGEFLOOR",
	"PCD_CHANGEFLOORDIRECT",
	"PCD_CHANGECEILING",
	"PCD_CHANGECEILINGDIRECT",
	"PCD_RESTART",
	"PCD_ANDLOGICAL",
	"PCD_ORLOGICAL",
	"PCD_ANDBITWISE",
	"PCD_ORBITWISE",
	"PCD_EORBITWISE",
	"PCD_NEGATELOGICAL",
	"PCD_LSHIFT",
	"PCD_RSHIFT",
	"PCD_UNARYMINUS",
	"PCD_IFNOTGOTO",
	"PCD_LINESIDE",
	"PCD_SCRIPTWAIT",
	"PCD_SCRIPTWAITDIRECT",
	"PCD_CLEARLINESPECIAL",
	"PCD_CASEGOTO",
	"PCD_BEGINPRINT",
	"PCD_ENDPRINT",
	"PCD_PRINTSTRING",
	"PCD_PRINTNUMBER",
	"PCD_PRINTCHARACTER",
	"PCD_PLAYERCOUNT",
	"PCD_GAMETYPE",
	"PCD_GAMESKILL",
	"PCD_TIMER",
	"PCD_SECTORSOUND",
	"PCD_AMBIENTSOUND",
	"PCD_SOUNDSEQUENCE",
	"PCD_SETLINETEXTURE",
	"PCD_SETLINEBLOCKING",
	"PCD_SETLINESPECIAL",
	"PCD_THINGSOUND",
	"PCD_ENDPRINTBOLD",
	// [RH] End of Hexen p-codes
	"PCD_ACTIVATORSOUND",
	"PCD_LOCALAMBIENTSOUND",
	"PCD_SETLINEMONSTERBLOCKING",
	// [BC] Start of new pcodes
	"PCD_PLAYERBLUESKULL",
	"PCD_PLAYERREDSKULL",
	"PCD_PLAYERYELLOWSKULL",
	"PCD_PLAYERMASTERSKULL",
	"PCD_PLAYERBLUECARD",
	"PCD_PLAYERREDCARD",
	"PCD_PLAYERYELLOWCARD",
	"PCD_PLAYERMASTERCARD",
	"PCD_PLAYERBLACKSKULL",
	"PCD_PLAYERSILVERSKULL",
	"PCD_PLAYERGOLDSKULL",
	"PCD_PLAYERBLACKCARD",
	"PCD_PLAYERSILVERCARD",
	"PCD_PLAYERONTEAM",
	"PCD_PLAYERTEAM",
	"PCD_PLAYERHEALTH",
	"PCD_PLAYERARMORPOINTS",
	"PCD_PLAYERFRAGS",
	"PCD_PLAYEREXPERT",
	"PCD_BLUETEAMCOUNT",
	"PCD_REDTEAMCOUNT",
	"PCD_BLUETEAMSCORE",
	"PCD_REDTEAMSCORE",
	"PCD_ISONEFLAGCTF",
	"PCD_LSPEC6",
	"PCD_LSPEC6DIRECT",
	"PCD_PRINTNAME",
	"PCD_MUSICCHANGE",
	"PCD_CONSOLECOMMANDDIRECT",
	"PCD_CONSOLECOMMAND",
	"PCD_SINGLEPLAYER",
	// [RH] End of Skull Tag p-codes
	"PCD_FIXEDMUL",
	"PCD_FIXEDDIV",
	"PCD_SETGRAVITY",
	"PCD_SETGRAVITYDIRECT",
	"PCD_SETAIRCONTROL",
	"PCD_SETAIRCONTROLDIRECT",
	"PCD_CLEARINVENTORY",
	"PCD_GIVEINVENTORY",
	"PCD_GIVEINVENTORYDIRECT",
	"PCD_TAKEINVENTORY",
	"PCD_TAKEINVENTORYDIRECT",
	"PCD_CHECKINVENTORY",
	"PCD_CHECKINVENTORYDIRECT",
	"PCD_SPAWN",
	"PCD_SPAWNDIRECT",
	"PCD_SPAWNSPOT",
	"PCD_SPAWNSPOTDIRECT",
	"PCD_SETMUSIC",
	"PCD_SETMUSICDIRECT",
	"PCD_LOCALSETMUSIC",
	"PCD_LOCALSETMUSICDIRECT",
	"PCD_PRINTFIXED",
	"PCD_PRINTLOCALIZED",
	"PCD_MOREHUDMESSAGE",
	"PCD_OPTHUDMESSAGE",
	"PCD_ENDHUDMESSAGE",
	"PCD_ENDHUDMESSAGEBOLD",
	"PCD_SETSTYLE",
	"PCD_SETSTYLEDIRECT",
	"PCD_SETFONT",
	"PCD_SETFONTDIRECT",
	"PCD_PUSHBYTE",
	"PCD_LSPEC1DIRECTB",
	"PCD_LSPEC2DIRECTB",
	"PCD_LSPEC3DIRECTB",
	"PCD_LSPEC4DIRECTB",
	"PCD_LSPEC5DIRECTB",
	"PCD_DELAYDIRECTB",
	"PCD_RANDOMDIRECTB",
	"PCD_PUSHBYTES",
	"PCD_PUSH2BYTES",
	"PCD_PUSH3BYTES",
	"PCD_PUSH4BYTES",
	"PCD_PUSH5BYTES",
	"PCD_SETTHINGSPECIAL",
	"PCD_ASSIGNGLOBALVAR",
	"PCD_PUSHGLOBALVAR",
	"PCD_ADDGLOBALVAR",
	"PCD_SUBGLOBALVAR",
	"PCD_MULGLOBALVAR",
	"PCD_DIVGLOBALVAR",
	"PCD_MODGLOBALVAR",
	"PCD_INCGLOBALVAR",
	"PCD_DECGLOBALVAR",
	"PCD_FADETO",
	"PCD_FADERANGE",
	"PCD_CANCELFADE",
	"PCD_PLAYMOVIE",
	"PCD_SETFLOORTRIGGER",
	"PCD_SETCEILINGTRIGGER",
	"PCD_GETACTORX",
	"PCD_GETACTORY",
	"PCD_GETACTORZ",
	"PCD_STARTTRANSLATION",
	"PCD_TRANSLATIONRANGE1",
	"PCD_TRANSLATIONRANGE2",
	"PCD_ENDTRANSLATION",
	"PCD_CALL",
	"PCD_CALLDISCARD",
	"PCD_RETURNVOID",
	"PCD_RETURNVAL",
	"PCD_PUSHMAPARRAY",
	"PCD_ASSIGNMAPARRAY",
	"PCD_ADDMAPARRAY",
	"PCD_SUBMAPARRAY",
	"PCD_MULMAPARRAY",
	"PCD_DIVMAPARRAY",
	"PCD_MODMAPARRAY",
	"PCD_INCMAPARRAY",
	"PCD_DECMAPARRAY",
	"PCD_DUP",
	"PCD_SWAP",
	"PCD_WRITETOINI",
	"PCD_GETFROMINI",
	"PCD_SIN",
	"PCD_COS",
	"PCD_VECTORANGLE",
	"PCD_CHECKWEAPON",
	"PCD_SETWEAPON",
	"PCD_TAGSTRING",
	"PCD_PUSHWORLDARRAY",
	"PCD_ASSIGNWORLDARRAY",
	"PCD_ADDWORLDARRAY",
	"PCD_SUBWORLDARRAY",
	"PCD_MULWORLDARRAY",
	"PCD_DIVWORLDARRAY",
	"PCD_MODWORLDARRAY",
	"PCD_INCWORLDARRAY",
	"PCD_DECWORLDARRAY",
	"PCD_PUSHGLOBALARRAY",
	"PCD_ASSIGNGLOBALARRAY",
	"PCD_ADDGLOBALARRAY",
	"PCD_SUBGLOBALARRAY",
	"PCD_MULGLOBALARRAY",
	"PCD_DIVGLOBALARRAY",
	"PCD_MODGLOBALARRAY",
	"PCD_INCGLOBALARRAY",
	"PCD_DECGLOBALARRAY",
	"PCD_SETMARINEWEAPON",
	"PCD_SETACTORPROPERTY",
	"PCD_GETACTORPROPERTY",
	"PCD_PLAYERNUMBER",
	"PCD_ACTIVATORTID",
	"PCD_SETMARINESPRITE",
	"PCD_GETSCREENWIDTH",
	"PCD_GETSCREENHEIGHT",
	"PCD_THING_PROJECTILE2",
	"PCD_STRLEN",
	"PCD_SETHUDSIZE",
	"PCD_GETCVAR",
	"PCD_CASEGOTOSORTED",
	"PCD_SETRESULTVALUE",
	"PCD_GETLINEROWOFFSET",
	"PCD_GETACTORFLOORZ",
	"PCD_GETACTORANGLE",
	"PCD_GETSECTORFLOORZ",
	"PCD_GETSECTORCEILINGZ",
	"PCD_LSPEC5RESULT",
	"PCD_GETSIGILPIECES",
	"PCD_GELEVELINFO",
	"PCD_CHANGESKY",
	"PCD_PLAYERINGAME",
	"PCD_PLAYERISBOT",
	"PCD_SETCAMERATOTEXTURE",
	"PCD_ENDLOG",
	"PCD_GETAMMOCAPACITY",
	"PCD_SETAMMOCAPACITY",
	// [JB] start of new pcodes
	"PCD_PRINTMAPCHARARRAY",
	"PCD_PRINTWORLDCHARARRAY",
	"PCD_PRINTGLOBALCHARARRAY",
	// [JB] end of new pcodes
	"PCD_SETACTORANGLE",
	"PCD_GRABINPUT",
	"PCD_SETMOUSEPOINTER",
	"PCD_MOVEMOUSEPOINTER",
	"PCD_SPAWNPROJECTILE",
	"PCD_GETSECTORLIGHTLEVEL",
	"PCD_GETACTORCEILINGZ",
	"PCD_SETACTORPOSITION",
	"PCD_CLEARACTORINVENTORY",
	"PCD_GIVEACTORINVENTORY",
	"PCD_TAKEACTORINVENTORY",
	"PCD_CHECKACTORINVENTORY",
	"PCD_THINGCOUNTNAME",
	"PCD_SPAWNSPOTFACING",
	"PCD_PLAYERCLASS",
	//[MW] start my p-codes
	"PCD_ANDSCRIPTVAR",
	"PCD_ANDMAPVAR",
	"PCD_ANDWORLDVAR",
	"PCD_ANDGLOBALVAR",
	"PCD_ANDMAPARRAY",
	"PCD_ANDWORLDARRAY",
	"PCD_ANDGLOBALARRAY",
	"PCD_EORSCRIPTVAR",
	"PCD_EORMAPVAR",
	"PCD_EORWORLDVAR",
	"PCD_EORGLOBALVAR",
	"PCD_EORMAPARRAY",
	"PCD_EORWORLDARRAY",
	"PCD_EORGLOBALARRAY",
	"PCD_ORSCRIPTVAR",
	"PCD_ORMAPVAR",
	"PCD_ORWORLDVAR",
	"PCD_ORGLOBALVAR",
	"PCD_ORMAPARRAY",
	"PCD_ORWORLDARRAY",
	"PCD_ORGLOBALARRAY",
	"PCD_LSSCRIPTVAR",
	"PCD_LSMAPVAR",
	"PCD_LSWORLDVAR",
	"PCD_LSGLOBALVAR",
	"PCD_LSMAPARRAY",
	"PCD_LSWORLDARRAY",
	"PCD_LSGLOBALARRAY",
	"PCD_RSSCRIPTVAR",
	"PCD_RSMAPVAR",
	"PCD_RSWORLDVAR",
	"PCD_RSGLOBALVAR",
	"PCD_RSMAPARRAY",
	"PCD_RSWORLDARRAY",
	"PCD_RSGLOBALARRAY",
	//[MW] end my p-codes
	"PCD_GETPLAYERINFO",
	"PCD_CHANGELEVEL",
	"PCD_SECTORDAMAGE",
	"PCD_REPLACETEXTURES",
	"PCD_NEGATEBINARY",
	"PCD_GETACTORPITCH",
	"PCD_SETACTORPITCH",
	"PCD_PRINTBIND",
	"PCD_SETACTORSTATE",
	"PCD_THINGDAMAGE2",
	"PCD_USEINVENTORY",
	"PCD_USEACTORINVENTORY",
	"PCD_CHECKACTORCEILINGTEXTURE",
	"PCD_CHECKACTORFLOORTEXTURE",
	"PCD_GETACTORLIGHTLEVEL",
	"PCD_SETMUGSHOTSTATE",
	"PCD_THINGCOUNTSECTOR",
	"PCD_THINGCOUNTNAMESECTOR",
	"PCD_CHECKPLAYERCAMERA",
	"PCD_MORPHACTOR",
	"PCD_UNMORPHACTOR",
	"PCD_GETPLAYERINPUT",
	"PCD_CLASSIFYACTOR",
	"PCD_PRINTBINARY",
	"PCD_PRINTHEX",
	"PCD_CALLFUNC",
	"PCD_SAVESTRING",			// [FDARI]
	"PCD_PRINTMAPCHRANGE",		// [FDARI] output range
	"PCD_PRINTWORLDCHRANGE",
	"PCD_PRINTGLOBALCHRANGE",
	"PCD_STRCPYTOMAPCHRANGE",	// [FDARI] input range
	"PCD_STRCPYTOWORLDCHRANGE",
	"PCD_STRCPYTOGLOBALCHRANGE",
	"PCD_PUSHFUNCTION",			// from Eternity
	"PCD_CALLSTACK",			// from Eternity
	"PCD_SCRIPTWAITNAMED",
	"PCD_TRANSLATIONRANGE3",
};
#endif
static void pCode_CommandLog(int location, int code, string prefix)
{
#if _DEBUG
	MS_Message(MSG_DEBUG, prefix + "> %06d = #%d:%s\n", location, code, pCode_Names[code]);
#else
	MS_Message(MSG_DEBUG, prefix + "> %06d = #%d\n", location, pCode);
#endif
}


// CODE --------------------------------------------------------------------

//==========================================================================
//
// PC_OpenObject
//
//==========================================================================
void PC_OpenObject(string name, size_t size, int flags)
{
	if(ObjectOpened)
		PC_CloseObject();
	
	if(name.length() >= MAX_FILE_NAME_LENGTH)
		ERR_Exit(ERR_FILE_NAME_TOO_LONG, false, name);
	
	ObjectName = name;
	pCode_Buffer.resize(size);
	pCode_ByteSizes.resize(size);
	ObjectFlags = flags;
	pCode_ScriptCount = 0;
	ObjectOpened = true;
	pCode_Append("ACS");
	PC_SkipInt(); // Script table offset
}

//==========================================================================
//
// PC_CloseObject
//
//==========================================================================
void PC_CloseObject()
{
	Message(MSG_DEBUG, "---- PC_CloseObject ----");
	pCode_AppendPadding(4 - pCode_Size % 4);
	if (!pCode_NoShrink || (NumLanguages > 1) || (NumStringLists > 0) ||
		(pCode_FunctionCount > 0) || MapVariablesInit || NumArrays != 0 ||
		pCode_EncryptStrings || !Imports.empty || HaveExtendedScripts)
	{
		if (pCode_EnforceHexen)
		{
			ERR_Exit(ERR_NOT_HEXEN, false);
		}
		if (pCode_WarnNotHexen)
		{
			line();
			line("These scripts have been upgraded because they use new features.");
			line("They will not be compatible with Hexen.");
		}
		CloseNew();
	}
	else
	{
		CloseOld();
	}
	if(MS_SaveFile(ObjectName, pCode_Buffer.data) == false)
	{
		ERR_Exit(ERR_SAVE_OBJECT_FAILED, false);
	}
}

//==========================================================================
//
// CloseOld
//
//==========================================================================
static void CloseOld()
{
	STR_WriteStrings();
	pCode_Append(pCode_ScriptCount);
	for (scriptInfo_t &info : ScriptInfo)
	{
		MS_Message(MSG_DEBUG, "Script %d, address = %d, arg count = %d\n",
			info.number, info.address, info.argCount);
		pCode_Append((info.number + info.type * 1000));
		pCode_Append(info.address);
		pCode_Append((int)info.argCount);
	}
	STR_WriteList();
}

//==========================================================================
//
// CloseNew
//
// Creates a new-format ACS file. For programs that don't know any better,
// this will look just like an old ACS file with no scripts or strings but
// with some extra junk in the middle. Both WadAuthor and DeePsea will not
// accept ACS files that do not have the ACS\0 header. Worse, WadAuthor
// will hang if the file begins with ACS\0 but does not look like something
// that might have been created with Raven's ACC. Thus, the chunks live in
// the string block, and there are two 0 dwords at the end of the file.
//
//==========================================================================

static void CloseNew()
{
	int i, j, count;
	int chunkStart;

	if (pCode_WadAuthor)
	{
		CreateDummyScripts();
	}

	chunkStart = pCode_Current;

	// Only write out those scripts that this acs file actually provides.
	for (i = j = 0; i < pCode_ScriptCount; ++i)
	{
		if(!ScriptInfo[i].isImported)
		{
			++j;
		}
	}
	if(j > 0)
	{
		PC_Append("SPTR", 4);
		PC_AppendInt(j * 8);
		for (i = 0; i < pCode_ScriptCount; i++)
		{
			ACS_Script *info = &ScriptInfo[i];
			if(!info->imported)
			{
				MS_Message(MSG_DEBUG, "Script %d, address = %d, arg count = %d\n",
					info->number, info->address, info->argCount);
				PC_AppendWord(info->number);
				PC_AppendByte(info->type);
				PC_AppendByte(info->argCount);
				PC_AppendInt(info->address);
			}
		}
	}

	// If any scripts have more than the maximum number of arguments, output them.
	for (i = j = 0; i < pCode_ScriptCount; ++i)
	{
		if(!ScriptInfo[i].imported && ScriptInfo[i].varCount > MAX_SCRIPT_VARIABLES)
		{
			++j;
		}
	}
	if(j > 0)
	{
		PC_Append("SVCT", 4);
		PC_AppendInt(j * 4);
		for (i = 0; i < pCode_ScriptCount; ++i)
		{
			scriptInfo_t *info = &ScriptInfo[i];
			if(!info->imported && info->varCount > MAX_SCRIPT_VARIABLES)
			{
				MS_Message(MSG_DEBUG, "Script %d, var count = %d\n",
					info->number, info->varCount);
				PC_AppendWord(info->number);
				PC_AppendWord(info->varCount);
			}
		}
	}

	// Write script flags in a separate chunk, so older ZDooms don't get confused
	for (i = j = 0; i < pCode_ScriptCount; ++i)
	{
		if(!ScriptInfo[i].imported && ScriptInfo[i].flags != 0)
		{
			++j;
		}
	}
	if (j > 0)
	{
		PC_Append("SFLG", 4);
		PC_AppendInt(j * 4);
		for (i = 0; i < pCode_ScriptCount; ++i)
		{
			scriptInfo_t *info = &ScriptInfo[i];
			if(!info->imported && info->flags != 0)
			{
				PC_AppendWord(info->number);
				PC_AppendWord(info->flags);
			}
		}
	}

	// Write the string table for named scripts.
	STR_WriteListChunk(STRLIST_NAMEDSCRIPTS, MAKE4CC('S', 'N', 'A', 'M'), false);

	if(pCode_FunctionCount > 0)
	{
		PC_Append("FUNC", 4);
		PC_AppendInt(pCode_FunctionCount * 8);
		for(i = 0; i < pCode_FunctionCount; ++i)
		{
			functionInfo_t *info = &FunctionInfo[i];
			MS_Message(MSG_DEBUG, "Function " + to_string(i) + ":" + STR_GetString(STRLIST_FUNCTIONS, info->name) + ", address = " + to_string(info->address) + ", arg count = %d, var count = %d\n",
				info->argCount, info->localCount);
			PC_AppendByte(info->argCount);
			PC_AppendByte(info->localCount);
			PC_AppendByte(info->hasReturnValue?1:0);
			PC_AppendByte(0);
			PC_AppendInt(info->address);
		}
		STR_WriteListChunk(STRLIST_FUNCTIONS, MAKE4CC('F', 'N', 'A', 'M'), false);
	}

	if(NumLanguages > 1)
	{
		for(i = 0; i < NumLanguages; i++)
		{
			STR_WriteChunk(i, pCode_EncryptStrings);
		}
	}
	else if (STR_ListSize(STRLIST_PICS) > 0)
	{
		STR_WriteChunk(0, pCode_EncryptStrings);
	}

	STR_WriteListChunk(STRLIST_PICS, MAKE4CC('P', 'I', 'C', 'S'), false);
	if(MapVariablesInit)
	{
		int j;

		for(i = 0; i < pa_MapVarCount; ++i)
		{
			if(MapVariables[i].initializer != 0)
				break;
		}
		for(j = pa_MapVarCount-1; j > i; --j)
		{
			if(MapVariables[j].initializer != 0)
				break;
		}
		++j;

		if (i < j)
		{
			PC_Append("MINI", 4);
			PC_AppendInt((j-i)*4+4);
			PC_AppendInt(i);						// First map var defined
			for(; i < j; ++i)
			{
				PC_AppendInt(MapVariables[i].initializer);
			}
		}
	}

	// If this is a library, record which map variables are
	// initialized with strings.
	if(ImportMode == IMPORT_Exporting)
	{
		count = 0;

		for(i = 0; i < pa_MapVarCount; ++i)
		{
			if(MapVariables[i].isString)
			{
				++count;
			}
		}
		if(count > 0)
		{
			PC_Append("MSTR", 4);
			PC_AppendInt(count*4);
			for(i = 0; i < pa_MapVarCount; ++i)
			{
				if(MapVariables[i].isString)
				{
					PC_AppendInt(i);
				}
			}
		}

		// Now do the same thing for arrays.
		for(count = 0, i = 0; i < pa_MapVarCount; ++i)
		{
			if(ArrayOfStrings[i])
			{
				++count;
			}
		}
		if(count > 0)
		{
			PC_Append("ASTR", 4);
			PC_AppendInt(count*4);
			for(i = 0; i < pa_MapVarCount; ++i)
			{
				if(ArrayOfStrings[i])
				{
					PC_AppendInt(i);
				}
			}
		}
	}

	// Publicize the names of map variables in a library.
	if(ImportMode == IMPORT_Exporting)
	{
		for(i = 0; i < pa_MapVarCount; ++i)
		{
			if(!MapVariables[i].imported)
			{
				STR_AppendToList(STRLIST_MAPVARS, MapVariables[i].name);
			}
			else
			{
				STR_AppendToList(STRLIST_MAPVARS, NULL);
			}
		}
		STR_WriteListChunk(STRLIST_MAPVARS, MAKE4CC('M', 'E', 'X', 'P'), false);
	}

	// Record the names of imported map variables
	count = 0;
	for(i = 0; i < pa_MapVarCount; ++i)
	{
		if(MapVariables[i].imported && !ArraySizes[i])
		{
			count += 5 + strlen(MapVariables[i].name);
		}
	}
	if(count > 0)
	{
		PC_Append("MIMP", 4);
		PC_AppendInt(count);
		for(i = 0; i < pa_MapVarCount; ++i)
		{
			if(MapVariables[i].imported && !ArraySizes[i])
			{
				PC_AppendInt(i);
				PC_AppendString(MapVariables[i].name);
			}
		}
	}

	if(NumArrays)
	{
		int count;

		// Arrays defined here
		for(count = 0, i = 0; i < pa_MapVarCount; ++i)
		{
			if(ArraySizes[i] && !MapVariables[i].imported)
			{
				++count;
			}
		}
		if(count)
		{
			PC_Append("ARAY", 4);
			PC_AppendInt(count*8);
			for(i = 0; i < pa_MapVarCount; ++i)
			{
				if(ArraySizes[i] && !MapVariables[i].imported)
				{
					PC_AppendInt(i);
					PC_AppendInt(ArraySizes[i]);
				}
			}
			for(i = 0; i < pa_MapVarCount; ++i)
			{
				if(ArrayInits[i])
				{
					int j;

					PC_Append("AINI", 4);
					PC_AppendInt(ArraySizes[i]*4+4);
					PC_AppendInt(i);
					MS_Message(MSG_DEBUG, "Writing array initializers for array %d (size %d)\n", i, ArraySizes[i]);
					for(j = 0; j < ArraySizes[i]; ++j)
					{
						PC_AppendInt(ArrayInits[i][j]);
					}
				}
			}
		}

		// Arrays imported from elsewhere
		for(count = 0, j = i = 0; i < pa_MapVarCount; ++i)
		{
			if(ArraySizes[i] && MapVariables[i].imported)
			{
				count += 9 + strlen(MapVariables[i].name);
				++j;
			}
		}
		if(count)
		{
			PC_Append("AIMP", 4);
			PC_AppendInt(count+4);
			PC_AppendInt(j);
			for(i = 0; i < pa_MapVarCount; ++i)
			{
				if(ArraySizes[i] && MapVariables[i].imported)
				{
					PC_AppendInt(i);
					PC_AppendInt(ArraySizes[i]);
					PC_AppendString(MapVariables[i].name);
				}
			}
		}
	}

	// Add a dummy chunk to indicate if this object is a library.
	if(ImportMode == IMPORT_Exporting)
	{
		PC_Append("ALIB", 4);
		PC_AppendInt(0);
	}

	// Record libraries imported by this object.
	if(Imports.size() > 0)
	{
		count = 0;
		for (i = 0; i < Imports.size(); ++i)
		{
			count += Imports[i].length() + 1;
		}
		if(count > 0)
		{
			PC_Append("LOAD", 4);
			PC_AppendInt(count);
			for (i = 0; i < Imports.size(); ++i)
			{
				PC_AppendString(Imports[i]);
			}
		}
	}

	PC_AppendInt(chunkStart);
	if(pCode_NoShrink)
	{
		PC_Append("ACSE", 4);
	}
	else
	{
		PC_Append("ACSe", 4);
	}
	PC_WriteInt(pCode_Current, 4);

	// WadAuthor compatibility when creating a library is pointless, because
	// that editor does not know anything about libraries and will never
	// find their scripts ever.
	if(pCode_WadAuthor && ImportMode != IMPORT_Exporting)
	{
		RecordDummyScripts();
	}
	else
	{
		pCode_Append(0);
	}
	pCode_Append(0);
}

//==========================================================================
//
// CreateDummyScripts
//
//==========================================================================
static void CreateDummyScripts()
{
	Message(MSG_DEBUG, "Creating dummy scripts to make WadAuthor happy.");
	pCode_AppendPadding(4 - (pCode_Current % 4));
	pCode_TemporaryStorage = pCode_Current;
	for(int i = 0; i < pCode_ScriptCount; ++i)
	{
		// Only create dummies for scripts WadAuthor could care about.
		if(!ScriptInfo[i].imported && ScriptInfo[i].number >= 0 && ScriptInfo[i].number <= 255)
		{
			pCode_AppendCommand(PCD_TERMINATE);
			if(!pCode_NoShrink)
			{
				pCode_AppendCommand(PCD_NOP);
				pCode_AppendCommand(PCD_NOP);
				pCode_AppendCommand(PCD_NOP);
			}
		}
	}
}

//==========================================================================
//
// RecordDummyScripts
//
//==========================================================================
static void RecordDummyScripts()
{
	int i, j, count;

	for(i = count = 0; i < pCode_ScriptCount; ++i)
	{
		if(!ScriptInfo[i].imported && ScriptInfo[i].number >= 0 && ScriptInfo[i].number <= 255)
		{
			++count;
		}
	}
	pCode_Append(count);
	for(i = j = 0; i < pCode_ScriptCount; ++i)
	{
		scriptInfo_t *info = &ScriptInfo[i];
		if(!info->imported && info->number >= 0 && ScriptInfo[i].number <= 255)
		{
			MS_Message(MSG_DEBUG, "Dummy script %d, address = %d, arg count = %d\n",
				info->number, info->address, info->argCount);
			pCode_Append((int)info->number);
			pCode_Append(pCode_TemporaryStorage + j * 4);
			pCode_Append((int)info->argCount);
			j++;
		}
	}
}

//==========================================================================
//
// PC_Append functions
//
//==========================================================================

void pCode_Append(int data)
{
	if (ImportMode != IMPORT_Importing)
	{
		Message(MSG_DEBUG, "AI> " + string(pCode_Current) + " = " + string(data));
		data = MS_LittleUINT(data);
		pCode_Buffer.add(data);
		pCode_ByteSizes.add(4);
		pCode_Size += 4;
		pCode_Current++;
	}
}
void pCode_Append(short data)
{
	if (ImportMode != IMPORT_Importing)
	{
		Message(MSG_DEBUG, "AS> " + string(pCode_Current) + " = " + string(data));
		data = MS_LittleUWORD(data);
		pCode_Buffer.add(data);
		pCode_ByteSizes.add(2);
		pCode_Size += 2;
		pCode_Current++;
	}
}
void pCode_Append(byte data)
{
	if (ImportMode != IMPORT_Importing)
	{
		Message(MSG_DEBUG, "AB> " + string(pCode_Current) + " = " + string(data));
		pCode_Buffer.add(data);
		pCode_ByteSizes.add(1);
		pCode_Size++;
		pCode_Current++;
	}
}
void pCode_Append(string data)
{
	//TODO: check to see if \0 is needed
	for (char c : data)
	{
		Message(MSG_DEBUG, "AC> " + string(pCode_Current) + " = " + data);
		pCode_Buffer.add(c);
		pCode_ByteSizes.add(1);
		pCode_Size++;
		pCode_Current++;
	}
}

void pCode_AppendCommand(pCode cmd)
{
	if (ImportMode != IMPORT_Importing)
	{
		pCode_LastAppendedCommand = cmd;
		if (pCode_NoShrink)
		{
			pCode_CommandLog(pCode_Current, cmd, "AP");
			cmd = (pCode)MS_LittleUINT(cmd);
			pCode_Append(cmd);
		}
		else
		{
			bool dupbyte = false;
			if (cmd == PCD_DUP && PushByteAddr)
			{ // If the last instruction was PCD_PUSHBYTE, convert this PCD_DUP to a
				// duplicate PCD_PUSHBYTE, so it can be merged into a single instruction below.
				cmd = PCD_PUSHBYTE;
				dupbyte = true;
				Message(MSG_DEBUG, "AP> PCD_DUP changed to PCD_PUSHBYTE");
			} // TODO: analyse and fix this mess
			else if (cmd != PCD_PUSHBYTE && PushByteAddr)
			{ // Maybe shrink a PCD_PUSHBYTE sequence into PCD_PUSHBYTES
				int runlen = (pCode_Current - PushByteAddr) / 2;
				int i;

				if (runlen > 5)
				{
					pCode_Buffer[PushByteAddr] = PCD_PUSHBYTES;
					for (i = 0; i < runlen; i++)
					{
						pCode_Buffer[PushByteAddr + i + 2] = pCode_Buffer[PushByteAddr + i * 2 + 1];
					}
					pCode_Buffer[PushByteAddr + 1] = runlen;
					pCode_Current = PushByteAddr + runlen + 2;
					MS_Message(MSG_DEBUG, "AC> Last %d PCD_PUSHBYTEs changed to #%d:PCD_PUSHBYTES\n",
						runlen, PCD_PUSHBYTES);
				}
				else if (runlen > 1)
				{
					pCode_Buffer[PushByteAddr] = PCD_PUSH2BYTES + runlen - 2;
					for (i = 1; i < runlen; i++)
					{
						pCode_Buffer[PushByteAddr + 1 + i] = pCode_Buffer[PushByteAddr + 1 + i * 2];
					}
					pCode_Current = PushByteAddr + runlen + 1;
					MS_Message(MSG_DEBUG, "AC> Last %d PCD_PUSHBYTEs changed to #%d:PCD_PUSH%dBYTES\n",
						runlen, PCD_PUSH2BYTES + runlen - 2, runlen);
				}
				PushByteAddr = 0;
			}
			else if (cmd == PCD_PUSHBYTE && PushByteAddr == 0)
			{ // Remember the first PCD_PUSHBYTE, in case there are more
				PushByteAddr = pCode_Current;
			}
			pCode_CommandLog(pCode_Current, cmd, "AP");

			if (cmd < 256 - 16)
			{
				pCode_Append((byte)cmd);
			}
			else
			{
				// Room for expansion: The top 16 pcodes in the [0,255]
				// range select a set of pcodes, and the next byte is
				// the pcode in that set.
				byte bcmd = ((cmd - (256 - 16)) >> 8) + (256 - 16);
				pCode_Append(bcmd);
				bcmd = (cmd - (256 - 16)) & 255;
				pCode_Append(bcmd);
			}
			if (dupbyte)
			{
				pCode_Append(pCode_Buffer[pCode_Current - 2]);
			}
		}
	}
}

void pCode_AppendPadding(int bytes)
{
	for (int i = 0; i < bytes; i++)
	{
		pCode_Buffer.add(0);
		pCode_ByteSizes.add(1);
		pCode_Current++;
		pCode_Size++;
	}
}

//==========================================================================
//
// pCode_AppendShrink
//
//==========================================================================
void pCode_AppendShrink(byte val)
{
	pCode_Append(val);
	if (!pCode_NoShrink)
	{
		pCode_ByteSizes[pCode_Buffer.lastIndex()] = 1;
	}
}

//==========================================================================
//
// pCode_AppendPushVal
//
//==========================================================================
void pCode_AppendPushVal(int val)
{
	if(pCode_NoShrink || val > 255)
	{
		pCode_AppendCommand(PCD_PUSHNUMBER);
		pCode_Append(val);
	}
	else
	{
		pCode_AppendCommand(PCD_PUSHBYTE);
		pCode_AppendShrink(val);
	}
}

//==========================================================================
//
// pCode_Skip
//
//==========================================================================
static void pCode_Skip(int size)
{
	if (ImportMode != IMPORT_Importing)
	{
		pCode_AppendPadding(size);
	}
}

//==========================================================================
//
// PC_PutMapVariable
//
//==========================================================================
void PC_PutMapVariable(int value) // Add naming?
{
	if (sym_MapVariables.size() < MAX_MAP_VARIABLES) // TODO: separate map variables, and separate global/world vars
	{
		ACS_Var temp = ACS_Var("", (pa_ConstExprIsString) ? VAR_STR : VAR_INT, pa_CurrentDepth, true);
		temp.initializer = value;
		temp.isDeclared = true;
		sym_Variables.add(move(temp));
		sym_Variables.lastAdded().index = sym_Variables.lastIndex();
		pCode_HexenEnforcer();
	}
}

//==========================================================================
//
// PC_NameMapVariable
//
//==========================================================================
void PC_NameMapVariable(int index, ACS_Node *node)
{
	if(index < MAX_MAP_VARIABLES)
	{
		sym_MapVariables[index].name = node->name();
		sym_MapVariables[index].isImported = node->isImported();
	}
}

//==========================================================================
//
// pCode_AddScript
//
//==========================================================================
void pCode_AddScript(int number, ScriptActivation type, int flags, int argCount, string name = "")
{
	if (flags != 0 || number < 0 || number >= 1000)
	{
		HaveExtendedScripts = true;
		pCode_HexenEnforcer();
	}

	for (ACS_Script& item : sym_Scripts)
		if (item.number == number)
			ERR_Error(ERR_SCRIPT_ALREADY_DEFINED, true);

	if (sym_Scripts.size() >= MAX_SCRIPT_COUNT)
		ERR_Error(ERR_TOO_MANY_SCRIPTS, true);

	else
	{
		ACS_Script script = ACS_Script(number, argCount, flags, type);

		script.srcLine = tk_Line;
		script.isImported = (ImportMode == IMPORT_Importing);
		if (!script.isImported)
			script.address = pCode_Current;
		script.index = pCode_ScriptCount++;

		sym_Scripts.add(move(script));
	}
}

//==========================================================================
//
// pCode_SetScriptVarCount
//
// Sets the number of local variables used by a script, including
// arguments.
//
//==========================================================================
void pCode_SetScriptVarCount(int number, ScriptActivation type, int varCount)
{
	for (ACS_Script &script : sym_Scripts)
	{
		if (script.number == number && script.type == type)
		{
			script.varCount = varCount;
			break;
		}
	}
}

//==========================================================================
//
// pCode_AddFunction
//
//==========================================================================
void pCode_AddFunction(ACS_Node *node)
{
	if(pCode_FunctionCount >= MAX_FUNCTION_COUNT)
		ERR_Error(ERR_TOO_MANY_FUNCTIONS, true);
	
	pCode_HexenEnforcer();

	ACS_Function function = ACS_Function(node->name(), node->type, node->cmd->argTypes, node->depth());

	function.type = node->cmd->type;
	function.argTypes = node->cmd->argTypes;
	function.varCount = node->cmd->varCount;
	function.name = STR_AppendToList (STRLIST_FUNCTIONS, node->name);
	function.address = node->cmd->address;
	function.index = pCode_FunctionCount++;

	sym_Functions.add(move(function));
}

//==========================================================================
//
// PC_AddArray
//
//==========================================================================
void PC_AddArray(int index, int size)
{
	NumArrays++;
	ArraySizes[index] = size;
	pCode_HexenEnforcer();
}

//==========================================================================
//
// PC_InitArray
//
//==========================================================================
void PC_InitArray(int index, int *entries, bool hasStrings)
{
	int i;

	// If the array is just initialized to zeros, then we don't need to
	// remember the initializer.
	for(i = 0; i < ArraySizes[index]; ++i)
	{
		if(entries[i] != 0)
		{
			break;
		}
	}
	if(i < ArraySizes[index])
	{
		ArrayInits[index] = new int[ArraySizes[index]];
		ArrayInits[index] = entries; //ArraySizes[index]
	}
	ArrayOfStrings[index] = hasStrings;
}

//==========================================================================
//
// PC_AddImport
//
//==========================================================================
int PC_AddImport(string name)
{
	if (Imports.size() >= MAX_IMPORTS)
	{
		ERR_Exit(ERR_TOO_MANY_IMPORTS, true);
	}
	pCode_HexenEnforcer();
	Imports.add(name);
	return Imports.size();
}

//==========================================================================
//
// pCode_HexenEnforcer
//
// Simple inline function that throws an error if required, to make neater code
//
//==========================================================================
inline void pCode_HexenEnforcer()
{
	if (pCode_EnforceHexen)
		ERR_Error(ERR_HEXEN_COMPAT, true);
}