
//**************************************************************************
//**
//** token.h
//**
//**************************************************************************

#pragma once

// HEADER FILES ------------------------------------------------------------

#include "common.h"
#include "error.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

enum tokenType_t : int
{
	TK_NONE,
	TK_EOF,
	TK_IDENTIFIER,		// VALUE: (char *) tk_String
	TK_STRING,			// VALUE: (char *) tk_String
	TK_NUMBER,			// VALUE: (int) tk_Number
	TK_LINESPECIAL,		// VALUE: (int) tk_LineSpecial
	TK_PLUS,			// '+'
	TK_MINUS,			// '-'
	TK_ASTERISK,		// '*'
	TK_SLASH,			// '/'
	TK_PERCENT,			// '%'
	TK_ASSIGN,			// '='
	TK_ADDASSIGN,		// '+='
	TK_SUBASSIGN,		// '-='
	TK_MULASSIGN,		// '*='
	TK_DIVASSIGN,		// '/='
	TK_MODASSIGN,		// '%='
	TK_INC,				// '++'
	TK_DEC,				// '--'
	TK_EQ,				// '=='
	TK_NE,				// '!='
	TK_LT,				// '<'
	TK_GT,				// '>'
	TK_LE,				// '<='
	TK_GE,				// '>='
	TK_LSHIFT,			// '<<'
	TK_RSHIFT,			// '>>'
	TK_ANDLOGICAL,		// '&&'
	TK_ORLOGICAL,		// '||'
	TK_ANDBITWISE,		// '&'
	TK_ORBITWISE,		// '|'
	TK_EORBITWISE,		// '^'
	TK_TILDE,			// '~'
	TK_LPAREN,			// '('
	TK_RPAREN,			// ')'
	TK_LBRACE,			// '{'
	TK_RBRACE,			// '}'
	TK_LBRACKET,		// '['
	TK_RBRACKET,		// ']'
	TK_COLON,			// ':'
	TK_SEMICOLON,		// ';'
	TK_COMMA,			// ','
	TK_PERIOD,			// '.'
	TK_NOT,				// '!'
	TK_NUMBERSIGN,		// '#'
	TK_CPPCOMMENT,		// '//'
	TK_STARTCOMMENT,	// '/*'
	TK_ENDCOMMENT,		// '*/'
	TK_BREAK,			// 'break'
	TK_CASE,			// 'case'
	TK_CONST,			// 'const'
	TK_CONTINUE,		// 'continue'
	TK_DEFAULT,			// 'default'
	TK_DEFINE,			// 'define'
	TK_DO,				// 'do'
	TK_ELSE,			// 'else'
	TK_FOR,				// 'for'
	TK_GOTO,			// 'goto'
	TK_IF,				// 'if'
	TK_INCLUDE,			// 'include'
	TK_INT,				// 'int'
	TK_OPEN,			// 'open'
	TK_PRINT,			// 'print'
	TK_PRINTBOLD,		// 'printbold'
	TK_LOG,				// 'log'
	TK_HUDMESSAGE,		// 'hudmessage'
	TK_HUDMESSAGEBOLD,	// 'hudmessagebold'
	TK_RESTART,			// 'restart'
	TK_SCRIPT,			// 'script'
	TK_SPECIAL,			// 'special'
	TK_STR,				// 'str'
	TK_SUSPEND,			// 'suspend'
	TK_SWITCH,			// 'switch'
	TK_TERMINATE,		// 'terminate'
	TK_UNTIL,			// 'until'
	TK_VOID,			// 'void'
	TK_WHILE,			// 'while'
	TK_WORLD,			// 'world'
	TK_GLOBAL,			// 'global'
	TK_RESPAWN,			// 'respawn'		[BC]
	TK_DEATH,			// 'death'			[BC]
	TK_ENTER,			// 'enter'			[BC]
	TK_PICKUP,			// 'pickup'			[BC]
	TK_BLUERETURN,		// 'bluereturn'		[BC]
	TK_REDRETURN,		// 'redreturn'		[BC]
	TK_WHITERETURN,		// 'whitereturn'	[BC]
	TK_NOCOMPACT,		// 'nocompact'
	TK_LIGHTNING,		// 'ligtning'
	TK_CREATETRANSLATION,// 'createtranslation'
	TK_FUNCTION,		// 'function'
	TK_RETURN,			// 'return'
	TK_WADAUTHOR,		// 'wadauthor'
	TK_NOWADAUTHOR,		// 'nowadauthor'
	TK_ACSEXECUTEWAIT,	// 'acs_executewait'
	TK_ACSNAMEDEXECUTEWAIT,// 'acs_namedexecutewait'
	TK_ENCRYPTSTRINGS,	// 'encryptstrings'
	TK_IMPORT,			// 'import'
	TK_LIBRARY,			// 'library'
	TK_LIBDEFINE,		// 'libdefine'
	TK_BOOL,			// 'bool'
	TK_NET,				// 'net'
	TK_CLIENTSIDE,		// 'clientside'
	TK_DISCONNECT,		// 'disconnect'
	TK_UNLOADING,		// 'unloading'
	TK_STATIC,			// 'static'
	TK_ANDASSIGN,		// '&='
	TK_ORASSIGN,		// '|='
	TK_EORASSIGN,		// '^='
	TK_LSASSIGN,		// '<<='
	TK_RSASSIGN,		// '>>='
	TK_STRPARAM_EVAL,	// 'strparam'
	TK_STRCPY,          // 'strcpy'
	// [JRT] New for ACC++
	//TK_TID,			// 'tid'
	//TK_FIXED,			// 'fixed'
	//TK_ACTOR,			// 'actor'
	TK_NEW,				// 'new'
	TK_STRUCT,			// 'struct'
	TK_METHOD,			// 'method'
	//TK_LOCAL,			// 'local'
	//TK_OVERRIDES,		// 'overrides'
	TK_CLASS,			// 'class'
	//TK_SOUND,			// 'sound'
	//TK_IMAGE,			// 'image'
	//TK_MUSIC,			// 'music'
	TK_DELETE,			// 'delete'
	TK_QSTART,			// '?'
	TK_TEMPLATE,		// 'template'
	TK_TYPEDEF,			// 'typedef'
	TK_PUBLIC,			// 'public'
	TK_PRIVATE,			// 'private'
	TK_POINTERTO,		// '->'
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void TK_Init();
void TK_OpenSource(string fileName);
void TK_Include(string fileName);
void TK_Import(string fileName, enum ImportModes prevMode);
void TK_CloseSource();
int TK_GetDepth();
tokenType_t TK_NextToken();
int TK_NextCharacter();
bool TK_NextTokenMustBe(int token, int error);
bool TK_TokenMustBe(int token, int error);
bool TK_Member(int *list);
void TK_Undo();
void TK_SkipLine();
void TK_SkipPast(int token);
void TK_SkipTo(int token);
void TK_AddIncludePath(string sourceName);
void TK_AddProgramIncludePath(string argv0);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

extern tokenType_t tk_Token;
extern int tk_Line;
extern int tk_Number;
extern string tk_String;
extern int tk_SpecialValue;
extern int tk_SpecialArgCount;
extern string tk_SourceName;
extern int tk_IncludedLines;
extern bool forSemicolonHack;
extern string MasterSourceLine;		// master line - Ty 07jan2000
extern int MasterSourcePos;			// master position - Ty 07jan2000
extern bool ClearMasterSourceLine;	// ready for new line - Ty 07jan2000
