
//**************************************************************************
//**
//** parse.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <assert.h>

#include "common.h"
#include "parse.h"
#include "symbol.h"
#include "pcode.h"
#include "token.h"
#include "error.h"
#include "misc.h"
#include "strlist.h"

// MACROS ------------------------------------------------------------------

#define MAX_STATEMENT_DEPTH 128
#define MAX_BREAK 128
#define MAX_CONTINUE 128
#define MAX_CASE 128
#define EXPR_STACK_DEPTH 64

// TYPES -------------------------------------------------------------------

enum statement_t : int
{
	STMT_SCRIPT,
	STMT_IF,
	STMT_ELSE,
	STMT_DO,
	STMT_WHILEUNTIL,
	STMT_SWITCH,
	STMT_FOR
};

struct loopInfo_t 
{
	int level;
	int address;
};

struct caseInfo_t : public loopInfo_t
{
	int value;
	bool isDefault;
};

struct prefunc_t
{
	struct prefunc_t *next;
	ACS_Node *node;
	int address;
	int argcount;
	int line;
	char *source;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void CountScript(int type);
static void Outside();
static void OuterScript();
static void OuterFunction();
static void OuterMapVar(int type);
static void OuterWorldVar(bool isGlobal);
static void OuterSpecialDef();
static void OuterDefine(bool force);
static void OuterInclude();
static void OuterImport();
static bool ProcessStatement(statement_t owner);
static void LeadingCompoundStatement(statement_t owner);
static void LeadingVarDeclare();
static void LeadingLineSpecial(bool executewait);
static void LeadingFunction(bool executewait);
static void LeadingIdentifier();
static void BuildPrintString();
static void ActionOnCharRange(bool write);
static void LeadingStrcpy();
static void LeadingPrint();
static void LeadingHudMessage();
static void LeadingVarAssign(symbolNode_t *sym);
static pcd_t GetAssignPCD(tokenType_t token, int type);
static void LeadingInternFunc(symbolNode_t *sym);
static void LeadingScriptFunc(symbolNode_t *sym);
static void LeadingSuspend();
static void LeadingTerminate();
static void LeadingRestart();
static void LeadingReturn();
static void LeadingIf();
static void LeadingFor();
static void LeadingWhileUntil();
static void LeadingDo();
static void LeadingSwitch();
static void LeadingCase();
static void LeadingDefault();
static void LeadingBreak();
static void LeadingContinue();
static void LeadingCreateTranslation();
static void LeadingIncDec(int token);
static void PushCase(int value, bool isDefault);
static caseInfo_t *GetCaseInfo();
static int CaseInfoCmp(const void *a, const void *b);
static bool DefaultInCurrent();
static void PushBreak();
static void WriteBreaks();
static bool BreakAncestor();
static void PushContinue();
static void WriteContinues(int address);
static bool ContinueAncestor();
static void ProcessInternFunc(symbolNode_t *sym);
static void ProcessScriptFunc(symbolNode_t *sym, bool discardReturn);
static void EvalExpression();
static void ExprLevX(int level);
static void ExprLevA();
static void ExprFactor();
static void ConstExprFactor();
static void SendExprCommand(pcd_t pcd);
static void PushExStk(int value);
static int PopExStk();
static pcd_t TokenToPCD(tokenType_t token);
static pcd_t GetPushVarPCD(int symType);
static pcd_t GetIncDecPCD(tokenType_t token, int symbol);
static int EvalConstExpression();
static void ParseArrayIndices(symbolNode_t *sym, int requiredIndices);
static void InitializeArray(symbolNode_t *sym, int dims[MAX_ARRAY_DIMS], int size);
static symbolNode_t *DemandSymbol(char *name);
static symbolNode_t *SpeculateSymbol(char *name, bool hasReturn);
static symbolNode_t *SpeculateFunction(const char *name, bool hasReturn);
static void UnspeculateFunction(symbolNode_t *sym);
static void AddScriptFuncRef(symbolNode_t *sym, int address, int argcount);
static void CheckForUndefinedFunctions();
static void SkipBraceBlock(int depth);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int pa_ScriptCount;
struct ScriptTypes *pa_TypedScriptCounts;
int pa_MapVarCount;
int pa_WorldVarCount;
int pa_GlobalVarCount;
int pa_WorldArrayCount;
int pa_GlobalArrayCount;
enum ImportModes ImportMode = IMPORT_None;
bool ExporterFlagged;
bool pa_ConstExprIsString;
int pa_CurrentDepth = 0;	// Current statement depth
int pa_FileDepth = 0;		// Outermost level in the current file

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte ScriptVarCount;
static statement_t StatementHistory[MAX_STATEMENT_DEPTH];
static int StatementIndex;
static loopInfo_t BreakInfo[MAX_BREAK];
static int BreakIndex;
static loopInfo_t ContinueInfo[MAX_CONTINUE];
static int ContinueIndex;
static caseInfo_t CaseInfo[MAX_CASE];
static int CaseIndex;
static int StatementLevel;
static int ExprStack[EXPR_STACK_DEPTH];
static int ExprStackIndex;
static bool ConstantExpression;
static symbolNode_t *InsideFunction;
static prefunc_t *FillinFunctions;
static prefunc_t **FillinFunctionsLatest = &FillinFunctions;
static bool ArrayHasStrings;

static int AdjustStmtLevel[] =
{
	false,		// STMT_SCRIPT
	true,		// STMT_IF
	true,		// STMT_ELSE
	true,		// STMT_DO
	true,		// STMT_WHILEUNTIL
	true,		// STMT_SWITCH
	true		// STMT_FOR
};

static bool IsBreakRoot[] =
{
	false,		// STMT_SCRIPT
	false,		// STMT_IF
	false,		// STMT_ELSE
	true,		// STMT_DO
	true,		// STMT_WHILEUNTIL
	true,		// STMT_SWITCH
	true		// STMT_FOR
};

static bool IsContinueRoot[] =
{
	false,		// STMT_SCRIPT
	false,		// STMT_IF
	false,		// STMT_ELSE
	true,		// STMT_DO
	true,		// STMT_WHILEUNTIL
	false,		// STMT_SWITCH
	true		// STMT_FOR
};

static tokenType_t LevAOps[] =
{
	TK_ORLOGICAL,
	TK_NONE
};

static tokenType_t LevBOps[] =
{
	TK_ANDLOGICAL,
	TK_NONE
};

static tokenType_t LevCOps[] =
{
	TK_ORBITWISE,
	TK_NONE
};

static tokenType_t LevDOps[] =
{
	TK_EORBITWISE,
	TK_NONE
};

static tokenType_t LevEOps[] =
{
	TK_ANDBITWISE,
	TK_NONE
};

static tokenType_t LevFOps[] =
{
	TK_EQ,
	TK_NE,
	TK_NONE
};

static tokenType_t LevGOps[] =
{
	TK_LT,
	TK_LE,
	TK_GT,
	TK_GE,
	TK_NONE
};

static tokenType_t LevHOps[] =
{
	TK_LSHIFT,
	TK_RSHIFT,
	TK_NONE
};

static tokenType_t LevIOps[] =
{
	TK_PLUS,
	TK_MINUS,
	TK_NONE
};

static tokenType_t LevJOps[] =
{
	TK_ASTERISK,
	TK_SLASH,
	TK_PERCENT,
	TK_NONE
};

static tokenType_t *OpsList[] =
{
	LevAOps,
	LevBOps,
	LevCOps,
	LevDOps,
	LevEOps,
	LevFOps,
	LevGOps,
	LevHOps,
	LevIOps,
	LevJOps,
	NULL
};

static tokenType_t AssignOps[] =
{
	TK_ASSIGN,
	TK_ADDASSIGN,
	TK_SUBASSIGN,
	TK_MULASSIGN,
	TK_DIVASSIGN,
	TK_MODASSIGN,
	TK_ANDASSIGN,
	TK_EORASSIGN,
	TK_ORASSIGN,
	TK_LSASSIGN,
	TK_RSASSIGN,
	TK_NONE
};

static struct ScriptTypes ScriptCounts[] =
{
	{ "closed",			0,							0 },
	{ "open",			OPEN_SCRIPTS_BASE,			0 },
	{ "respawn",		RESPAWN_SCRIPTS_BASE,		0 },
	{ "death",			DEATH_SCRIPTS_BASE,			0 },
	{ "enter",			ENTER_SCRIPTS_BASE,			0 },
	{ "pickup",			PICKUP_SCRIPTS_BASE,		0 },
	{ "bluereturn",		BLUE_RETURN_SCRIPTS_BASE,	0 },
	{ "redreturn",		RED_RETURN_SCRIPTS_BASE,	0 },
	{ "whitereturn",	WHITE_RETURN_SCRIPTS_BASE,	0 },
	{ "lightning",		LIGHTNING_SCRIPTS_BASE,		0 },
	{ "disconnect",		DISCONNECT_SCRIPTS_BASE,	0 },
	{ "unloading",		UNLOADING_SCRIPTS_BASE,		0 },
	{ "return",			RETURN_SCRIPTS_BASE,		0 },
	{ NULL,				-1,							0 }
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// PA_Parse
//
//==========================================================================

void PA_Parse()
{
	int i;

	pa_ScriptCount = 0;
	pa_TypedScriptCounts = ScriptCounts;
	for (i = 0; ScriptCounts[i].TypeName != NULL; i++)
	{
		ScriptCounts[i].TypeCount = 0;
	}
	pa_MapVarCount = 0;
	pa_WorldVarCount = 0;
	pa_GlobalVarCount = 0;
	pa_WorldArrayCount = 0;
	pa_GlobalArrayCount = 0;
	TK_NextToken();
	Outside();
	CheckForUndefinedFunctions();
	ERR_Finish();
}

//==========================================================================
//
// CountScript
//
//==========================================================================

static void CountScript(int type)
{
	int i;

	for (i = 0; ScriptCounts[i].TypeName != NULL; i++)
	{
		if (ScriptCounts[i].TypeBase == type)
		{
			if (type != 0)
			{
				MS_Message(MSG_DEBUG, "Script type: %s\n",
					ScriptCounts[i].TypeName);
			}
			ScriptCounts[i].TypeCount++;
			return;
		}
	}
	return;
}

//==========================================================================
//
// Outside
//
//==========================================================================

static void Outside()
{
	bool done = false;

	//Preprocessors should be inspected first
	while (!done)
	{
		switch (tk_Token)
		{
		case TK_NUMBERSIGN:
			TK_NextToken();
			switch (tk_Token)
			{
			case TK_DEFINE:
				OuterDefine(false);
				break;
			case TK_LIBDEFINE:
				OuterDefine(true);
				break;
			case TK_INCLUDE:
				OuterInclude();
				break;
			case TK_NOCOMPACT:
				if (ImportMode != IMPORT_Importing)
				{
					if (pc_Address != 8)
					{
						ERR_Error(ERR_NOCOMPACT_NOT_HERE, true);
					}
					MS_Message(MSG_DEBUG, "Forcing NoShrink\n");
					pc_NoShrink = true;
				}
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			case TK_WADAUTHOR:
				if (ImportMode != IMPORT_Importing)
				{
					MS_Message(MSG_DEBUG, "Will write WadAuthor-compatible object\n");
					MS_Message(MSG_NORMAL, "You don't need to use #wadauthor anymore.\n");
					pc_WadAuthor = true;
				}
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			case TK_NOWADAUTHOR:
				if (ImportMode != IMPORT_Importing)
				{
					MS_Message(MSG_DEBUG, "Will write WadAuthor-incompatible object\n");
					pc_WadAuthor = false;
				}
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			case TK_ENCRYPTSTRINGS:
				if (ImportMode != IMPORT_Importing)
				{
					MS_Message(MSG_DEBUG, "Strings will be encrypted\n");
					pc_EncryptStrings = true;
					if (pc_EnforceHexen)
					{
						ERR_Error(ERR_HEXEN_COMPAT, true);
					}
				}
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			case TK_IMPORT:
				OuterImport();
				break;
			case TK_LIBRARY:
				TK_NextTokenMustBe(TK_STRING, ERR_STRING_LIT_NOT_FOUND);
				if (ImportMode == IMPORT_None)
				{
					MS_Message(MSG_DEBUG, "Allocations modified for exporting\n");
					ImportMode = IMPORT_Exporting;
				}
				else if (ImportMode == IMPORT_Importing)
				{
					PC_AddImport(tk_String);
					ExporterFlagged = true;
				}
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			default:
				ERR_Error(ERR_INVALID_DIRECTIVE, true);
				TK_SkipTo(TK_NUMBERSIGN);
				break;
			}
			break;
		default:
			TK_SkipTo(TK_NUMBERSIGN);
			break;
		}
	}

	done = false;

	while(!done)
	{
		switch(tk_Token)
		{
		case TK_EOF:

			done = true;
			break;
		case TK_SCRIPT:
			OuterScript();
			break;
		case TK_FUNCTION:
			OuterFunction();
			break;
		// [JRT] Types matter now
		case TK_INT:
			OuterMapVar(VAR_INT);
			break;
		case TK_STR:
			OuterMapVar(VAR_STR);
			break;
		case TK_BOOL:
			OuterMapVar(VAR_BOOL);
			break;
		case TK_FIXED:
			OuterMapVar(VT_FIXED);
			break;
		case TK_TID:
			OuterMapVar(VT_TID);
			break;
		case TK_ACTOR:
			//OuterMapStruct(VT_ACTOR);
			break;
		case TK_CLASS:
			//OuterMapStruct(VT_CLASS);
			break;
		// [JRT] End new types
		case TK_WORLD:
			OuterWorldVar(false);
			break;
		case TK_GLOBAL:
			OuterWorldVar(true);
			break;
		case TK_SPECIAL:
			OuterSpecialDef();
			break;
		case TK_NUMBERSIGN: // [JRT] "pre" processed already.
			TK_SkipLine();
			break;
		default:
			ERR_Exit(ERR_INVALID_DECLARATOR, true, NULL);
			break;
	   	}
	}
}

//==========================================================================
//
// OuterScript
//
//==========================================================================

static void OuterScript()
{
	int scriptNumber;
	symbolNode_t *sym;
	int scriptType, scriptFlags;

	MS_Message(MSG_DEBUG, "---- OuterScript ----\n");
	BreakIndex = 0;
	CaseIndex = 0;
	StatementLevel = 0;
	ScriptVarCount = 0;
	SY_FreeLocals();
	TK_NextToken();

	if(ImportMode == IMPORT_Importing)
	{
		// When importing, the script number is not recorded, because
		// it might be a #define that is not included by the main .acs
		// file, so processing it would generate a syntax error.
		SkipBraceBlock(0);
		TK_NextToken();
		return;
	}

	// [RH] If you want to use script 0, it must be written as <<0>>.
	// This is to avoid using it accidentally, since ZDoom uses script
	// 0 to implement many of the Strife-specific line specials.

	if(tk_Token == TK_LSHIFT)
	{
		TK_NextTokenMustBe(TK_NUMBER, ERR_SCRIPT_OUT_OF_RANGE);
		if(tk_Number != 0)
		{
			ERR_Exit(ERR_SCRIPT_OUT_OF_RANGE, true, NULL);
		}
		TK_NextTokenMustBe(TK_RSHIFT, ERR_SCRIPT_OUT_OF_RANGE);
		TK_NextToken();
		scriptNumber = 0;
	}
	else if(tk_Token == TK_STRING)
	{ // Named scripts start counting at -1 and go down from there.
		if(tk_String.compare("None") == 0)
		{
			ERR_Error(ERR_SCRIPT_NAMED_NONE, true, NULL);
		}
		scriptNumber = -1 - STR_FindInListInsensitive(STRLIST_NAMEDSCRIPTS, tk_String);
		TK_NextToken();
	}
	else
	{
		scriptNumber = EvalConstExpression();
		if(scriptNumber < 1 || scriptNumber > 32767)
		{
			TK_Undo();
			ERR_Error(ERR_SCRIPT_OUT_OF_RANGE, true, NULL);
			SkipBraceBlock(0);
			TK_NextToken();
			return;
		}
	}
	if (scriptNumber >= 0)
	{
		MS_Message(MSG_DEBUG, "Script number: %d\n", scriptNumber);
	}
	else
	{
		MS_Message(MSG_DEBUG, "Script name: %s (%d)\n",
			STR_GetString(STRLIST_NAMEDSCRIPTS, -scriptNumber - 1),
			scriptNumber);
	}
	scriptType = 0;
	scriptFlags = 0;
	if(tk_Token == TK_LPAREN)
	{
		if(TK_NextToken() == TK_VOID)
		{
			TK_NextTokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
		}
		else
		{
			TK_Undo();
			do
			{
				TK_NextTokenMustBe(TK_INT, ERR_BAD_VAR_TYPE);
				TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
				if(ScriptVarCount == 4)
				{
					ERR_Error(ERR_TOO_MANY_SCRIPT_ARGS, true);
				}
				if(SY_FindLocal(tk_String) != NULL)
				{ // Redefined
					ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
				}
				else if(ScriptVarCount < 4)
				{
					sym = SY_InsertLocal(tk_String, SY_SCRIPTVAR);
					sym->info.var.index = ScriptVarCount;
					ScriptVarCount++;
				}
				TK_NextToken();
			} while(tk_Token == TK_COMMA);
			TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
		}
		TK_NextToken();
		switch(tk_Token)
		{
		case TK_DISCONNECT:
			scriptType = DISCONNECT_SCRIPTS_BASE;
			if(ScriptVarCount != 1)
			{
				ERR_Error(ERR_DISCONNECT_NEEDS_1_ARG, true);
			}
			break;

		case TK_OPEN:
		case TK_RESPAWN:
		case TK_DEATH:
		case TK_ENTER:
		case TK_PICKUP:
		case TK_BLUERETURN:
		case TK_REDRETURN:
		case TK_WHITERETURN:
		case TK_LIGHTNING:
		case TK_UNLOADING:
		case TK_RETURN:
			ERR_Error(ERR_UNCLOSED_WITH_ARGS, true);
			break;

		default:
			TK_Undo();
		}
		MS_Message(MSG_DEBUG, "Script type: %s (%d %s)\n",
			scriptType == 0 ? "closed" : "disconnect",
			ScriptVarCount, ScriptVarCount == 1 ? "arg" : "args");
	}
	else switch (tk_Token)
	{
	case TK_OPEN:
		scriptType = OPEN_SCRIPTS_BASE;
		break;

	case TK_RESPAWN:	// [BC]
		scriptType = RESPAWN_SCRIPTS_BASE;
		break;

	case TK_DEATH:		// [BC]
		scriptType = DEATH_SCRIPTS_BASE;
		break;

	case TK_ENTER:		// [BC]
		scriptType = ENTER_SCRIPTS_BASE;
		break;

	case TK_RETURN:
		scriptType = RETURN_SCRIPTS_BASE;
		break;

	case TK_PICKUP:		// [BC]
		scriptType = PICKUP_SCRIPTS_BASE;
		break;

	case TK_BLUERETURN:	// [BC]
		scriptType = BLUE_RETURN_SCRIPTS_BASE;
		break;

	case TK_REDRETURN:	// [BC]
		scriptType = RED_RETURN_SCRIPTS_BASE;
		break;

	case TK_WHITERETURN:	// [BC]
		scriptType = WHITE_RETURN_SCRIPTS_BASE;
		break;

	case TK_LIGHTNING:
		scriptType = LIGHTNING_SCRIPTS_BASE;
		break;

	case TK_UNLOADING:
		scriptType = UNLOADING_SCRIPTS_BASE;
		break;

	case TK_DISCONNECT:
		scriptType = DISCONNECT_SCRIPTS_BASE;
		ERR_Error (ERR_DISCONNECT_NEEDS_1_ARG, true);
		break;

	default:
		ERR_Error(ERR_BAD_SCRIPT_DECL, true);
		SkipBraceBlock(0);
		TK_NextToken();
		return;
	}
	TK_NextToken();
	if(tk_Token == TK_NET)
	{
		scriptFlags |= NET_SCRIPT_FLAG;
		TK_NextToken();
	}
	// [BB] If NET and CLIENTSIDE are specified, this construction can only parse
	// "NET CLIENTSIDE" but not "CLIENTSIDE NET".
	if(tk_Token == TK_CLIENTSIDE)
	{
		scriptFlags |= CLIENTSIDE_SCRIPT_FLAG;
		TK_NextToken();
	}
	CountScript(scriptType);
	PC_AddScript(scriptNumber, scriptType, scriptFlags, ScriptVarCount);
	pc_LastAppendedCommand = PCD_NOP;
	if(ProcessStatement(STMT_SCRIPT) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	if(pc_LastAppendedCommand != PCD_TERMINATE)
	{
		PC_AppendCmd(PCD_TERMINATE);
	}
	PC_SetScriptVarCount(scriptNumber, scriptType, ScriptVarCount);
	pa_ScriptCount++;
}

//==========================================================================
//
// OuterFunction
//
//==========================================================================

static void OuterFunction()
{
	enum ImportModes importing;
	bool hasReturn;
	symbolNode_t *sym;
	int defLine;

	MS_Message(MSG_DEBUG, "---- OuterFunction ----\n");
	importing = ImportMode;
	BreakIndex = 0;
	CaseIndex = 0;
	StatementLevel = 0;
	ScriptVarCount = 0;
	SY_FreeLocals();
	TK_NextToken();
	if(tk_Token != TK_STR && tk_Token != TK_INT &&
		tk_Token != TK_VOID && tk_Token != TK_BOOL)
	{
		ERR_Error(ERR_BAD_RETURN_TYPE, true);
		tk_Token = TK_VOID;
	}
	hasReturn = tk_Token != TK_VOID;
	TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
	sym = SY_FindGlobal(tk_String);
	if(sym != NULL)
	{
		if(sym->type != SY_SCRIPTFUNC)
		{ // Redefined
			ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
			SkipBraceBlock(0);
			TK_NextToken();
			return;
		}
		if(!sym->info.scriptFunc.predefined)
		{
			ERR_Error(ERR_FUNCTION_ALREADY_DEFINED, true);
			ERR_ErrorAt(sym->info.scriptFunc.sourceName, sym->info.scriptFunc.sourceLine);
			ERR_Error(ERR_NONE, true, "Previous definition was here.");
			SkipBraceBlock(0);
			TK_NextToken();
			return;
		}
		if(sym->info.scriptFunc.hasReturnValue && !hasReturn)
		{
			ERR_Error(ERR_PREVIOUS_NOT_VOID, true);
			ERR_ErrorAt(sym->info.scriptFunc.sourceName, sym->info.scriptFunc.sourceLine);
			ERR_Error(ERR_NONE, true, "Previous use was here.");
		}
	}
	else
	{
		sym = SY_InsertGlobal(tk_String, SY_SCRIPTFUNC);
		sym->info.scriptFunc.address = (importing == IMPORT_Importing ? 0 : pc_Address);
		sym->info.scriptFunc.predefined = false;
	}
	defLine = tk_Line;

	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	if(TK_NextToken() == TK_VOID)
	{
		TK_NextTokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	}
	else
	{
		TK_Undo();
		do
		{
			int type;
			symbolNode_t *local;

			TK_NextToken();
/*			if(tk_Token != TK_INT && tk_Token != TK_STR && tk_Token != TK_BOOL)
			{
				ERR_Error(ERR_BAD_VAR_TYPE, true);
				tk_Token = TK_INT;
			}
*/			if (tk_Token == TK_INT)
			{
				if(TK_NextToken() == TK_LBRACKET)
				{
					TK_NextTokenMustBe(TK_RBRACKET, ERR_BAD_VAR_TYPE);
					type = SY_SCRIPTALIAS;
				}
				else
				{
					TK_Undo();
					type = SY_SCRIPTVAR;
				}
			}
			else if (tk_Token == TK_STR || tk_Token == TK_CLASS || tk_Token == TK_ACTOR || tk_Token == TK_BOOL || tk_Token == TK_FIXED)
			{
				type = SY_SCRIPTVAR;
			}
			else
			{
				type = SY_SCRIPTVAR;
				ERR_Error(ERR_BAD_VAR_TYPE, true);
				tk_Token = TK_INT;
			}
			TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
			if(SY_FindLocal(tk_String) != NULL)
			{ // Redefined
				ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
			}
			else
			{
				local = SY_InsertLocal(tk_String, type);
				local->info.var.index = ScriptVarCount;
				ScriptVarCount++;
			}
			TK_NextToken();
		} while(tk_Token == TK_COMMA);
		TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	}

	sym->info.scriptFunc.sourceLine = defLine;
	sym->info.scriptFunc.sourceName = tk_SourceName;
	sym->info.scriptFunc.argCount = ScriptVarCount;
	sym->info.scriptFunc.address = (importing == IMPORT_Importing) ? 0 : pc_Address;
	sym->info.scriptFunc.hasReturnValue = hasReturn;

	if(importing == IMPORT_Importing)
	{
		SkipBraceBlock(0);
		TK_NextToken();
		sym->info.scriptFunc.predefined = false;
		sym->info.scriptFunc.varCount = ScriptVarCount;
		return;
	}

	TK_NextToken();
	InsideFunction = sym;
	pc_LastAppendedCommand = PCD_NOP;

	// If we just call ProcessStatement(STMT_SCRIPT), and this function
	// needs to return a value but the last pcode output was not a return,
	// then the line number given in the error can be confusing because it
	// is beyond the end of the function. To avoid this, we process the
	// compound statement ourself and check if it returned something
	// before checking for the '}'. If a return is required, then the error
	// line will be shown as the one that contains the '}' (if present).

	TK_TokenMustBe(TK_LBRACE, ERR_MISSING_LBRACE);
	TK_NextToken();
	do {} while(ProcessStatement(STMT_SCRIPT) == true);

	if(pc_LastAppendedCommand != PCD_RETURNVOID &&
	   pc_LastAppendedCommand != PCD_RETURNVAL)
	{
		if(hasReturn)
		{
			TK_Undo();
			ERR_Error(ERR_MUST_RETURN_A_VALUE, true, NULL);
		}
		PC_AppendCmd(PCD_RETURNVOID);
	}

	TK_TokenMustBe(TK_RBRACE, ERR_INVALID_STATEMENT);
	TK_NextToken();

	sym->info.scriptFunc.predefined = false;
	sym->info.scriptFunc.varCount = ScriptVarCount -
		sym->info.scriptFunc.argCount;
	PC_AddFunction(sym);
	UnspeculateFunction(sym);
	InsideFunction = NULL;
}

//==========================================================================
//
// OuterMapVar
//
//==========================================================================

static void OuterMapVar(int local)
{
	symbolNode_t *sym = NULL;
	int index, i;

	MS_Message(MSG_DEBUG, "---- %s ----\n", local ? "LeadingStaticVarDeclare" : "OuterMapVar");
	do
	{
		if(pa_MapVarCount >= MAX_MAP_VARIABLES)
		{
			ERR_Error(ERR_TOO_MANY_MAP_VARS, true);
			index = MAX_MAP_VARIABLES;
		}
		TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
		sym = local ? SY_FindLocal(tk_String)
					: SY_FindGlobal(tk_String);
		if(sym != NULL)
		{ // Redefined
			ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
			index = MAX_MAP_VARIABLES;
		}
		else
		{
			sym = local ? SY_InsertLocal(tk_String, SY_MAPVAR)
						: SY_InsertGlobal(tk_String, SY_MAPVAR);
			if(ImportMode == IMPORT_Importing)
			{
				sym->info.var.index = index = 0;
			}
			else
			{
				sym->info.var.index = index = pa_MapVarCount;
				if (!local)
				{ // Local variables are not exported
					PC_NameMapVariable(index, sym);
				}
				pa_MapVarCount++;
			}
		}
		TK_NextToken();
		if(tk_Token == TK_ASSIGN)
		{
			if(ImportMode != IMPORT_Importing)
			{
				TK_NextToken();
				PC_PutMapVariable (index, EvalConstExpression());
			}
			else
			{
				// When importing, skip the initializer, because we don't care.
				do
				{
					TK_NextToken();
				} while(tk_Token != TK_COMMA && tk_Token != TK_SEMICOLON);
			}
		}
		else if(tk_Token == TK_LBRACKET)
		{
			int size = 0;
			int ndim = 0;
			int dims[MAX_ARRAY_DIMS];

			memset(dims, 0, sizeof(dims));

			while(tk_Token == TK_LBRACKET)
			{
				if(ndim == MAX_ARRAY_DIMS)
				{
					ERR_Error(ERR_TOO_MANY_ARRAY_DIMS, true);
					do
					{
						TK_NextToken();
					} while(tk_Token != TK_COMMA && tk_Token != TK_SEMICOLON);
					break;
				}
				TK_NextToken();
				if (tk_Token == TK_RBRACKET)
				{
					ERR_Error(ERR_NEED_ARRAY_SIZE, true);
				}
				else
				{
					dims[ndim] = EvalConstExpression();
					if(dims[ndim] == 0)
					{
						ERR_Error(ERR_ZERO_DIMENSION, true);
						dims[ndim] = 1;
					}
					if(ndim == 0)
					{
						size = dims[ndim];
					}
					else
					{
						size *= dims[ndim];
					}
				}
				ndim++;
				TK_TokenMustBe(TK_RBRACKET, ERR_MISSING_RBRACKET);
				TK_NextToken();
			}
			if(sym != NULL)
			{
				if(ImportMode != IMPORT_Importing)
				{
					PC_AddArray(index, size);
				}
				MS_Message(MSG_DEBUG, "%s changed to an array of size %d\n", sym->name, size);
				sym->type = SY_MAPARRAY;
				sym->info.array.index = index;
				sym->info.array.ndim = ndim;
				sym->info.array.size = size;
				if(ndim > 0)
				{
					sym->info.array.dimensions[ndim-1] = 1;
					for(i = ndim - 2; i >= 0; --i)
					{
						sym->info.array.dimensions[i] =
							sym->info.array.dimensions[i+1] * dims[i+1];
					}
				}
				MS_Message(MSG_DEBUG, " - with multipliers ");
				for(i = 0; i < ndim; ++i)
				{
					MS_Message(MSG_DEBUG, "[%d]", sym->info.array.dimensions[i]);
				}
				MS_Message(MSG_DEBUG, "\n");
				if(tk_Token == TK_ASSIGN)
				{
					InitializeArray(sym, dims, size);
				}
			}
		}
	} while(tk_Token == TK_COMMA);
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// OuterWorldVar
//
//==========================================================================

static void OuterWorldVar(bool isGlobal)
{
	int index;
	symbolNode_t *sym;

	MS_Message(MSG_DEBUG, "---- Outer%sVar ----\n", isGlobal ? "Global" : "World");
	if(TK_NextToken() != TK_INT)
	{
		if(tk_Token != TK_BOOL)
		{
			TK_TokenMustBe(TK_STR, ERR_BAD_VAR_TYPE);
		}
	}
	do
	{
		TK_NextTokenMustBe(TK_NUMBER, ERR_MISSING_WVAR_INDEX);
		if(tk_Number >= (isGlobal ? MAX_GLOBAL_VARIABLES : MAX_WORLD_VARIABLES))
		{
			ERR_Error(ERR_BAD_WVAR_INDEX + isGlobal, true);
			index = 0;
		}
		else
		{
			index = tk_Number;
		}
		TK_NextTokenMustBe(TK_COLON, ERR_MISSING_WVAR_COLON+isGlobal);
		TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
		if(SY_FindGlobal(tk_String) != NULL)
		{ // Redefined
			ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
		}
		else
		{
			TK_NextToken();
			if(tk_Token == TK_LBRACKET)
			{
				TK_NextToken ();
				if(tk_Token != TK_RBRACKET)
				{
					ERR_Error(ERR_NO_NEED_ARRAY_SIZE, true);
					TK_SkipPast(TK_RBRACKET);
				}
				else
				{
					TK_NextToken();
				}
				if(tk_Token == TK_LBRACKET)
				{
					ERR_Error(ERR_NO_MULTIDIMENSIONS, true);
					do
					{
						TK_SkipPast(TK_RBRACKET);
					}
					while(tk_Token == TK_LBRACKET);
				}
				sym = SY_InsertGlobal(tk_String, isGlobal ? SY_GLOBALARRAY : SY_WORLDARRAY);
				sym->info.array.index = index;
				sym->info.array.ndim = 1;
				sym->info.array.size = 0x7fffffff;	// not used
				memset(sym->info.array.dimensions, 0, sizeof(sym->info.array.dimensions));

				if (isGlobal)
					pa_GlobalArrayCount++;
				else
					pa_WorldArrayCount++;
			}
			else
			{
				sym = SY_InsertGlobal(tk_String, isGlobal ? SY_GLOBALVAR : SY_WORLDVAR);
				sym->info.var.index = index;
				if (isGlobal)
					pa_GlobalVarCount++;
				else
					pa_WorldVarCount++;
			}
		}
	} while(tk_Token == TK_COMMA);
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// OuterSpecialDef
//
//==========================================================================

static void OuterSpecialDef()
{
	int special;
	symbolNode_t *sym;

	MS_Message(MSG_DEBUG, "---- OuterSpecialDef ----\n");
	if(ImportMode == IMPORT_Importing)
	{
		// No need to process special definitions when importing.
		TK_SkipPast(TK_SEMICOLON);
	}
	else
	{
		do
		{
			if (TK_NextToken() == TK_MINUS)
			{
				TK_NextTokenMustBe(TK_NUMBER, ERR_MISSING_SPEC_VAL);
				special = -tk_Number;
			}
			else
			{
				TK_TokenMustBe(TK_NUMBER, ERR_MISSING_SPEC_VAL);
				special = tk_Number;
			}
			TK_NextTokenMustBe(TK_COLON, ERR_MISSING_SPEC_COLON);
			TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
			sym = SY_InsertGlobalUnique(tk_String, SY_SPECIAL);
			TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
			TK_NextTokenMustBe(TK_NUMBER, ERR_MISSING_SPEC_ARGC);
			sym->info.special.value = special;
			sym->info.special.argCount = tk_Number | (tk_Number << 16);
			TK_NextToken();
			if(tk_Token == TK_COMMA)
			{ // Get maximum arg count
				TK_NextTokenMustBe(TK_NUMBER, ERR_MISSING_SPEC_ARGC);
				sym->info.special.argCount =
					(sym->info.special.argCount & 0xffff) | (tk_Number << 16);
			}
			else
			{
				TK_Undo ();
			}
			TK_NextTokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
			TK_NextToken();
		} while(tk_Token == TK_COMMA);
		TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
		TK_NextToken();
	}
}

//==========================================================================
//
// OuterDefine
//
//==========================================================================

static void OuterDefine(bool force)
{
	int value;
	symbolNode_t *sym;

	MS_Message(MSG_DEBUG, "---- OuterDefine %s----\n",
		force ? "(forced) " : "");
	TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
	sym = SY_InsertGlobalUnique(tk_String, SY_CONSTANT);
	TK_NextToken();
	value = EvalConstExpression();
	MS_Message(MSG_DEBUG, "Constant value: %d\n", value);
	sym->info.constant.value = value;
	// Defines inside an import are deleted when the import is popped.
	if(ImportMode != IMPORT_Importing || force)
	{
		sym->info.constant.fileDepth = 0;
	}
	else
	{
		sym->info.constant.fileDepth = TK_GetDepth();
	}
}

//==========================================================================
//
// OuterInclude
//
//==========================================================================

static void OuterInclude()
{
	// Don't include inside an import
	if(ImportMode != IMPORT_Importing)
	{
		MS_Message(MSG_DEBUG, "---- OuterInclude ----\n");
		TK_NextTokenMustBe(TK_STRING, ERR_STRING_LIT_NOT_FOUND);
		TK_Include(tk_String);
	}
	else
	{
		TK_NextToken();
	}
	TK_NextToken();
}

//==========================================================================
//
// OuterImport
//
//==========================================================================

static void OuterImport()
{

	MS_Message(MSG_DEBUG, "---- OuterImport ----\n");
	if(ImportMode == IMPORT_Importing)
	{
		// Don't import inside an import
		TK_NextToken();
	}
	else
	{
		MS_Message(MSG_DEBUG, "Importing a file\n");
		TK_NextTokenMustBe(TK_STRING, ERR_STRING_LIT_NOT_FOUND);
		TK_Import(tk_String, ImportMode);
	}
	TK_NextToken();
}

//==========================================================================
//
// ProcessStatement
//
//==========================================================================

static bool ProcessStatement(statement_t owner)
{
	if(StatementIndex == MAX_STATEMENT_DEPTH)
	{
		ERR_Exit(ERR_STATEMENT_OVERFLOW, true);
	}
	StatementHistory[StatementIndex++] = owner;
	StatementLevel += AdjustStmtLevel[owner];
	switch(tk_Token)
	{
		case TK_INT:
		case TK_STR:
		case TK_BOOL:
		case TK_STATIC:
			LeadingVarDeclare();
			break;
		case TK_LINESPECIAL:
			if (tk_SpecialValue >= 0)
			{
				LeadingLineSpecial(false);
			}
			else
			{
				LeadingFunction(false);
			}
			break;
		case TK_ACSEXECUTEWAIT:
			tk_SpecialArgCount = 1 | (5<<16);
			tk_SpecialValue = 80;
			LeadingLineSpecial(true);
			break;
		case TK_ACSNAMEDEXECUTEWAIT:
			tk_SpecialArgCount = 1 | (5<<16);
			tk_SpecialValue = -39;
			LeadingFunction(true);
			break;
		case TK_RESTART:
			LeadingRestart();
			break;
		case TK_SUSPEND:
			LeadingSuspend();
			break;
		case TK_TERMINATE:
			LeadingTerminate();
			break;
		case TK_RETURN:
			LeadingReturn();
			break;
		case TK_IDENTIFIER:
			LeadingIdentifier();
			break;
		case TK_PRINT:
		case TK_PRINTBOLD:
		case TK_LOG:
			LeadingPrint();
			break;

		case TK_STRPARAM_EVAL:
			LeadingPrint();
			PC_AppendCmd(PCD_DROP);
			/* Duplicate code: LeadingPrint() post-processing: */
			TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
			TK_NextToken();
			break;

		case TK_STRCPY:
			LeadingStrcpy();
			PC_AppendCmd(PCD_DROP);
			TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
			TK_NextToken();
			break;

		case TK_HUDMESSAGE:
		case TK_HUDMESSAGEBOLD:
			LeadingHudMessage();
			break;
		case TK_IF:
			LeadingIf();
			break;
		case TK_FOR:
			LeadingFor();
			break;
		case TK_WHILE:
		case TK_UNTIL:
			LeadingWhileUntil();
			break;
		case TK_DO:
			LeadingDo();
			break;
		case TK_SWITCH:
			LeadingSwitch();
			break;
		case TK_CASE:
			if(owner != STMT_SWITCH)
			{
				ERR_Error(ERR_CASE_NOT_IN_SWITCH, true);
				TK_SkipPast(TK_COLON);
			}
			else
			{
				LeadingCase();
			}
			break;
		case TK_DEFAULT:
			if(owner != STMT_SWITCH)
			{
				ERR_Error(ERR_DEFAULT_NOT_IN_SWITCH, true);
				TK_SkipPast(TK_COLON);
			}
			else if(DefaultInCurrent() == true)
			{
				ERR_Error(ERR_MULTIPLE_DEFAULT, true);
				TK_SkipPast(TK_COLON);
			}
			else
			{
				LeadingDefault();
			}
			break;
		case TK_BREAK:
			if(BreakAncestor() == false)
			{
				ERR_Error(ERR_MISPLACED_BREAK, true);
				TK_SkipPast(TK_SEMICOLON);
			}
			else
			{
				LeadingBreak();
			}
			break;
		case TK_CONTINUE:
			if(ContinueAncestor() == false)
			{
				ERR_Error(ERR_MISPLACED_CONTINUE, true);
				TK_SkipPast(TK_SEMICOLON);
			}
			else
			{
				LeadingContinue();
			}
			break;
		case TK_CREATETRANSLATION:
			LeadingCreateTranslation();
			break;
		case TK_LBRACE:
			LeadingCompoundStatement(owner);
			break;
		case TK_SEMICOLON:
			TK_NextToken();
			break;
		case TK_INC:
		case TK_DEC:
			LeadingIncDec(tk_Token);
			break;

		default:
			StatementIndex--;
			StatementLevel -= AdjustStmtLevel[owner];
			return false;
			break;
	}
	StatementIndex--;
	StatementLevel -= AdjustStmtLevel[owner];
	return true;
}

//==========================================================================
//
// LeadingCompoundStatement
//
//==========================================================================

static void LeadingCompoundStatement(statement_t owner)
{
	//StatementLevel += AdjustStmtLevel[owner];
	TK_NextToken(); // Eat the TK_LBRACE
	do {} while(ProcessStatement(owner) == true);
	TK_TokenMustBe(TK_RBRACE, ERR_INVALID_STATEMENT);
	TK_NextToken();
	//StatementLevel -= AdjustStmtLevel[owner];
}

//==========================================================================
//
// LeadingVarDeclare
//
//==========================================================================

static void LeadingVarDeclare()
{
	symbolNode_t *sym = NULL;

	if(tk_Token == TK_STATIC)
	{
		TK_NextToken();
		if(tk_Token != TK_INT && tk_Token != TK_STR && tk_Token != TK_BOOL)
		{
			ERR_Error(ERR_BAD_VAR_TYPE, true);
			TK_Undo();
		}
		OuterMapVar(true);
		return;
	}

	MS_Message(MSG_DEBUG, "---- LeadingVarDeclare ----\n");
	do
	{
		TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INVALID_IDENTIFIER);
#if 0
		if(ScriptVarCount == MAX_SCRIPT_VARIABLES)
		{
			ERR_Error(InsideFunction
				? ERR_TOO_MANY_FUNCTION_VARS
				: ERR_TOO_MANY_SCRIPT_VARS,
				true, NULL);
			ScriptVarCount++;
		}
		else
#endif 
		if(SY_FindLocal(tk_String) != NULL)
		{ // Redefined
			ERR_Error(ERR_REDEFINED_IDENTIFIER, true, tk_String);
		}
		else
		{
			sym = SY_InsertLocal(tk_String, SY_SCRIPTVAR);
			sym->info.var.index = ScriptVarCount;
			ScriptVarCount++;
		}
		TK_NextToken();
		if(tk_Token == TK_LBRACKET)
		{
			ERR_Error(ERR_ARRAY_MAPVAR_ONLY, true);
			do {} while(TK_NextToken() != TK_COMMA && tk_Token != TK_SEMICOLON);
		}
		else if(tk_Token == TK_ASSIGN)
		{
			TK_NextToken();
			EvalExpression();
			if(sym != NULL)
			{
				PC_AppendCmd(PCD_ASSIGNSCRIPTVAR);
				PC_AppendShrink(sym->info.var.index);
			}
		}
	} while(tk_Token == TK_COMMA);
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// LeadingLineSpecial
//
//==========================================================================

static void LeadingLineSpecial(bool executewait)
{
	int i;
	int argCount;
	int argCountMin;
	int argCountMax;
	int argSave[8];
	int specialValue;
	bool direct;

	MS_Message(MSG_DEBUG, "---- LeadingLineSpecial ----\n");
	argCountMin = tk_SpecialArgCount & 0xffff;
	argCountMax = tk_SpecialArgCount >> 16;
	specialValue = tk_SpecialValue;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	i = 0;
	if(argCountMax > 0)
	{
		if(TK_NextToken() == TK_CONST)
		{
			TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
			direct = true;
		}
		else
		{
			TK_Undo();
			direct = false;
		}
		do
		{
			if(i == argCountMax)
			{
				ERR_Error(ERR_BAD_LSPEC_ARG_COUNT, true);
				i = argCountMax+1;
			}
			TK_NextToken();
			if(direct == true)
			{
				argSave[i] = EvalConstExpression();
			}
			else
			{
				EvalExpression();
				if (i == 0 && executewait)
				{
					PC_AppendCmd(PCD_DUP);
				}
			}
			if(i < argCountMax)
			{
				i++;
			}
		} while(tk_Token == TK_COMMA);
		if(i < argCountMin)
		{
			ERR_Error(ERR_BAD_LSPEC_ARG_COUNT, true);
			TK_SkipPast(TK_SEMICOLON);
			return;
		}
		argCount = i;
	}
	else
	{
		// [RH] I added some zero-argument specials without realizing that
		// ACS won't allow for less than one, so fake them as one-argument
		// specials with a parameter of 0.
		argCount = 1;
		direct = true;
		argSave[0] = 0;
		TK_NextToken ();
	}
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	if(direct == false)
	{
		PC_AppendCmd(PCD_LSPEC1+(argCount-1));
		if(pc_NoShrink)
		{
			PC_AppendInt(specialValue);
		}
		else
		{
			if (specialValue > 255)
			{
				ERR_Error(ERR_SPECIAL_RANGE, true);
			}
			PC_AppendByte((byte)specialValue);
		}
		if(executewait)
		{
			PC_AppendCmd(PCD_SCRIPTWAIT);
		}
	}
	else
	{
		bool useintform;

		if(pc_NoShrink)
		{
			PC_AppendCmd(PCD_LSPEC1DIRECT+(argCount-1));
			PC_AppendInt(specialValue);
			useintform = true;
		}
		else
		{
			useintform = false;
			for (i = 0; i < argCount; i++)
			{
				if ((int)argSave[i] > 255)
				{
					useintform = true;
					break;
				}
			}
			PC_AppendCmd((argCount-1)+(useintform?PCD_LSPEC1DIRECT:PCD_LSPEC1DIRECTB));
			PC_AppendByte((byte)specialValue);
		}
		if (useintform)
		{
			for (i = 0; i < argCount; i++)
			{
				PC_AppendInt(argSave[i]);
			}
		}
		else
		{
			for (i = 0; i < argCount; i++)
			{
				PC_AppendByte((byte)argSave[i]);
			}
		}
		if(executewait)
		{
			PC_AppendCmd(PCD_SCRIPTWAITDIRECT);
			PC_AppendInt(argSave[0]);
		}
	}
	TK_NextToken();
}

//==========================================================================
//
// LeadingLineSpecial
//
//==========================================================================

static void LeadingFunction(bool executewait)
{
	int i;
	int argCount;
	int argCountMin;
	int argCountMax;
	int specialValue;

	MS_Message(MSG_DEBUG, "---- LeadingFunction ----\n");
	argCountMin = tk_SpecialArgCount & 0xffff;
	argCountMax = tk_SpecialArgCount >> 16;
	specialValue = -tk_SpecialValue;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	i = 0;
	if(argCountMax > 0)
	{
		if(TK_NextToken() == TK_CONST)
		{
			// Just skip const declarators
			TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
		}
		else
		{
			TK_Undo();
		}
		do
		{
			if(i == argCountMax)
			{
				ERR_Error(ERR_BAD_ARG_COUNT, true);
				i = argCountMax+1;
			}
			TK_NextToken();
			EvalExpression();
			if (i == 0 && executewait)
			{
				PC_AppendCmd(PCD_DUP);
			}
			if(i < argCountMax)
			{
				i++;
			}
		} while(tk_Token == TK_COMMA);
		if(i < argCountMin)
		{
			ERR_Error(ERR_BAD_ARG_COUNT, true);
			TK_SkipPast(TK_SEMICOLON);
			return;
		}
		argCount = i;
	}
	else
	{
		argCount = 0;
		TK_NextToken ();
	}
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_CALLFUNC);
	if(pc_NoShrink)
	{
		PC_AppendInt(argCount);
		PC_AppendInt(specialValue);
	}
	else
	{
		PC_AppendByte((byte)argCount);
		PC_AppendWord((short)specialValue);
	}
	PC_AppendCmd(PCD_DROP);
	if(executewait)
	{
		PC_AppendCmd(PCD_SCRIPTWAITNAMED);
	}
	TK_NextToken();
}

//==========================================================================
//
// LeadingIdentifier
//
//==========================================================================

static void LeadingIdentifier()
{
	ACS_Node *node = SpeculateSymbol(tk_String, false);

	switch (node->type)
	{
		case SY_MAPARRAY:
		case SY_SCRIPTVAR:
		case SY_SCRIPTALIAS:
		case SY_MAPVAR:
		case SY_WORLDVAR:
		case SY_GLOBALVAR:
		case SY_WORLDARRAY:
		case SY_GLOBALARRAY:
			LeadingVarAssign(node);
			break;
		case SY_INTERNFUNC:
			LeadingInternFunc(node);
			break;
		case SY_SCRIPTFUNC:
			LeadingScriptFunc(node);
			break;
		default:
			break;
	}
}

//==========================================================================
//
// LeadingInternFunc
//
//==========================================================================

static void LeadingInternFunc(ACS_Node *node)
{
	if (InsideFunction && node->cmd->isLatent)
	{
		ERR_Error(ERR_LATENT_IN_FUNC, true);
	}
	ProcessInternFunc(node);
	if(node->cmd->type != VAR_VOID)
	{
		PC_AppendCmd(PCD_DROP);
	}
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// ProcessInternFunc
//
//==========================================================================

static void ProcessInternFunc(ACS_Node *node)
{
	int i;
	int argCount;
	int optMask;
	int outMask;
	bool direct;
	bool specialDirect;
	int argSave[8];

	MS_Message(MSG_DEBUG, "---- ProcessInternFunc ----\n");
	argCount = node->cmd->argTypes.size();
	optMask = node->cmd->optMask;
	outMask = node->cmd->outMask;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	if(TK_NextToken() == TK_CONST)
	{
		TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
		if(node->cmd->DirectCMD == PCD_NOP)
		{
			ERR_Error(ERR_NO_DIRECT_VER, true, NULL);
			direct = false;
			specialDirect = false;
		}
		else
		{
			direct = true;
			if (pc_NoShrink || argCount > 2 ||
				(node->cmd->DirectCMD != PCD_DELAYDIRECT &&
				node->cmd->DirectCMD != PCD_RANDOMDIRECT))
			{
				specialDirect = false;
				PC_AppendCmd(node->cmd->DirectCMD);
			}
			else
			{
				specialDirect = true;
			}
		}
		TK_NextToken();
	}
	else
	{
		direct = false;
		specialDirect = false;	// keep GCC quiet
	}
	i = 0;
	if(argCount > 0)
	{
		if(tk_Token == TK_RPAREN)
		{
			ERR_Error(ERR_MISSING_PARAM, true);
		}
		else
		{
			TK_Undo(); // Adjust for first expression
			do
			{
				if(i == argCount)
				{
					ERR_Error(ERR_BAD_ARG_COUNT, true);
					TK_SkipTo(TK_SEMICOLON);
					TK_Undo();
					return;
				}
				TK_NextToken();
				if(direct == true)
				{
					if (tk_Token != TK_COMMA)
					{
						if (specialDirect)
						{
							argSave[i] = EvalConstExpression();
						}
						else
						{
							PC_AppendInt(EvalConstExpression());
						}
					}
					else
					{
						if (optMask & 1)
						{
							if (specialDirect)
							{
								argSave[i] = 0;
							}
							else
							{
								PC_AppendInt(0);
							}
						}
						else
						{
							ERR_Error(ERR_MISSING_PARAM, true);
						}
					}
				}
				else
				{
					if (tk_Token != TK_COMMA)
					{
						if (!(outMask & 1))
						{
							EvalExpression();
						}
						else if (tk_Token != TK_IDENTIFIER)
						{
							ERR_Error (ERR_PARM_MUST_BE_VAR, true);
							do
							{
								TK_NextToken();
							} while (tk_Token != TK_COMMA && tk_Token != TK_RPAREN);
						}
						else
						{
							ACS_Node *node = new ACS_Node(NODE_VARIABLE, DemandSymbol (tk_String));
							PC_AppendCmd (PCD_PUSHNUMBER);
							switch (node->type)
							{
							case SY_SCRIPTVAR:
								PC_AppendInt(sym->info.var.index | OUTVAR_SCRIPT_SPEC);
								break;
							case SY_MAPVAR:
								PC_AppendInt(sym->info.var.index | OUTVAR_MAP_SPEC);
								break;
							case SY_WORLDVAR:
								PC_AppendInt(sym->info.var.index | OUTVAR_WORLD_SPEC);
								break;
							case SY_GLOBALVAR:
								PC_AppendInt(sym->info.var.index | OUTVAR_GLOBAL_SPEC);
								break;
							default:
								ERR_Error (ERR_PARM_MUST_BE_VAR, true);
								do
								{
									TK_NextToken();
								} while (tk_Token != TK_COMMA && tk_Token != TK_RPAREN);
								break;
							}
							TK_NextToken ();
						}
					}
					else
					{
						if (optMask & 1)
						{
							PC_AppendPushVal(0);
						}
						else
						{
							ERR_Error(ERR_MISSING_PARAM, true);
						}
					}
				}
				i++;
				optMask >>= 1;
				outMask >>= 1;
			} while(tk_Token == TK_COMMA);
		}
	}
	while (i < argCount && (optMask & 1))
	{
		if (direct == true)
		{
			if (specialDirect)
			{
				argSave[i] = 0;
			}
			else
			{
				PC_AppendInt(0);
			}
		}
		else
		{
			PC_AppendPushVal(0);
		}
		i++;
		optMask >>= 1;
	}
	if(i != argCount && i > 0)
	{
		ERR_Error(ERR_BAD_ARG_COUNT, true);
	}
	TK_TokenMustBe(TK_RPAREN, argCount > 0 ? ERR_MISSING_RPAREN : ERR_BAD_ARG_COUNT);
	if(direct == false)
	{
		PC_AppendCmd(sym->info.internFunc.stackCommand);
	}
	else if (specialDirect)
	{
		bool useintform = false;
		pcd_t shortpcd;

		switch (node->cmd->DirectCMD)
		{
		case PCD_DELAYDIRECT:
			shortpcd = PCD_DELAYDIRECTB;
			break;
		case PCD_RANDOMDIRECT:
			shortpcd = PCD_RANDOMDIRECTB;
			break;
		default:
			useintform = true;
			shortpcd = PCD_NOP;
			break;
		}

		if (!useintform)
		{
			for (i = 0; i < argCount; i++)
			{
				if ((int)argSave[i] > 255)
				{
					useintform = true;
					break;
				}
			}
		}

		if (useintform)
		{
			PC_AppendCmd(sym->info.internFunc.directCommand);
			for (i = 0; i < argCount; i++)
			{
				PC_AppendInt (argSave[i]);
			}
		}
		else
		{
			PC_AppendCmd (shortpcd);
			for (i = 0; i < argCount; i++)
			{
				PC_AppendByte ((byte)argSave[i]);
			}
		}
	}
	TK_NextToken();
}

//==========================================================================
//
// LeadingScriptFunc
//
//==========================================================================

static void LeadingScriptFunc(ACS_Node *node)
{
	ProcessScriptFunc(node, true);
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}


//==========================================================================
//
// ProcessScriptFunc
//
//==========================================================================

static void ProcessScriptFunc(ACS_Node *sym, bool discardReturn)
{
	int i;
	int argCount;

	MS_Message(MSG_DEBUG, "---- ProcessScriptFunc ----\n");
	argCount = sym->info.scriptFunc.argCount;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	i = 0;
	if(argCount == 0)
	{
		TK_NextTokenMustBe(TK_RPAREN, ERR_BAD_ARG_COUNT);
	}
	else if(argCount > 0)
	{
		TK_NextToken();
		if(tk_Token == TK_RPAREN)
		{
			ERR_Error(ERR_BAD_ARG_COUNT, true);
			TK_SkipTo(TK_SEMICOLON);
			return;
		}
		TK_Undo();
		do
		{
			if(i == argCount)
			{
				ERR_Error(ERR_BAD_ARG_COUNT, true);
				TK_SkipTo(TK_SEMICOLON);
				return;
			}
			TK_NextToken();
			if (tk_Token != TK_COMMA)
			{
				EvalExpression();
			}
			else
			{
				ERR_Error(ERR_MISSING_PARAM, true);
				TK_SkipTo(TK_SEMICOLON);
				return;
			}
			i++;
		} while(tk_Token == TK_COMMA);
	}
	if(argCount < 0)
	{ // Function has not been defined yet, so assume arg count is correct
		TK_NextToken();
		while (tk_Token != TK_RPAREN)
		{
			EvalExpression();
			i++;
			if (tk_Token == TK_COMMA)
			{
				TK_NextToken();
			}
			else if (tk_Token != TK_RPAREN)
			{
				ERR_Error(ERR_MISSING_PARAM, true);
				TK_SkipTo(TK_SEMICOLON);
				return;
			}
		}
	}
	else if(i != argCount)
	{
		ERR_Error(ERR_BAD_ARG_COUNT, true);
		TK_SkipTo(TK_SEMICOLON);
		return;
	}
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	PC_AppendCmd(discardReturn ? PCD_CALLDISCARD : PCD_CALL);
	if(sym->info.scriptFunc.predefined && ImportMode != IMPORT_Importing)
	{
		AddScriptFuncRef(sym, pc_Address, i);
	}
	if (pc_NoShrink)
	{
		PC_AppendInt(sym->info.scriptFunc.funcNumber);
	}
	else
	{
		PC_AppendByte((byte)sym->info.scriptFunc.funcNumber);
	}
	TK_NextToken();
}

//==========================================================================
//
// BuildPrintString
//
//==========================================================================

static void BuildPrintString()
{
	pcd_t printCmd;

	do
	{
		switch(TK_NextCharacter())
		{
			case 'a': // character array support [JB]
				ActionOnCharRange(false);
				continue;
			case 's': // string
				printCmd = PCD_PRINTSTRING;
				break;
			case 'l': // [RH] localized string
				printCmd = PCD_PRINTLOCALIZED;
				break;
			case 'i': // integer
			case 'd': // decimal
				printCmd = PCD_PRINTNUMBER;
				break;
			case 'c': // character
				printCmd = PCD_PRINTCHARACTER;
				break;
			case 'n': // [BC] name
				printCmd = PCD_PRINTNAME;
				break;
			case 'f': // [RH] fixed point
				printCmd = PCD_PRINTFIXED;
				break;
			case 'k': // [GRB] key binding
				printCmd = PCD_PRINTBIND;
				break;
			case 'b': // [RH] binary integer
				printCmd = PCD_PRINTBINARY;
				break;
			case 'x': // [RH] hexadecimal integer
				printCmd = PCD_PRINTHEX;
				break;
			default:
				printCmd = PCD_PRINTSTRING;
				ERR_Error(ERR_UNKNOWN_PRTYPE, true);
				break;
		}
		TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
		TK_NextToken();
		EvalExpression();
		PC_AppendCmd(printCmd);
	} while(tk_Token == TK_COMMA);
}

//==========================================================================
//
// ActionOnCharRange // FDARI (mutation of PrintCharArray // JB)
//
//==========================================================================

static void ActionOnCharRange(bool write)
{
	bool rangeConstraints;
	symbolNode_t *sym;
	TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
	TK_NextToken();

	if (tk_Token == TK_LPAREN)
	{
		rangeConstraints = true;
		TK_NextToken();
	}
	else
	{
		rangeConstraints = false;
	}

	sym = SpeculateSymbol(tk_String, false);
	if((sym->type != SY_MAPARRAY) && (sym->type != SY_WORLDARRAY)
		&& (sym->type != SY_GLOBALARRAY))
	{
		ERR_Error(ERR_NOT_AN_ARRAY, true);
	}
	TK_NextToken();
	if(sym->info.array.ndim > 1)
	{
		ParseArrayIndices(sym, sym->info.array.ndim-1);
	}
	else
	{
		PC_AppendPushVal(0);
	}

	PC_AppendPushVal(sym->info.array.index);

	
	if (rangeConstraints)
	{
		switch (tk_Token)
		{
		case TK_RPAREN:
			rangeConstraints = false;
			TK_NextToken();
			break;
		case TK_COMMA:
			TK_NextToken();
			EvalExpression();

			switch (tk_Token)
			{
			case TK_RPAREN:
				TK_NextToken();
				PC_AppendPushVal(0x7FFFFFFF); 
				break;
			case TK_COMMA:
				TK_NextToken();
				EvalExpression(); // limit on capacity
				TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
				TK_NextToken();
				break;
			default:
				ERR_Error(ERR_MISSING_RPAREN, true);
				break;
			}

			break;
		default:
			ERR_Error(ERR_MISSING_RPAREN, true);
			break;
		}
	}

	if (write)
	{
		if (!rangeConstraints)
		{
			PC_AppendPushVal(0);
			PC_AppendPushVal(0x7FFFFFFF);
		}

		TK_TokenMustBe(TK_COMMA, ERR_MISSING_COMMA);
		TK_NextToken();
		EvalExpression();

		if (tk_Token == TK_COMMA)
		{
			TK_NextToken();
			EvalExpression();
		}
		else
		{
			PC_AppendPushVal(0);
		}
	}

	if(sym->type == SY_MAPARRAY)
	{
		if (write) PC_AppendCmd(PCD_STRCPYTOMAPCHRANGE);
		else PC_AppendCmd( rangeConstraints ? PCD_PRINTMAPCHRANGE : PCD_PRINTMAPCHARARRAY );
	}
	else if(sym->type == SY_WORLDARRAY)
	{
		if (write) PC_AppendCmd(PCD_STRCPYTOWORLDCHRANGE);
		else PC_AppendCmd( rangeConstraints ? PCD_PRINTWORLDCHRANGE : PCD_PRINTWORLDCHARARRAY );
	}
	else // if(sym->type == SY_GLOBALARRAY)
	{
		if (write) PC_AppendCmd(PCD_STRCPYTOGLOBALCHRANGE);
		else PC_AppendCmd( rangeConstraints ? PCD_PRINTGLOBALCHRANGE : PCD_PRINTGLOBALCHARARRAY );
	}
}

//==========================================================================
//
// LeadingStrcpy
//
//==========================================================================

static void LeadingStrcpy()
{
	MS_Message(MSG_DEBUG, "---- LeadingStrcpy ----\n");
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	
	switch(TK_NextCharacter()) // structure borrowed from printbuilder
	{
		case 'a':
			ActionOnCharRange(true);
			break;

		default:
			ERR_Error(ERR_UNKNOWN_PRTYPE, true);
			break;
	}

	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
}

//==========================================================================
//
// LeadingPrint
//
//==========================================================================

static void LeadingPrint()
{
	tokenType_t stmtToken;

	MS_Message(MSG_DEBUG, "---- LeadingPrint ----\n");
	stmtToken = tk_Token; // Will be TK_PRINT or TK_PRINTBOLD, TK_LOG or TK_STRPARAM_EVAL [FDARI]
	PC_AppendCmd(PCD_BEGINPRINT);
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	BuildPrintString();
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);

	switch (stmtToken)
	{
	case TK_PRINT:
		PC_AppendCmd(PCD_ENDPRINT);
		break;

	case TK_PRINTBOLD:
		PC_AppendCmd(PCD_ENDPRINTBOLD);
		break;

	case TK_STRPARAM_EVAL:
		PC_AppendCmd(PCD_SAVESTRING);
		return; // THE CALLER MUST DO THE POST-PROCESSING

	case TK_LOG:
	default:
		PC_AppendCmd(PCD_ENDLOG);
		break;
	}

	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// LeadingHudMessage
//
// hudmessage(str text; int type, int id, int color, fixed x, fixed y, fixed holdtime, ...)
//
//==========================================================================

static void LeadingHudMessage()
{
	tokenType_t stmtToken;
	int i;

	MS_Message(MSG_DEBUG, "---- LeadingHudMessage ----\n");
	stmtToken = tk_Token; // Will be TK_HUDMESSAGE or TK_HUDMESSAGEBOLD
	PC_AppendCmd(PCD_BEGINPRINT);
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	BuildPrintString();
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_PARAM);
	PC_AppendCmd(PCD_MOREHUDMESSAGE);
	for (i = 6; i > 0; i--)
	{
		TK_NextToken();
		EvalExpression();
		if (i > 1)
			TK_TokenMustBe(TK_COMMA, ERR_MISSING_PARAM);
	}
	if (tk_Token == TK_COMMA)
	{ // HUD message has optional parameters
		PC_AppendCmd(PCD_OPTHUDMESSAGE);
		do
		{
			TK_NextToken();
			EvalExpression();
		} while (tk_Token == TK_COMMA);
	}
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	PC_AppendCmd(stmtToken == TK_HUDMESSAGE ? 
		PCD_ENDHUDMESSAGE : PCD_ENDHUDMESSAGEBOLD);
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// LeadingCreateTranslation
//
// Simple grammar:
//
// tranlationstmt: CreateTranslation ( exp opt_args ) ;
// opt_args: /* empty: just reset the translation */ | , arglist
// arglist: arg | arglist arg
// arg: range = replacement
// range: exp : exp
// replacement: palrep | colorrep
// palrep: exp : exp
// colorrep: [exp,exp,exp]:[exp,exp,exp]
// desatrep: %colorrep
//==========================================================================

static void LeadingCreateTranslation()
{
	MS_Message(MSG_DEBUG, "---- LeadingCreateTranslation ----\n");
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	TK_NextToken();
	EvalExpression();
	PC_AppendCmd(PCD_STARTTRANSLATION);
	while (tk_Token == TK_COMMA)
	{
		pcd_t translationcode;

		TK_NextToken();
		EvalExpression();	// Get first palette entry in range
		TK_TokenMustBe(TK_COLON, ERR_MISSING_COLON);
		TK_NextToken();
		EvalExpression();	// Get second palette entry in range
		TK_TokenMustBe(TK_ASSIGN, ERR_MISSING_ASSIGN);

		TK_NextToken();
		if(tk_Token == TK_PERCENT)
		{
			translationcode = PCD_TRANSLATIONRANGE3;
			TK_NextTokenMustBe(TK_LBRACKET, ERR_MISSING_LBRACKET);
		}
		else
		{
			translationcode = PCD_TRANSLATIONRANGE2;
		}
		if(tk_Token == TK_LBRACKET)
		{ // Replacement is color range
			int i, j;

			TK_NextToken();

			for(j = 2; j != 0; --j)
			{
				for(i = 3; i != 0; --i)
				{
					EvalExpression();
					if(i != 1)
					{
						TK_TokenMustBe(TK_COMMA, ERR_MISSING_COMMA);
					}
					else
					{
						TK_TokenMustBe(TK_RBRACKET, ERR_MISSING_RBRACKET);
					}
					TK_NextToken();
				}
				if(j == 2)
				{
					TK_TokenMustBe(TK_COLON, ERR_MISSING_COLON);
					TK_NextTokenMustBe(TK_LBRACKET, ERR_MISSING_LBRACKET);
					TK_NextToken();
				}
			}
		}
		else
		{ // Replacement is palette range
			EvalExpression();
			TK_TokenMustBe(TK_COLON, ERR_MISSING_COLON);
			TK_NextToken();
			EvalExpression();
			translationcode = PCD_TRANSLATIONRANGE1;
		}
		PC_AppendCmd(translationcode);
	}
	PC_AppendCmd(PCD_ENDTRANSLATION);
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
}

//==========================================================================
//
// LeadingIf
//
//==========================================================================

static void LeadingIf()
{
	int jumpAddrPtr1;
	int jumpAddrPtr2;

	MS_Message(MSG_DEBUG, "---- LeadingIf ----\n");
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	TK_NextToken();
	EvalExpression();
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	PC_AppendCmd(PCD_IFNOTGOTO);
	jumpAddrPtr1 = pc_Address;
	PC_SkipInt();
	TK_NextToken();
	if(ProcessStatement(STMT_IF) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	if(tk_Token == TK_ELSE)
	{
		PC_AppendCmd(PCD_GOTO);
		jumpAddrPtr2 = pc_Address;
		PC_SkipInt();
		PC_WriteInt(pc_Address, jumpAddrPtr1);
		TK_NextToken();
		if(ProcessStatement(STMT_ELSE) == false)
		{
			ERR_Error(ERR_INVALID_STATEMENT, true);
		}
		PC_WriteInt(pc_Address, jumpAddrPtr2);
	}
	else
	{
		PC_WriteInt(pc_Address, jumpAddrPtr1);
	}
}

//==========================================================================
//
// LeadingFor
//
//==========================================================================

static void LeadingFor()
{
	int exprAddr;
	int incAddr;
	int ifgotoAddr;
	int	gotoAddr;

	MS_Message(MSG_DEBUG, "---- LeadingFor ----\n");
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	TK_NextToken();
	if(ProcessStatement(STMT_IF) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	exprAddr = pc_Address;
	EvalExpression();
	TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	TK_NextToken();
	PC_AppendCmd(PCD_IFGOTO);
	ifgotoAddr = pc_Address;
	PC_SkipInt();
	PC_AppendCmd(PCD_GOTO);
	gotoAddr = pc_Address;
	PC_SkipInt();
	incAddr = pc_Address;
	forSemicolonHack = true;
	if(ProcessStatement(STMT_IF) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	forSemicolonHack = false;
	PC_AppendCmd(PCD_GOTO);
	PC_AppendInt(exprAddr);
	PC_WriteInt(pc_Address,ifgotoAddr);
	if(ProcessStatement(STMT_FOR) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	PC_AppendCmd(PCD_GOTO);
	PC_AppendInt(incAddr);
	WriteContinues(incAddr);
	WriteBreaks();
	PC_WriteInt(pc_Address,gotoAddr);
}

//==========================================================================
//
// LeadingWhileUntil
//
//==========================================================================

static void LeadingWhileUntil()
{
	tokenType_t stmtToken;
	int topAddr;
	int outAddrPtr;

	MS_Message(MSG_DEBUG, "---- LeadingWhileUntil ----\n");
	stmtToken = tk_Token;
	topAddr = pc_Address;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	TK_NextToken();
	EvalExpression();
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	PC_AppendCmd(stmtToken == TK_WHILE ? PCD_IFNOTGOTO : PCD_IFGOTO);
	outAddrPtr = pc_Address;
	PC_SkipInt();
	TK_NextToken();
	if(ProcessStatement(STMT_WHILEUNTIL) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true);
	}
	PC_AppendCmd(PCD_GOTO);
	PC_AppendInt(topAddr);

	PC_WriteInt(pc_Address, outAddrPtr);

	WriteContinues(topAddr);
	WriteBreaks();
}

//==========================================================================
//
// LeadingDo
//
//==========================================================================

static void LeadingDo()
{
	int topAddr;
	int exprAddr;
	tokenType_t stmtToken;

	MS_Message(MSG_DEBUG, "---- LeadingDo ----\n");
	topAddr = pc_Address;
	TK_NextToken();
	if(ProcessStatement(STMT_DO) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true, NULL);
	}
	if(tk_Token != TK_WHILE && tk_Token != TK_UNTIL)
	{
		ERR_Error(ERR_BAD_DO_STATEMENT, true, NULL);
		TK_SkipPast(TK_SEMICOLON);
		return;
	}
	stmtToken = tk_Token;
	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	exprAddr = pc_Address;
	TK_NextToken();
	EvalExpression();
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(stmtToken == TK_WHILE ? PCD_IFGOTO : PCD_IFNOTGOTO);
	PC_AppendInt(topAddr);
	WriteContinues(exprAddr);
	WriteBreaks();
	TK_NextToken();
}

//==========================================================================
//
// LeadingSwitch
//
//==========================================================================

static void LeadingSwitch()
{
	int switcherAddrPtr;
	int outAddrPtr;
	caseInfo_t *cInfo;
	int defaultAddress;

	MS_Message(MSG_DEBUG, "---- LeadingSwitch ----\n");

	TK_NextTokenMustBe(TK_LPAREN, ERR_MISSING_LPAREN);
	TK_NextToken();
	EvalExpression();
	TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);

	PC_AppendCmd(PCD_GOTO);
	switcherAddrPtr = pc_Address;
	PC_SkipInt();

	TK_NextToken();
	if(ProcessStatement(STMT_SWITCH) == false)
	{
		ERR_Error(ERR_INVALID_STATEMENT, true, NULL);
	}

	PC_AppendCmd(PCD_GOTO);
	outAddrPtr = pc_Address;
	PC_SkipInt();

	PC_WriteInt(pc_Address, switcherAddrPtr);
	defaultAddress = 0;

	if(pc_HexenCase)
	{
		while((cInfo = GetCaseInfo()) != NULL)
		{
			if(cInfo->isDefault == true)
			{
				defaultAddress = cInfo->address;
				continue;
			}
			PC_AppendCmd(PCD_CASEGOTO);
			PC_AppendInt(cInfo->value);
			PC_AppendInt(cInfo->address);
		}
	}
	else if(CaseIndex != 0)
	{
		caseInfo_t *maxCase = &CaseInfo[CaseIndex];
		caseInfo_t *minCase = maxCase;

		// [RH] Sort cases so that the VM can handle them with
		// a quick binary search.
		while((cInfo = GetCaseInfo()) != NULL)
		{
			minCase = cInfo;
		}
		qsort(minCase, maxCase - minCase, sizeof(caseInfo_t), CaseInfoCmp);
		if(minCase->isDefault == true)
		{
			defaultAddress = minCase->address;
			minCase++;
		}
		if (minCase < maxCase)
		{
			PC_AppendCmd(PCD_CASEGOTOSORTED);
			if(pc_Address%4 != 0)
			{ // Align to a 4-byte boundary
				int pad = 0;
				PC_Append((void *)&pad, 4-(pc_Address%4));
			}
			PC_AppendInt(maxCase - minCase);
			for(; minCase < maxCase; ++minCase)
			{
				PC_AppendInt(minCase->value);
				PC_AppendInt(minCase->address);
			}
		}
	}
	PC_AppendCmd(PCD_DROP);

	if(defaultAddress != 0)
	{
		PC_AppendCmd(PCD_GOTO);
		PC_AppendInt(defaultAddress);
	}

	PC_WriteInt(pc_Address, outAddrPtr);

	WriteBreaks();
}

//==========================================================================
//
// LeadingCase
//
//==========================================================================

static void LeadingCase()
{
	MS_Message(MSG_DEBUG, "---- LeadingCase ----\n");
	TK_NextToken();
	PushCase(EvalConstExpression(), false);
	TK_TokenMustBe(TK_COLON, ERR_MISSING_COLON);
	TK_NextToken();
}

//==========================================================================
//
// LeadingDefault
//
//==========================================================================

static void LeadingDefault()
{
	MS_Message(MSG_DEBUG, "---- LeadingDefault ----\n");
	TK_NextTokenMustBe(TK_COLON, ERR_MISSING_COLON);
	PushCase(0, true);
	TK_NextToken();
}

//==========================================================================
//
// PushCase
//
//==========================================================================

static void PushCase(int value, bool isDefault)
{
	if(CaseIndex == MAX_CASE)
	{
		ERR_Exit(ERR_CASE_OVERFLOW, true);
	}
	CaseInfo[CaseIndex].level = StatementLevel;
	CaseInfo[CaseIndex].value = value;
	CaseInfo[CaseIndex].isDefault = isDefault;
	CaseInfo[CaseIndex].address = pc_Address;
	CaseIndex++;
}

//==========================================================================
//
// GetCaseInfo
//
//==========================================================================

static caseInfo_t *GetCaseInfo()
{
	if(CaseIndex == 0)
	{
		return NULL;
	}
	if(CaseInfo[CaseIndex-1].level > StatementLevel)
	{
		return &CaseInfo[--CaseIndex];
	}
	return NULL;
}

//==========================================================================
//
// CaseInfoCmp
//
//==========================================================================

static int CaseInfoCmp (const void *a, const void *b)
{
	const caseInfo_t *ca = (const caseInfo_t *)a;
	const caseInfo_t *cb = (const caseInfo_t *)b;

	// The default case always gets moved to the front.
	if(ca->isDefault)
	{
		return -1;
	}
	if(cb->isDefault)
	{
		return 1;
	}
	return ca->value - cb->value;
}

//==========================================================================
//
// DefaultInCurrent
//
//==========================================================================

static bool DefaultInCurrent()
{
	int i;

	for(i = 0; i < CaseIndex; i++)
	{
		if(CaseInfo[i].isDefault == true
			&& CaseInfo[i].level == StatementLevel)
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// LeadingBreak
//
//==========================================================================

static void LeadingBreak()
{
	MS_Message(MSG_DEBUG, "---- LeadingBreak ----\n");
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_GOTO);
	PushBreak();
	PC_SkipInt();
	TK_NextToken();
}

//==========================================================================
//
// PushBreak
//
//==========================================================================

static void PushBreak()
{
	if(BreakIndex == MAX_CASE)
	{
		ERR_Exit(ERR_BREAK_OVERFLOW, true);
	}
	BreakInfo[BreakIndex].level = StatementLevel;
	BreakInfo[BreakIndex].addressPtr = pc_Address;
	BreakIndex++;
}

//==========================================================================
//
// WriteBreaks
//
//==========================================================================

static void WriteBreaks()
{
	while(BreakIndex && BreakInfo[BreakIndex-1].level > StatementLevel)
	{
		PC_WriteInt(pc_Address, BreakInfo[--BreakIndex].addressPtr);
	}
}

//==========================================================================
//
// BreakAncestor
//
// Returns true if the current statement history contains a break root
// statement.
//
//==========================================================================

static bool BreakAncestor()
{
	int i;

	for(i = 0; i < StatementIndex; i++)
	{
		if(IsBreakRoot[StatementHistory[i]])
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// LeadingContinue
//
//==========================================================================

static void LeadingContinue()
{
	MS_Message(MSG_DEBUG, "---- LeadingContinue ----\n");
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_GOTO);
	PushContinue();
	PC_SkipInt();
	TK_NextToken();
}

//==========================================================================
//
// PushContinue
//
//==========================================================================

static void PushContinue()
{
	if(ContinueIndex == MAX_CONTINUE)
	{
		ERR_Exit(ERR_CONTINUE_OVERFLOW, true);
	}
	ContinueInfo[ContinueIndex].level = StatementLevel;
	ContinueInfo[ContinueIndex].addressPtr = pc_Address;
	ContinueIndex++;
}

//==========================================================================
//
// WriteContinues
//
//==========================================================================

static void WriteContinues(int address)
{
	if(ContinueIndex == 0)
	{
		return;
	}
	while(ContinueInfo[ContinueIndex-1].level > StatementLevel)
	{
		PC_WriteInt(address, ContinueInfo[--ContinueIndex].addressPtr);
	}
}

//==========================================================================
//
// ContinueAncestor
//
//==========================================================================

static bool ContinueAncestor()
{
	int i;

	for(i = 0; i < StatementIndex; i++)
	{
		if(IsContinueRoot[StatementHistory[i]])
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// LeadingIncDec
//
//==========================================================================

static void LeadingIncDec(tokenType_t token)
{
	ACS_Node *sym;

	MS_Message(MSG_DEBUG, "---- LeadingIncDec ----\n");
	TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INCDEC_OP_ON_NON_VAR);
	sym = DemandSymbol(tk_String);
	if(sym->type != SY_SCRIPTVAR && sym->type != SY_MAPVAR
		&& sym->type != SY_WORLDVAR && sym->type != SY_GLOBALVAR
		&& sym->type != SY_MAPARRAY && sym->type != SY_GLOBALARRAY
		&& sym->type != SY_WORLDARRAY && sym->type != SY_SCRIPTALIAS)
	{
		ERR_Error(ERR_INCDEC_OP_ON_NON_VAR, true);
		TK_SkipPast(TK_SEMICOLON);
		return;
	}
	TK_NextToken();
	if(sym->type == SY_MAPARRAY || sym->type == SY_WORLDARRAY
		|| sym->type == SY_GLOBALARRAY)
	{
		ParseArrayIndices(sym, sym->info.array.ndim);
	}
	else if(tk_Token == TK_LBRACKET)
	{
		ERR_Error(ERR_NOT_AN_ARRAY, true, sym->name);
		while(tk_Token == TK_LBRACKET)
		{
			TK_SkipPast(TK_RBRACKET);
		}
	}
	PC_AppendCmd(GetIncDecPCD(token, sym->type));
	PC_AppendShrink(sym->info.var.index);
	if(forSemicolonHack)
	{
		TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
	}
	else
	{
		TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	}
	TK_NextToken();
}

//==========================================================================
//
// LeadingVarAssign
//
//==========================================================================

static void LeadingVarAssign(ACS_Node *sym)
{
	bool done;
	tokenType_t assignToken;

	MS_Message(MSG_DEBUG, "---- LeadingVarAssign ----\n");
	done = false;
	do
	{
		TK_NextToken(); // Fetch assignment operator
		if(sym->type == SY_MAPARRAY || sym->type == SY_WORLDARRAY
			|| sym->type == SY_GLOBALARRAY)
		{
			ParseArrayIndices(sym, sym->info.array.ndim);
		}
		else if(tk_Token == TK_LBRACKET)
		{
			ERR_Error(ERR_NOT_AN_ARRAY, true, sym->name);
			while(tk_Token == TK_LBRACKET)
			{
				TK_SkipPast(TK_RBRACKET);
			}
		}
		if(tk_Token == TK_INC || tk_Token == TK_DEC)
		{ // Postfix increment or decrement
			PC_AppendCmd(GetIncDecPCD(tk_Token, sym->type));
			if (pc_NoShrink)
			{
				PC_AppendInt(sym->info.var.index);
			}
			else
			{
				PC_AppendByte(sym->info.var.index);
			}
			TK_NextToken();
		}
		else
		{ // Normal operator
			if(TK_Member((int*)AssignOps) == false)
			{
				ERR_Error(ERR_MISSING_ASSIGN_OP, true);
				TK_SkipPast(TK_SEMICOLON);
				return;
			}
			assignToken = tk_Token;
			TK_NextToken();
			EvalExpression();
			PC_AppendCmd(GetAssignPCD(assignToken, sym->type));
			PC_AppendShrink(sym->info.var.index);
		}
		if(tk_Token == TK_COMMA)
		{
			TK_NextTokenMustBe(TK_IDENTIFIER, ERR_BAD_ASSIGNMENT);
			sym = DemandSymbol(tk_String);
			if(sym->type != SY_SCRIPTVAR && sym->type != SY_MAPVAR
				&& sym->type != SY_WORLDVAR && sym->type != SY_GLOBALVAR
				&& sym->type != SY_WORLDARRAY && sym->type != SY_GLOBALARRAY
				&& sym->type != SY_SCRIPTALIAS)
			{
				ERR_Error(ERR_BAD_ASSIGNMENT, true);
				TK_SkipPast(TK_SEMICOLON);
				return;
			}
		}
		else
		{
			TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
			TK_NextToken();
			done = true;
		}
	} while(done == false);
}

//==========================================================================
//
// GetAssignPCD
//
//==========================================================================

static pcd_t GetAssignPCD(tokenType_t token, int symbol)
{
	size_t i, j;
	static tokenType_t tokenLookup[11] =
	{
		TK_ASSIGN, TK_ADDASSIGN, TK_SUBASSIGN,
		TK_MULASSIGN, TK_DIVASSIGN, TK_MODASSIGN,
		TK_ANDASSIGN, TK_EORASSIGN, TK_ORASSIGN,
		TK_LSASSIGN, TK_RSASSIGN
	};
	static int symbolLookup[] =
	{
		SY_SCRIPTVAR, SY_MAPVAR, SY_WORLDVAR, SY_GLOBALVAR, SY_MAPARRAY,
		SY_WORLDARRAY, SY_GLOBALARRAY
	};
	static pcd_t assignmentLookup[11][7] =
	{
		{ PCD_ASSIGNSCRIPTVAR, PCD_ASSIGNMAPVAR, PCD_ASSIGNWORLDVAR, PCD_ASSIGNGLOBALVAR, PCD_ASSIGNMAPARRAY, PCD_ASSIGNWORLDARRAY, PCD_ASSIGNGLOBALARRAY },
		{ PCD_ADDSCRIPTVAR, PCD_ADDMAPVAR, PCD_ADDWORLDVAR, PCD_ADDGLOBALVAR, PCD_ADDMAPARRAY, PCD_ADDWORLDARRAY, PCD_ADDGLOBALARRAY },
		{ PCD_SUBSCRIPTVAR, PCD_SUBMAPVAR, PCD_SUBWORLDVAR, PCD_SUBGLOBALVAR, PCD_SUBMAPARRAY, PCD_SUBWORLDARRAY, PCD_SUBGLOBALARRAY },
		{ PCD_MULSCRIPTVAR, PCD_MULMAPVAR, PCD_MULWORLDVAR, PCD_MULGLOBALVAR, PCD_MULMAPARRAY, PCD_MULWORLDARRAY, PCD_MULGLOBALARRAY },
		{ PCD_DIVSCRIPTVAR, PCD_DIVMAPVAR, PCD_DIVWORLDVAR, PCD_DIVGLOBALVAR, PCD_DIVMAPARRAY, PCD_DIVWORLDARRAY, PCD_DIVGLOBALARRAY },
		{ PCD_MODSCRIPTVAR, PCD_MODMAPVAR, PCD_MODWORLDVAR, PCD_MODGLOBALVAR, PCD_MODMAPARRAY, PCD_MODWORLDARRAY, PCD_MODGLOBALARRAY },
		{ PCD_ANDSCRIPTVAR, PCD_ANDMAPVAR, PCD_ANDWORLDVAR, PCD_ANDGLOBALVAR, PCD_ANDMAPARRAY, PCD_ANDWORLDARRAY, PCD_ANDGLOBALARRAY },
		{ PCD_EORSCRIPTVAR, PCD_EORMAPVAR, PCD_EORWORLDVAR, PCD_EORGLOBALVAR, PCD_EORMAPARRAY, PCD_EORWORLDARRAY, PCD_EORGLOBALARRAY },
		{ PCD_ORSCRIPTVAR, PCD_ORMAPVAR, PCD_ORWORLDVAR, PCD_ORGLOBALVAR, PCD_ORMAPARRAY, PCD_ORWORLDARRAY, PCD_ORGLOBALARRAY },
		{ PCD_LSSCRIPTVAR, PCD_LSMAPVAR, PCD_LSWORLDVAR, PCD_LSGLOBALVAR, PCD_LSMAPARRAY, PCD_LSWORLDARRAY, PCD_LSGLOBALARRAY },
		{ PCD_RSSCRIPTVAR, PCD_RSMAPVAR, PCD_RSWORLDVAR, PCD_RSGLOBALVAR, PCD_RSMAPARRAY, PCD_RSWORLDARRAY, PCD_RSGLOBALARRAY }
	};

	for(i = 0; i < 11; ++i)
	{
		if(tokenLookup[i] == token)
		{
			for (j = 0; j < 7; ++j)
			{
				if (symbolLookup[j] == symbol)
				{
					return assignmentLookup[i][j];
				}
			}
			break;
		}
	}
	return PCD_NOP;
}

//==========================================================================
//
// LeadingSuspend
//
//==========================================================================

static void LeadingSuspend()
{
	MS_Message(MSG_DEBUG, "---- LeadingSuspend ----\n");
	if(InsideFunction)
	{
		ERR_Error(ERR_SUSPEND_IN_FUNCTION, true);
	}
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_SUSPEND);
	TK_NextToken();
}

//==========================================================================
//
// LeadingTerminate
//
//==========================================================================

static void LeadingTerminate()
{
	MS_Message(MSG_DEBUG, "---- LeadingTerminate ----\n");
	if(InsideFunction)
	{
		ERR_Error(ERR_TERMINATE_IN_FUNCTION, true);
	}
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_TERMINATE);
	TK_NextToken();
}

//==========================================================================
//
// LeadingRestart
//
//==========================================================================

static void LeadingRestart()
{
	MS_Message(MSG_DEBUG, "---- LeadingRestart ----\n");
	if(InsideFunction)
	{
		ERR_Error(ERR_RESTART_IN_FUNCTION, true);
	}
	TK_NextTokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
	PC_AppendCmd(PCD_RESTART);
	TK_NextToken();
}

//==========================================================================
//
// LeadingReturn
//
//==========================================================================

static void LeadingReturn()
{
	MS_Message(MSG_DEBUG, "---- LeadingReturn ----\n");
	if(!InsideFunction)
	{
		ERR_Error(ERR_RETURN_OUTSIDE_FUNCTION, true);
		while (TK_NextToken () != TK_SEMICOLON)
		{
			if (tk_Token == TK_EOF)
				break;
		}
	}
	else
	{
		TK_NextToken();
		if(tk_Token == TK_SEMICOLON)
		{
			if(InsideFunction->info.scriptFunc.hasReturnValue)
			{
				ERR_Error(ERR_MUST_RETURN_A_VALUE, true);
			}
			PC_AppendCmd(PCD_RETURNVOID);
		}
		else
		{
			if(!InsideFunction->info.scriptFunc.hasReturnValue)
			{
				ERR_Error(ERR_MUST_NOT_RETURN_A_VALUE, true);
			}
			EvalExpression();
			TK_TokenMustBe(TK_SEMICOLON, ERR_MISSING_SEMICOLON);
			PC_AppendCmd(PCD_RETURNVAL);
		}
		TK_NextToken();
	}
}

//==========================================================================
//
// EvalConstExpression
//
//==========================================================================

static int EvalConstExpression()
{
	pa_ConstExprIsString = false;	// Used by PC_PutMapVariable
	ExprStackIndex = 0;
	ConstantExpression = true;
	ExprLevA();
	if(ExprStackIndex != 1)
	{
		ERR_Error(ERR_BAD_CONST_EXPR, true, NULL);
		ExprStack[0] = 0;
		ExprStackIndex = 1;
	}
	return PopExStk();
}

//==========================================================================
//
// EvalExpression
//
// [RH] Rewrote all the ExprLevA - ExprLevJ functions in favor of a single
// table-driven parser function.
//
//==========================================================================

static void EvalExpression()
{
	ConstantExpression = false;
	ExprLevA();
}

static void ExprLevA()
{
	ExprLevX(0);
}

// Operator precedence levels:
// Operator: ||
// Operator: &&
// Operator: |
// Operator: ^
// Operator: &
// Operators: == !=
// Operators: < <= > >=
// Operators: << >>
// Operators: + -
// Operators: * / %

static void ExprLevX(int level)
{
	if(OpsList[level] == NULL)
	{
		bool unaryMinus;

		unaryMinus = false;
		if(tk_Token == TK_MINUS)
		{
			unaryMinus = true;
			TK_NextToken();
		}
		if(tk_Token == TK_PLUS)
		{
			// Completely ignore unary plus
			TK_NextToken();
		}
		if(ConstantExpression == true)
		{
			ConstExprFactor();
		}
		else
		{
			ExprFactor();
		}
		if(unaryMinus == true)
		{
			SendExprCommand(PCD_UNARYMINUS);
		}
	}
	else
	{
		ExprLevX(level + 1);
		while (TK_Member((int*)OpsList[level]))
		{
			tokenType_t token = tk_Token;
			TK_NextToken();
			ExprLevX(level + 1);
			SendExprCommand(TokenToPCD(token));
		}
	}
}

static void ExprLineSpecial()
{
	int argCountMin = tk_SpecialArgCount & 0xffff;
	int argCountMax = tk_SpecialArgCount >> 16;
	int specialValue = tk_SpecialValue;

	// There are two ways to use a special in an expression:
	// 1. The special name by itself returns the special's number.
	// 2. The special followed by parameters actually executes the special.
	TK_NextToken();
	if(tk_Token != TK_LPAREN)
	{
		PC_AppendPushVal(specialValue);
	}
	else
	{
		int argCount = 0;

		TK_NextToken();
		if(tk_Token != TK_RPAREN)
		{
			TK_Undo();
			do
			{
				TK_NextToken();
				EvalExpression();
				argCount++;
			} while(tk_Token == TK_COMMA);
		}
		if(argCount < argCountMin || argCount > argCountMax)
		{
			ERR_Error(specialValue >=0? ERR_BAD_LSPEC_ARG_COUNT : ERR_BAD_ARG_COUNT, true);
			return;
		}
		if (specialValue >= 0)
		{
			for(; argCount < 5; ++argCount)
			{
				PC_AppendPushVal(0);
			}
			TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
			TK_NextToken();
			PC_AppendCmd(PCD_LSPEC5RESULT);
			if(pc_NoShrink)
			{
				PC_AppendInt(specialValue);
			}
			else
			{
				PC_AppendByte((byte)specialValue);
			}
		}
		else
		{
			TK_TokenMustBe(TK_RPAREN, ERR_MISSING_RPAREN);
			TK_NextToken();
			PC_AppendCmd(PCD_CALLFUNC);
			if(pc_NoShrink)
			{
				PC_AppendInt(argCount);
				PC_AppendInt(-specialValue);
			}
			else
			{
				PC_AppendByte((byte)argCount);
				PC_AppendWord((short)-specialValue);
			}
		}
	}
}

static void ExprFactor()
{
	ACS_Node *sym;
	tokenType_t opToken;

	switch(tk_Token)
	{
	case TK_STRING:
		if (ImportMode != IMPORT_Importing)
		{
			tk_Number = STR_Find(tk_String);
			PC_AppendPushVal(tk_Number);
			if (ImportMode == IMPORT_Exporting)
			{
				// The VM identifies strings by storing a library ID in the
				// high word of the string index. The library ID is not
				// known until the script actually gets loaded into the game,
				// so we need to use this p-code to tack the ID of the
				// currently running library to the index of the string that
				// just got pushed onto the stack.
				//
				// Simply assuming that a string is from the current library
				// is not good enough, because they can be passed around
				// between libraries and the map's behavior. Thus, we need
				// to know which object file a particular string belongs to.
				//
				// A specific example:
				// A map's behavior calls a function in a library and passes
				// a string:
				//
				//    LibFunc ("The library will do something with this string.");
				//
				// The library function needs to know that the string originated
				// outside the library. Similarly, if a library function returns
				// a string, the caller needs to know that the string did not
				// originate from the same object file.
				//
				// And that's why strings have library IDs tacked onto them.
				// The map's main behavior (i.e. an object that is not a library)
				// always uses library ID 0 to identify its strings, so its
				// strings don't need to be tagged.
				PC_AppendCmd(PCD_TAGSTRING);
			}
		}
		TK_NextToken();
		break;
	case TK_NUMBER:
		PC_AppendPushVal(tk_Number);
		TK_NextToken();
		break;
	case TK_LPAREN:
		TK_NextToken();
		ExprLevA();
		if(tk_Token != TK_RPAREN)
		{
			ERR_Error(ERR_BAD_EXPR, true, NULL);
			TK_SkipPast(TK_RPAREN);
		}
		else
		{
			TK_NextToken();
		}
		break;
	case TK_NOT:
		TK_NextToken();
		ExprFactor();
		PC_AppendCmd(PCD_NEGATELOGICAL);
		break;
	case TK_TILDE:
		TK_NextToken();
		ExprFactor();
		PC_AppendCmd(PCD_NEGATEBINARY);
		break;
	case TK_INC:
	case TK_DEC:
		opToken = tk_Token;
		TK_NextTokenMustBe(TK_IDENTIFIER, ERR_INCDEC_OP_ON_NON_VAR);
		sym = DemandSymbol(tk_String);
		if(sym->type != SY_SCRIPTVAR && sym->type != SY_MAPVAR
			&& sym->type != SY_WORLDVAR && sym->type != SY_GLOBALVAR
			&& sym->type != SY_MAPARRAY && sym->type != SY_WORLDARRAY
			&& sym->type != SY_GLOBALARRAY)
		{
			ERR_Error(ERR_INCDEC_OP_ON_NON_VAR, true);
		}
		else
		{
			TK_NextToken();
			if(sym->type == SY_MAPARRAY || sym->type == SY_WORLDARRAY
				|| sym->type == SY_GLOBALARRAY)
			{
				ParseArrayIndices(sym, sym->info.array.ndim);
				PC_AppendCmd(PCD_DUP);
			}
			else if(tk_Token == TK_LBRACKET)
			{
				ERR_Error(ERR_NOT_AN_ARRAY, true, sym->name);
				while(tk_Token == TK_LBRACKET)
				{
					TK_SkipPast(TK_RBRACKET);
				}
			}
			PC_AppendCmd(GetIncDecPCD(opToken, sym->type));
			PC_AppendShrink(sym->info.var.index);
			PC_AppendCmd(GetPushVarPCD(sym->type));
			PC_AppendShrink(sym->info.var.index);
		}
		break;
	case TK_IDENTIFIER:
		sym = SpeculateSymbol(tk_String, true);
		switch(sym->type)
		{
			case SY_SCRIPTALIAS:
				// FIXME
				break;
			case SY_MAPARRAY:
			case SY_WORLDARRAY:
			case SY_GLOBALARRAY:
				TK_NextToken();
				ParseArrayIndices(sym, sym->info.array.ndim);
				// fallthrough
			case SY_SCRIPTVAR:
			case SY_MAPVAR:
			case SY_WORLDVAR:
			case SY_GLOBALVAR:
				if(sym->type != SY_MAPARRAY && sym->type != SY_WORLDARRAY
					&& sym->type != SY_GLOBALARRAY)
				{
					TK_NextToken();
					if(tk_Token == TK_LBRACKET)
					{
						ERR_Error(ERR_NOT_AN_ARRAY, true, sym->name);
						while(tk_Token == TK_LBRACKET)
						{
							TK_SkipPast(TK_RBRACKET);
						}
					}
				}
				if((tk_Token == TK_INC || tk_Token == TK_DEC)
					&& (sym->type == SY_MAPARRAY || sym->type == SY_WORLDARRAY
						|| sym->type == SY_GLOBALARRAY))
				{
					PC_AppendCmd(PCD_DUP);
				}
				PC_AppendCmd(GetPushVarPCD(sym->type));
				PC_AppendShrink(sym->info.var.index);
				if(tk_Token == TK_INC || tk_Token == TK_DEC)
				{
					if(sym->type == SY_MAPARRAY || sym->type == SY_WORLDARRAY
						|| sym->type == SY_GLOBALARRAY)
					{
						PC_AppendCmd(PCD_SWAP);
					}
					PC_AppendCmd(GetIncDecPCD(tk_Token, sym->type));
					PC_AppendShrink(sym->info.var.index);
					TK_NextToken();
				}
				break;
			case SY_INTERNFUNC:
				if(sym->info.internFunc.hasReturnValue == false)
				{
					ERR_Error(ERR_EXPR_FUNC_NO_RET_VAL, true);
				}
				ProcessInternFunc(sym);
				break;
			case SY_SCRIPTFUNC:
				if(sym->info.scriptFunc.hasReturnValue == false)
				{
					ERR_Error(ERR_EXPR_FUNC_NO_RET_VAL, true);
				}
				ProcessScriptFunc(sym, false);
				break;
			default:
				ERR_Error(ERR_ILLEGAL_EXPR_IDENT, true, tk_String);
				TK_NextToken();
				break;
		}
		break;
	case TK_LINESPECIAL:
		ExprLineSpecial();
		break;
	case TK_STRPARAM_EVAL:
		LeadingPrint();
		TK_NextToken();
		break;
	case TK_STRCPY:
		LeadingStrcpy();
		TK_NextToken();
		break;
	default:
		ERR_Error(ERR_BAD_EXPR, true);
		TK_NextToken();
		break;
	}
}

static void ConstExprFactor()
{
	switch(tk_Token)
	{
	case TK_STRING:
		if (ImportMode != IMPORT_Importing)
		{
			int strnum = STR_Find(tk_String);
			if (ImportMode == IMPORT_Exporting)
			{
				pa_ConstExprIsString = true;
			}
			PushExStk(strnum);
		}
		else
		{
			// Importing, so it doesn't matter
			PushExStk(0);
		}
		TK_NextToken();
		break;
	case TK_NUMBER:
		PushExStk(tk_Number);
		TK_NextToken();
		break;
	case TK_LPAREN:
		TK_NextToken();
		ExprLevA();
		if(tk_Token != TK_RPAREN)
		{
			ERR_Error(ERR_BAD_CONST_EXPR, true);
			TK_SkipPast(TK_RPAREN);
		}
		else
		{
			TK_NextToken();
		}
		break;
	case TK_NOT:
		TK_NextToken();
		ConstExprFactor();
		SendExprCommand(PCD_NEGATELOGICAL);
		break;
	case TK_TILDE:
		TK_NextToken();
		ConstExprFactor();
		SendExprCommand(PCD_NEGATEBINARY);
		break;
	default:
		ERR_Error(ERR_BAD_CONST_EXPR, true);
		PushExStk(0);
		while(tk_Token != TK_COMMA &&
			tk_Token != TK_SEMICOLON &&
			tk_Token != TK_RPAREN)
		{
			if(tk_Token == TK_EOF)
			{
				ERR_Exit(ERR_EOF, true);
			}
			TK_NextToken();
		}
		break;
	}
}

//==========================================================================
//
// SendExprCommand
//
//==========================================================================

static void SendExprCommand(pcd_t pcd)
{
	int operand1, operand2;

	if(ConstantExpression == false)
	{
		PC_AppendCmd(pcd);
		return;
	}
	switch(pcd)
	{
		case PCD_ADD:
			PushExStk(PopExStk()+PopExStk());
			break;
		case PCD_SUBTRACT:
			operand2 = PopExStk();
			PushExStk(PopExStk()-operand2);
			break;
		case PCD_MULTIPLY:
			PushExStk(PopExStk()*PopExStk());
			break;
		case PCD_DIVIDE:
			operand2 = PopExStk();
			PushExStk(PopExStk()/operand2);
			break;
		case PCD_MODULUS:
			operand2 = PopExStk();
			PushExStk(PopExStk()%operand2);
			break;
		case PCD_EQ:
			PushExStk(PopExStk() == PopExStk());
			break;
		case PCD_NE:
			PushExStk(PopExStk() != PopExStk());
			break;
		case PCD_LT:
			operand2 = PopExStk();
			PushExStk(PopExStk() < operand2);
			break;
		case PCD_GT:
			operand2 = PopExStk();
			PushExStk(PopExStk() > operand2);
			break;
		case PCD_LE:
			operand2 = PopExStk();
			PushExStk(PopExStk() <= operand2);
			break;
		case PCD_GE:
			operand2 = PopExStk();
			PushExStk(PopExStk() >= operand2);
			break;
		case PCD_ANDLOGICAL:
			operand2 = PopExStk();
			operand1 = PopExStk();
			PushExStk(operand1 && operand2);
			break;
		case PCD_ORLOGICAL:
			operand2 = PopExStk();
			operand1 = PopExStk();
			PushExStk(operand1 || operand2);
			break;
		case PCD_ANDBITWISE:
			PushExStk(PopExStk()&PopExStk());
			break;
		case PCD_ORBITWISE:
			PushExStk(PopExStk()|PopExStk());
			break;
		case PCD_EORBITWISE:
			PushExStk(PopExStk()^PopExStk());
			break;
		case PCD_NEGATELOGICAL:
			PushExStk(!PopExStk());
			break;
		case PCD_NEGATEBINARY:
			PushExStk(~PopExStk());
			break;
		case PCD_LSHIFT:
			operand2 = PopExStk();
			PushExStk(PopExStk()<<operand2);
			break;
		case PCD_RSHIFT:
			operand2 = PopExStk();
			PushExStk(PopExStk()>>operand2);
			break;
		case PCD_UNARYMINUS:
			PushExStk(-PopExStk());
			break;
		default:
			ERR_Exit(ERR_UNKNOWN_CONST_EXPR_PCD, true);
			break;
	}
}

//==========================================================================
//
// PushExStk
//
//==========================================================================

static void PushExStk(int value)
{
	if(ExprStackIndex == EXPR_STACK_DEPTH)
	{
		ERR_Exit(ERR_EXPR_STACK_OVERFLOW, true);
	}
	ExprStack[ExprStackIndex++] = value;
}

//==========================================================================
//
// PopExStk
//
//==========================================================================

static int PopExStk()
{
	if(ExprStackIndex < 1)
	{
		ERR_Error(ERR_EXPR_STACK_EMPTY, true);
		return 0;
	}
	return ExprStack[--ExprStackIndex];
}

//==========================================================================
//
// TokenToPCD
//
//==========================================================================

static pcd_t TokenToPCD(tokenType_t token)
{
	int i;
	static struct
	{
		tokenType_t token;
		pcd_t pcd;
	}  operatorLookup[] =
	{
		{ TK_ORLOGICAL,		PCD_ORLOGICAL },
		{ TK_ANDLOGICAL,	PCD_ANDLOGICAL },
		{ TK_ORBITWISE,		PCD_ORBITWISE },
		{ TK_EORBITWISE,	PCD_EORBITWISE },
		{ TK_ANDBITWISE,	PCD_ANDBITWISE },
		{ TK_EQ,			PCD_EQ },
		{ TK_NE,			PCD_NE },
		{ TK_LT,			PCD_LT },
		{ TK_LE,			PCD_LE },
		{ TK_GT,			PCD_GT },
		{ TK_GE,			PCD_GE },
		{ TK_LSHIFT,		PCD_LSHIFT },
		{ TK_RSHIFT,		PCD_RSHIFT },
		{ TK_PLUS,			PCD_ADD },
		{ TK_MINUS,			PCD_SUBTRACT },
		{ TK_ASTERISK,		PCD_MULTIPLY },
		{ TK_SLASH,			PCD_DIVIDE },
		{ TK_PERCENT,		PCD_MODULUS },
		{ TK_NONE,			PCD_NOP }
	};

	for(i = 0; operatorLookup[i].token != TK_NONE; i++)
	{
		if(operatorLookup[i].token == token)
		{
			return operatorLookup[i].pcd;
		}
	}
	return PCD_NOP;
}

//==========================================================================
//
// GetPushVarPCD
//
//==========================================================================

static pcd_t GetPushVarPCD(int symType)
{
	switch(symType)
	{
		case SY_SCRIPTVAR:
			return PCD_PUSHSCRIPTVAR;
		case SY_MAPVAR:
			return PCD_PUSHMAPVAR;
		case SY_WORLDVAR:
			return PCD_PUSHWORLDVAR;
		case SY_GLOBALVAR:
			return PCD_PUSHGLOBALVAR;
		case SY_MAPARRAY:
			return PCD_PUSHMAPARRAY;
		case SY_WORLDARRAY:
			return PCD_PUSHWORLDARRAY;
		case SY_GLOBALARRAY:
			return PCD_PUSHGLOBALARRAY;
		default:
			break;
	}
	return PCD_NOP;
}

//==========================================================================
//
// GetIncDecPCD
//
//==========================================================================

static pcd_t GetIncDecPCD(tokenType_t token, int symbol)
{
	int i;
	static struct
	{
		tokenType_t token;
		int symbol;
		pcd_t pcd;
	}  incDecLookup[] =
	{
		{ TK_INC, SY_SCRIPTVAR, PCD_INCSCRIPTVAR },
		{ TK_INC, SY_MAPVAR, PCD_INCMAPVAR },
		{ TK_INC, SY_WORLDVAR, PCD_INCWORLDVAR },
		{ TK_INC, SY_GLOBALVAR, PCD_INCGLOBALVAR },
		{ TK_INC, SY_MAPARRAY, PCD_INCMAPARRAY },
		{ TK_INC, SY_WORLDARRAY, PCD_INCWORLDARRAY },
		{ TK_INC, SY_GLOBALARRAY, PCD_INCGLOBALARRAY },

		{ TK_DEC, SY_SCRIPTVAR, PCD_DECSCRIPTVAR },
		{ TK_DEC, SY_MAPVAR, PCD_DECMAPVAR },
		{ TK_DEC, SY_WORLDVAR, PCD_DECWORLDVAR },
		{ TK_DEC, SY_GLOBALVAR, PCD_DECGLOBALVAR },
		{ TK_DEC, SY_MAPARRAY, PCD_DECMAPARRAY },
		{ TK_DEC, SY_WORLDARRAY, PCD_DECWORLDARRAY },
		{ TK_DEC, SY_GLOBALARRAY, PCD_DECGLOBALARRAY },

		{ TK_NONE, SY_DUMMY, PCD_NOP }
	};

	for(i = 0; incDecLookup[i].token != TK_NONE; i++)
	{
		if(incDecLookup[i].token == token
			&& incDecLookup[i].symbol == symbol)
		{
			return incDecLookup[i].pcd;
		}
	}
	return PCD_NOP;
}

//==========================================================================
//
// ParseArrayIndices
//
//==========================================================================

static void ParseArrayIndices(ACS_Node *sym, int requiredIndices)
{
	bool warned = false;
	int i;

	if(requiredIndices > 0)
	{
		TK_TokenMustBe(TK_LBRACKET, ERR_MISSING_LBRACKET);
	}
	i = 0;
	while(tk_Token == TK_LBRACKET)
	{
		TK_NextToken();
		if((sym->type == SY_MAPARRAY && i == requiredIndices) ||
			(sym->type != SY_MAPARRAY && i > 0))
		{
			if (!warned)
			{
				warned = true;
				if(sym->info.array.ndim == requiredIndices)
				{
					ERR_Error(ERR_TOO_MANY_DIM_USED, true,
						sym->name, sym->info.array.ndim);
				}
				else
				{
					ERR_Error(ERR_NOT_A_CHAR_ARRAY, true, sym->name,
						sym->info.array.ndim, requiredIndices);
				}
			}
		}
		EvalExpression();
		if(i < sym->info.array.ndim - 1 && sym->info.array.dimensions[i] > 1)
		{
			PC_AppendPushVal(sym->info.array.dimensions[i]);
			PC_AppendCmd(PCD_MULTIPLY);
		}
		if(i > 0)
		{
			PC_AppendCmd(PCD_ADD);
		}
		i++;
		TK_TokenMustBe(TK_RBRACKET, ERR_MISSING_RBRACKET);
		TK_NextToken();
	}
	if(i < requiredIndices)
	{
		ERR_Error(ERR_TOO_FEW_DIM_USED, true,
			sym->name, requiredIndices - i);
	}
	// if there were unspecified indices, multiply the offset by their sizes [JB]
	if(requiredIndices < sym->info.array.ndim - 1)
	{
		int i, mult = 1;
		for(i = 0; i < sym->info.array.ndim - requiredIndices - 1; ++i)
		{
			mult *= sym->info.array.dimensions[sym->info.array.ndim - 2 - i];
		}
		if(mult > 1)
		{
			PC_AppendPushVal(mult);
			PC_AppendCmd(PCD_MULTIPLY);
		}
	}
}

static void ProcessArrayLevel(int level, int *entry, int ndim,
	int dims[MAX_ARRAY_DIMS], int muls[MAX_ARRAY_DIMS], char *name)
{
	int warned_too_many = false;
	int i;

	for(i = 0; ; ++i)
	{
		if(tk_Token == TK_COMMA)
		{
			entry += muls[level-1];
			TK_NextToken();
		}
		else if(tk_Token == TK_RBRACE)
		{
			TK_NextToken();
			return;
		}
		else
		{
			if(level == ndim)
			{
				if(tk_Token == TK_LBRACE)
				{
					ERR_Error(ERR_TOO_MANY_DIM_USED, true, name, ndim);
					SkipBraceBlock(0);
					TK_NextToken();
					entry++;
				}
				else
				{
					int val;

					if (i >= dims[level - 1] && !warned_too_many)
					{
						warned_too_many = true;
						ERR_Error(ERR_TOO_MANY_ARRAY_INIT, true);
					}
					val = EvalConstExpression();
					ArrayHasStrings |= pa_ConstExprIsString;
					if (i < dims[level - 1])
					{
						entry[i] = val;
					}
				}
			}
			else
			{
				//Bugfix for r3226 by Zom-B
				if (i >= dims[level - 1])
				{
					if (!warned_too_many)
					{
						warned_too_many = true;
						ERR_Error(ERR_TOO_MANY_ARRAY_INIT, true);
					}
					// Allow execution to continue without stray memory access
					entry -= muls[level-1];
				}
				TK_TokenMustBe(TK_LBRACE, ERR_MISSING_LBRACE_ARR);
				TK_NextToken();
				ProcessArrayLevel(level+1, entry, ndim, dims, muls, name);
				assert(level > 0);
				entry += muls[level-1];
			}
			if(i < dims[level-1]-1)
			{
				if(tk_Token != TK_RBRACE)
				{
					TK_TokenMustBe(TK_COMMA, ERR_MISSING_COMMA);
					TK_NextToken();
				}
			}
			else
			{
				if(tk_Token != TK_COMMA)
				{
					TK_TokenMustBe(TK_RBRACE, ERR_MISSING_RBRACE_ARR);
				}
				else
				{
					TK_NextToken();
				}
			}
		}
	}
	TK_TokenMustBe(TK_RBRACE, ERR_MISSING_RBRACE_ARR);
	TK_NextToken();
}

//==========================================================================
//
// InitializeArray
//
//==========================================================================

static void InitializeArray(ACS_Node *sym, int dims[MAX_ARRAY_DIMS], int size)
{
	static int *entries = NULL;
	static int lastsize = -1;

	if(lastsize < size)
	{
		entries = new int[size];
		lastsize = size;
	}
	memset(entries, 0, sizeof(int)*size);

	TK_NextTokenMustBe(TK_LBRACE, ERR_MISSING_LBRACE_ARR);
	TK_NextToken();
	ArrayHasStrings = false;
	ProcessArrayLevel(1, entries, sym->info.array.ndim, dims,
		sym->info.array.dimensions, sym->name);
	if(ImportMode != IMPORT_Importing)
	{
		PC_InitArray(sym->info.array.index, entries, ArrayHasStrings);
	}
}

//==========================================================================
//
// DemandSymbol
//
//==========================================================================

static ACS_Node *DemandSymbol(string name)
{
	ACS_Node *sym;

	if((sym = SY_Find(name)) == NULL)
	{
		ERR_Exit(ERR_UNKNOWN_IDENTIFIER, true, name);
	}
	return sym;
}

//==========================================================================
//
// SpeculateSymbol
//
//==========================================================================

static ACS_Node *SpeculateSymbol(string name, bool hasReturn)
{
	ACS_Node *sym;

	sym = SY_Find(name);
	if(sym == NULL)
	{
		string name = tk_String;

		TK_NextToken();
		if(tk_Token == TK_LPAREN)
		{ // Looks like a function call
			sym = SpeculateFunction(name, hasReturn);
			TK_Undo();
		}
		else
		{
			ERR_Exit(ERR_UNKNOWN_IDENTIFIER, true, name);
		}
	}
	return sym;
}

//==========================================================================
//
// SpeculateFunction
//
// Add a temporary symbol for a function that is used before it is defined.
//
//==========================================================================

static ACS_Node *SpeculateFunction(const string name, bool hasReturn)
{
	ACS_Node *sym;
	
	MS_Message(MSG_DEBUG, "---- SpeculateFunction %s ----\n", name);
	sym = SY_InsertGlobal(tk_String, SY_SCRIPTFUNC);
	sym->info.scriptFunc.predefined = true;
	sym->info.scriptFunc.hasReturnValue = hasReturn;
	sym->info.scriptFunc.sourceLine = tk_Line;
	sym->info.scriptFunc.sourceName = tk_SourceName;
	sym->info.scriptFunc.argCount = -1;
	sym->info.scriptFunc.funcNumber = 0;
	return sym;
}

//==========================================================================
//
// UnspeculateFunction
//
// Fills in function calls that were made before this function was defined.
//
//==========================================================================

static void UnspeculateFunction(ACS_Node *sym)
{
	prefunc_t *fillin;
	prefunc_t **prev;

	prev = &FillinFunctions;
	fillin = FillinFunctions;
	while(fillin != NULL)
	{
		prefunc_t *next = fillin->next;
		if(fillin->sym == sym)
		{
			if(fillin->argcount != sym->info.scriptFunc.argCount)
			{
				ERR_ErrorAt(fillin->source, fillin->line);
				ERR_Error(ERR_FUNC_ARGUMENT_COUNT, true, sym->name,
					sym->info.scriptFunc.argCount,
					sym->info.scriptFunc.argCount == 1 ? "" : "s");
			}

			if(pc_NoShrink)
			{
				PC_WriteInt(sym->info.scriptFunc.funcNumber, fillin->address);
			}
			else
			{
				PC_WriteByte((byte)sym->info.scriptFunc.funcNumber, fillin->address);
			}
			if(FillinFunctionsLatest == &fillin->next)
			{
				FillinFunctionsLatest = prev;
			}
			free (fillin);
			*prev = next;
		}
		else
		{
			prev = &fillin->next;
		}
		fillin = next;
	}
}

//==========================================================================
//
// AddScriptFuncRef
//
//==========================================================================

static void AddScriptFuncRef(ACS_Node *sym, int address, int argcount)
{
	prefunc_t *fillin = new prefunc_t;
	fillin->next = NULL;
	fillin->sym = sym;
	fillin->address = address;
	fillin->argcount = argcount;
	fillin->line = tk_Line;
	fillin->source = tk_SourceName;
	*FillinFunctionsLatest = fillin;
	FillinFunctionsLatest = &fillin->next;
}

//==========================================================================
//
// Check for undefined functions
//
//==========================================================================

static void CheckForUndefinedFunctions()
{
	prefunc_t *fillin = FillinFunctions;

	while(fillin)
	{
		ERR_ErrorAt(fillin->source, fillin->line);
		ERR_Error(ERR_UNDEFINED_FUNC, true, fillin->sym->name);
		fillin = fillin->next;
	}
}

//==========================================================================
//
// SkipBraceBlock
//
// If depth is 0, it scans for the first { and then starts going from there.
// At exit, the terminating } is left as the current token.
//
//==========================================================================

void SkipBraceBlock(int depth)
{
	if (depth == 0)
	{
		// Find first {
		while(tk_Token != TK_LBRACE)
		{
			if(tk_Token == TK_EOF)
			{
				ERR_Exit(ERR_EOF, false);
			}
			TK_NextToken();
		}
		depth = 1;
	}
	// Match it with a }
	do
	{
		TK_NextToken();
		if(tk_Token == TK_EOF)
		{
			ERR_Exit(ERR_EOF, false);
		}
		else if (tk_Token == TK_LBRACE)
		{
			depth++;
		}
		else if (tk_Token == TK_RBRACE)
		{
			depth--;
		}
	} while (depth > 0);
}
