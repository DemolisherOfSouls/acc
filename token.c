
//**************************************************************************
//**
//** token.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#if defined(_WIN32) && !defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __NeXT__
#include <libc.h>
#else
#ifndef unix
#include <io.h>
#endif
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include "common.h"
#include "token.h"
#include "error.h"
#include "misc.h"
#include "symbol.h"
#include "parse.h"

// MACROS ------------------------------------------------------------------

#define NON_HEX_DIGIT 255
#define MAX_NESTED_SOURCES 16
#define MAX_FILENAMES_SIZE	4096

// TYPES -------------------------------------------------------------------

enum chr_t : char
{
	CHR_EOF,
	CHR_LETTER,
	CHR_NUMBER,
	CHR_QUOTE,
	CHR_SPECIAL
};

struct nestInfo_t
{
	vector<char> data;
	string name;
	int size;
	int pos;
	int line;
	bool incLineNumber;
	bool imported;
	enum ImportModes prevMode;
	char lastChar;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int SortKeywords(const void *a, const void *b);
static void SetLocalIncludePath(string sourceName);
static int PopNestedSource(enum ImportModes *prevMode);
static void ProcessLetterToken();
static void ProcessNumberToken();
static void EvalFixedConstant(int whole);
static void EvalHexConstant();
static void EvalRadixConstant();
static int DigitValue(char digit, int radix);
static void ProcessQuoteToken();
static void ProcessSpecialToken();
static bool CheckForKeyword();
static bool CheckForLineSpecial();
static bool CheckForConstant();
static void NextChr();
static void SkipComment();
static void BumpMasterSourceLine(char Chr, bool clear); // master line - Ty 07jan2000
static char *AddFileName(const char *name);
static int OctalChar();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

tokenType_t tk_Token;
int tk_Line;
int tk_Number;
string tk_String;
int tk_SpecialValue;
int tk_SpecialArgCount;
string tk_SourceName;
int tk_IncludedLines;
bool forSemicolonHack;
string MasterSourceLine;  // master line - Ty 07jan2000
int MasterSourcePos;      // master position - Ty 07jan2000
int PrevMasterSourcePos;	// previous master position - RH 09feb2000
bool ClearMasterSourceLine;  // master clear flag - Ty 07jan2000

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char Chr;
static int Pos;
static vector<char> File;
static bool SourceOpen;
static char ASCIIToChrCode[256];
static char ASCIIToHexDigit[256];
static string TokenStringBuffer;
static nestInfo_t OpenFiles[MAX_NESTED_SOURCES];
static bool AlreadyGot;
static int NestDepth;
static bool IncLineNumber;
static vector<string> FileNames;
static size_t FileNamesLen;

// Pascal 12/11/08
// Include paths. Lowest is searched first.
// Include path 0 is always set to the directory of the file being parsed.
static vector<string> IncludePaths;

struct keyword_s
{
	char* name;
	tokenType_t token;
};

keyword_s Keywords[] =
{
	{ "break", TK_BREAK },
	{ "case", TK_CASE },
	{ "const", TK_CONST },
	{ "continue", TK_CONTINUE },
	{ "default", TK_DEFAULT },
	{ "define", TK_DEFINE },
	{ "do", TK_DO },
	{ "else", TK_ELSE },
	{ "for", TK_FOR },
	{ "goto", TK_GOTO },
	{ "if", TK_IF },
	{ "include", TK_INCLUDE },
	{ "int", TK_INT },
	{ "open", TK_OPEN },
	{ "print", TK_PRINT },
	{ "printbold", TK_PRINTBOLD },
	{ "log", TK_LOG },
	{ "hudmessage", TK_HUDMESSAGE },
	{ "hudmessagebold", TK_HUDMESSAGEBOLD },
	{ "restart", TK_RESTART },
	{ "script", TK_SCRIPT },
	{ "special", TK_SPECIAL },
	{ "str", TK_STR },
	{ "suspend", TK_SUSPEND },
	{ "switch", TK_SWITCH },
	{ "terminate", TK_TERMINATE },
	{ "until", TK_UNTIL },
	{ "void", TK_VOID },
	{ "while", TK_WHILE },
	{ "world", TK_WORLD },
	{ "global", TK_GLOBAL },
	// [BC] Start Skulltag tokens.
	{ "respawn", TK_RESPAWN },
	{ "death", TK_DEATH },
	{ "enter", TK_ENTER },
	{ "pickup", TK_PICKUP },
	{ "bluereturn", TK_BLUERETURN },
	{ "redreturn", TK_REDRETURN },
	{ "whitereturn", TK_WHITERETURN },
	// [BC] End Skulltag tokens.
	{ "nocompact", TK_NOCOMPACT },
	{ "lightning", TK_LIGHTNING },
	{ "createtranslation", TK_CREATETRANSLATION },
	{ "function", TK_FUNCTION },
	{ "return", TK_RETURN },
	{ "wadauthor", TK_WADAUTHOR },
	{ "nowadauthor", TK_NOWADAUTHOR },
	{ "acs_executewait", TK_ACSEXECUTEWAIT },
	{ "acs_namedexecutewait", TK_ACSNAMEDEXECUTEWAIT },
	{ "encryptstrings", TK_ENCRYPTSTRINGS },
	{ "import", TK_IMPORT },
	{ "library", TK_LIBRARY },
	{ "libdefine", TK_LIBDEFINE },
	{ "bool", TK_BOOL },
	{ "net", TK_NET },
	{ "clientside", TK_CLIENTSIDE }, // [BB]
	{ "disconnect", TK_DISCONNECT },
	{ "unloading", TK_UNLOADING },
	{ "static", TK_STATIC },
	{ "strparam", TK_STRPARAM_EVAL }, // [FDARI]
	{ "strcpy", TK_STRCPY }, // [FDARI]
	// [JRT] Start new tokens
	//{ "tid", TK_TID },
	//{ "fixed", TK_FIXED },
	//{ "actor", TK_ACTOR },
	//{ "sound", TK_SOUND },
	//{ "overrides", TK_OVERRIDES },
	//{ "image", TK_IMAGE },
	{ "new", TK_NEW },
	{ "struct", TK_STRUCT },
	{ "method", TK_METHOD },
	{ "class", TK_CLASS },
	{ "delete", TK_DELETE },
	{ "template", TK_TEMPLATE },
	{ "typedef", TK_TYPEDEF },
	{ "public", TK_PUBLIC },
	{ "private", TK_PRIVATE }
};

#define NUM_KEYWORDS (sizeof(Keywords)/sizeof(keyword_s))

// CODE --------------------------------------------------------------------

//==========================================================================
//
// TK_Init
//
//==========================================================================
void TK_Init()
{
	int i;
	
	for(i = 0; i < 256; i++)
	{
		ASCIIToChrCode[i] = CHR_SPECIAL;
		ASCIIToHexDigit[i] = NON_HEX_DIGIT;
	}
	for(i = '0'; i <= '9'; i++)
	{
		ASCIIToChrCode[i] = CHR_NUMBER;
		ASCIIToHexDigit[i] = i-'0';
	}
	for(i = 'A'; i <= 'F'; i++)
	{
		ASCIIToHexDigit[i] = 10+(i-'A');
	}
	for(i = 'a'; i <= 'f'; i++)
	{
		ASCIIToHexDigit[i] = 10+(i-'a');
	}
	for(i = 'A'; i <= 'Z'; i++)
	{
		ASCIIToChrCode[i] = CHR_LETTER;
	}
	for(i = 'a'; i <= 'z'; i++)
	{
		ASCIIToChrCode[i] = CHR_LETTER;
	}
	ASCIIToChrCode[ASCII_QUOTE] = CHR_QUOTE;
	ASCIIToChrCode[ASCII_UNDERSCORE] = CHR_LETTER;
	ASCIIToChrCode[EOF_CHARACTER] = CHR_EOF;
	tk_String = TokenStringBuffer;
	IncLineNumber = false;
	tk_IncludedLines = 0;
	IncludePaths = vector<string>(MAX_INCLUDE_PATHS);
	SourceOpen = false;
	MasterSourceLine = ""; // master line - Ty 07jan2000
	MasterSourcePos = 0;      // master position - Ty 07jan2000
	ClearMasterSourceLine = true; // clear the line to start
	qsort (Keywords, NUM_KEYWORDS, sizeof(keyword_s), SortKeywords);
	FileNames = vector<string>(MAX_INCLUDE_PATHS);
	FileNamesLen = 0;
	File = vector<char>(DEFAULT_OBJECT_SIZE);
}

//==========================================================================
//
// SortKeywords
//
//==========================================================================
static int SortKeywords(const void *a, const void *b)
{
	return strcmp (((struct keyword_s *)a)->name,
		((struct keyword_s *)b)->name);
}

//==========================================================================
//
// TK_OpenSource
//
//==========================================================================
void TK_OpenSource(string fileName)
{
	TK_CloseSource();
	MS_LoadFile(fileName, File);
	tk_SourceName = AddFileName(fileName);
	SetLocalIncludePath(fileName);
	SourceOpen = true;
	tk_Line = 1;
	tk_Token = TK_NONE;
	AlreadyGot = false;
	NestDepth = 0;
	NextChr();
}

//==========================================================================
//
// AddFileName
//
//==========================================================================
static int AddFileName(string name)
{
	FileNames.add(name);
	return FileNames.lastIndex();
}

//==========================================================================
//
// TK_AddIncludePath
// This adds an include path with less priority than the ones already added
// 
// Pascal 12/11/08
//
//==========================================================================
void TK_AddIncludePath(string sourcePath)
{
	if (IncludePaths.size() < MAX_INCLUDE_PATHS)
	{
		// Not ending with directory delimiter?
		if (!MS_IsDirectoryDelimiter(sourcePath.back()))
		{
			// Add a directory delimiter to the include path
			sourcePath.append("/");
		}

		// Add to list
		IncludePaths.add(sourcePath);

		MS_Message(MSG_DEBUG, "Add include path ", IncludePaths.size(), ": \"", sourcePath, "\"");
	}
}

//==========================================================================
//
// TK_AddProgramIncludePath
// Adds an include path for the directory of the executable.
//
//==========================================================================
void TK_AddProgramIncludePath(char* progname)
{
	if(IncludePaths.size() < MAX_INCLUDE_PATHS)
	{
#ifdef _WIN32
#ifdef _MSC_VER
#if _MSC_VER >= 1300
		if (_get_pgmptr(&progname) != 0)
		{
			return;
		}
		string pn = progname;
#else
		progname = _pgmptr;
#endif
#else
		char progbuff[1024];
		GetModuleFileName(0, progbuff, sizeof(progbuff));
		progbuff[sizeof(progbuff)-1] = '\0';
		progname = progbuff;
#endif
#else
		char progbuff[PATH_MAX];
		if (realpath(progname, progbuff) != NULL)
		{
			progname = progbuff;
		}
#endif
		
		if(MS_StripFilename(pn))
		{
			IncludePaths.add(pn);
			MS_Message(MSG_DEBUG, "Program include path is ", IncludePaths.size(), ": \"", pn, "\"");
		}
		else {
			IncludePaths.add(pn);
		}
		
	}
}

//==========================================================================
//
// SetLocalIncludePath
// This sets the first include path
// 
// Pascal 12/11/08
//
//==========================================================================
static void SetLocalIncludePath(string sourceName)
{
	IncludePaths.prefer(0, sourceName);

	if (MS_StripFilename(IncludePaths.lastAdded()) == false)
	{
		IncludePaths.lastAdded = "";
	}
}


//==========================================================================
//
// TK_Include
//
//==========================================================================

void TK_Include(string fileName)
{
	string sourceName;
	nestInfo_t *info;
	bool foundfile = false;
	
	MS_Message(MSG_DEBUG, "*Including %s\n", fileName);
	if(NestDepth == MAX_NESTED_SOURCES)
	{
		ERR_Exit(ERR_INCL_NESTING_TOO_DEEP, true, fileName);
	}
	info = &OpenFiles[NestDepth++];
	info->name = tk_SourceName;
	info->size = File.size;
	info->pos = FilePtr;
	info->line = tk_Line;
	info->incLineNumber = IncLineNumber;
	info->lastChar = Chr;
	info->imported = false;
	
	// Pascal 30/11/08
	// Handle absolute paths
	if(MS_IsPathAbsolute(fileName))
	{
#if defined(_WIN32) || defined(__MSDOS__)
		sourceName[0] = '\0';
		if(MS_IsDirectoryDelimiter(fileName[0]))
		{
			// The source file is absolute for the drive, but does not
			// specify a drive. Use the path for the current file to
			// get the drive letter, if it has one.
			if(IncludePaths[0][0] != '\0' && IncludePaths[0][1] == ':')
			{
				sourceName[0] = IncludePaths[0][0];
				sourceName[1] = ':';
				sourceName[2] = '\0';
			}
		}
		sourceName += fileName;
#else
		strcpy(sourceName, fileName);
#endif
		foundfile = MS_FileExists(sourceName);
	}
	else
	{
		// Pascal 12/11/08
		// Find the file in the include paths
		for(int i = 0; i < IncludePaths.size; i++)
		{
			sourceName = IncludePaths[i];
			sourceName += fileName;
			if(MS_FileExists(sourceName))
			{
				foundfile = true;
				break;
			}
		}
	}
	
	if(!foundfile)
	{
		ERR_ErrorAt(tk_SourceName, tk_Line);
		ERR_Exit(ERR_CANT_FIND_INCLUDE, true, fileName, tk_SourceName, tk_Line);
	}

	MS_Message(MSG_DEBUG, "*Include file found at %s\n", sourceName);

	// Now change the first include path to the file directory
	SetLocalIncludePath(sourceName);
	
	tk_SourceName = AddFileName(sourceName);
	MS_LoadFile(tk_SourceName, &File);
	FilePtr = FileStart;
	tk_Line = 1;
	IncLineNumber = false;
	tk_Token = TK_NONE;
	AlreadyGot = false;
	BumpMasterSourceLine('x',true); // dummy x
	NextChr();
}

//==========================================================================
//
// TK_Import
//
//==========================================================================

void TK_Import(char *fileName, enum ImportModes prevMode)
{
	TK_Include (fileName);
	OpenFiles[NestDepth - 1].imported = true;
	OpenFiles[NestDepth - 1].prevMode = prevMode;
	ImportMode = IMPORT_Importing;
}

//==========================================================================
//
// PopNestedSource
//
//==========================================================================

static int PopNestedSource(enum ImportModes *prevMode)
{
	nestInfo_t *info;
	//TODO:Finsh this
	MS_Message(MSG_DEBUG, "*Leaving %s\n", tk_SourceName);
	SY_FreeConstants(NestDepth);
	tk_IncludedLines += tk_Line;
	info = &OpenFiles[--NestDepth];
	tk_SourceName = info->name;
	Pos = info->pos;
	tk_Line = info->line;
	IncLineNumber = info->incLineNumber;
	Chr = info->lastChar;
	tk_Token = TK_NONE;
	AlreadyGot = false;
	
	// Pascal 12/11/08
	// Set the first include path back to this file directory
	SetLocalIncludePath(tk_SourceName);
	
	*prevMode = info->prevMode;
	return info->imported ? 2 : 0;
}

//==========================================================================
//
// TK_CloseSource
//
//==========================================================================
void TK_CloseSource()
{
	if(SourceOpen)
	{
		File.clear();
		SourceOpen = false;
	}
}

//==========================================================================
//
// TK_GetDepth
//
//==========================================================================
int TK_GetDepth()
{
	return NestDepth;
}

//==========================================================================
//
// TK_NextToken
//
//==========================================================================
tokenType_t TK_NextToken()
{
	enum ImportModes prevMode;
	bool validToken = false;

	if(AlreadyGot == true)
	{
		int t = MasterSourcePos;
		MasterSourcePos = PrevMasterSourcePos;
		PrevMasterSourcePos = t;
		AlreadyGot = false;
		return tk_Token;
	}
	PrevMasterSourcePos = MasterSourcePos;
	do
	{
		while(Chr == ASCII_SPACE)
		{
			NextChr();
		}
		switch(ASCIIToChrCode[(byte)Chr])
		{
		case CHR_EOF:
			tk_Token = TK_EOF;
			break;
		case CHR_LETTER:
			ProcessLetterToken();
			break;
		case CHR_NUMBER:
			ProcessNumberToken();
			break;
		case CHR_QUOTE:
			ProcessQuoteToken();
			break;
		default:
			ProcessSpecialToken();
			break;
		}
		if(tk_Token == TK_STARTCOMMENT)
		{
			SkipComment();
		}
		else if(tk_Token == TK_CPPCOMMENT)
		{
			SkipCPPComment();
		}
		else if((tk_Token == TK_EOF) && (NestDepth > 0))
		{
			if (PopNestedSource(&prevMode))
			{
				ImportMode = prevMode;
				if(!ExporterFlagged)
				{
					ERR_Exit(ERR_EXPORTER_NOT_FLAGGED, false);
				}
				SY_ClearShared();
			}
		}
		else
		{
			validToken = true;
		}
	} while(validToken == false);
	return tk_Token;
}

//==========================================================================
//
// TK_NextCharacter
//
//==========================================================================
int TK_NextCharacter()
{
	int c;

	while(Chr == ASCII_SPACE)
	{
		NextChr();
	}
	c = (int)Chr;
	if(c == EOF_CHARACTER)
	{
		c = -1;
	}
	NextChr();
	return c;
}

//==========================================================================
//
// TK_SkipPast
//
//==========================================================================
void TK_SkipPast(int token)
{
	while (tk_Token != token && tk_Token != TK_EOF)
	{
		TK_NextToken();
	}
	TK_NextToken();
}

//==========================================================================
//
// TK_SkipTo
//
//==========================================================================
void TK_SkipTo(int token)
{
	while (tk_Token != token && tk_Token != TK_EOF)
	{
		TK_NextToken();
	}
}

//==========================================================================
//
// TK_NextTokenMustBe
//
//==========================================================================
bool TK_NextTokenMustBe(int token, int error)
{
	if(TK_NextToken() != token)
	{
		ERR_Error(error, true);
		/*
		if(skipToken == TK_EOF)
		{
			ERR_Finish();
		}
		else if(skipToken != TK_NONE)
		{
			TK_SkipPast(skipToken);
		}
		return false;
		*/
		ERR_Finish();
	}
	return true;
}

//==========================================================================
//
// TK_TokenMustBe
//
//==========================================================================
bool TK_TokenMustBe(int token, int error)
{
	if (token == TK_SEMICOLON && forSemicolonHack)
	{
		token = TK_RPAREN;
	}
	if(tk_Token != token)
	{
		ERR_Error(error, true);
		/*
		if(skipToken == TK_EOF)
		{
			ERR_Finish();
		}
		else if(skipToken != TK_NONE)
		{
			while(tk_Token != skipToken)
			{
				TK_NextToken();
			}
		}
		return false;
		*/
		ERR_Finish();
	}
	return true;
}

//==========================================================================
//
// TK_Member
//
//==========================================================================
bool TK_Member(tokenType_t *list)
{
	int i;

	for(i = 0; list[i] != TK_NONE; i++)
	{
		if(tk_Token == list[i])
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// TK_Undo
//
//==========================================================================
void TK_Undo()
{
	if(tk_Token != TK_NONE)
	{
		if (AlreadyGot == false)
		{
			int t = MasterSourcePos;
			MasterSourcePos = PrevMasterSourcePos;
			PrevMasterSourcePos = t;
			AlreadyGot = true;
		}
	}
}

//==========================================================================
//
// ProcessLetterToken
//
//==========================================================================
static void ProcessLetterToken()
{
	int pos = 0;
	while (ASCIIToChrCode[Chr] == CHR_LETTER
		|| ASCIIToChrCode[Chr] == CHR_NUMBER)
	{
		if(pos == MAX_IDENTIFIER_LENGTH)
		{
			ERR_Error(ERR_IDENTIFIER_TOO_LONG, true);
		}
		if(pos++ < MAX_IDENTIFIER_LENGTH)
		{
			//Append the current char to the buffer
			TokenStringBuffer.append(1, Chr);
		}
		NextChr();
	}
	TokenStringBuffer.tolower();
	if(!CheckForKeyword() && !CheckForLineSpecial() && !CheckForConstant())
	{
		tk_Token = TK_IDENTIFIER;
	}
}

//==========================================================================
//
// CheckForKeyword
//
//==========================================================================
static bool CheckForKeyword()
{
	int min, max, probe, lexx;

	// [RH] Use a binary search
	min = 0;
	max = NUM_KEYWORDS-1;
	probe = NUM_KEYWORDS/2;

	while (max - min >= 0)
	{
		lexx = tk_String.compare(Keywords[probe].name);
		if(lexx == 0)
		{
			tk_Token = Keywords[probe].token;
			return true;
		}
		else if(lexx < 0)
		{
			max = probe-1;
		}
		else
		{
			min = probe+1;
		}
		probe = (max-min)/2+min;
	}

	return false;
}

//==========================================================================
//
// CheckForLineSpecial
//
//==========================================================================
static bool CheckForLineSpecial()
{
	symbolNode_t *sym;

	sym = SY_FindGlobal(tk_String);
	if(sym == NULL)
	{
		return false;
	}
	if(sym->type != SY_SPECIAL)
	{
		return false;
	}
	tk_Token = TK_LINESPECIAL;
	tk_SpecialValue = sym->info.special.value;
	tk_SpecialArgCount = sym->info.special.argCount;
	return true;
}

//==========================================================================
//
// CheckForConstant
//
//==========================================================================
static bool CheckForConstant()
{
	symbolNode_t *sym;

	sym = SY_FindGlobal(tk_String);
	if(sym == NULL)
	{
		return false;
	}
	if(sym->type != SY_CONSTANT)
	{
		return false;
	}
	tk_Token = TK_NUMBER;
	tk_Number = sym->info.constant.value;
	return true;
}

//==========================================================================
//
// ProcessNumberToken
//
//==========================================================================
static void ProcessNumberToken()
{
	char c;

	c = Chr;
	NextChr();
	if(c == '0' && (Chr == 'x' || Chr == 'X'))
	{ // Hexadecimal constant
		NextChr();
		EvalHexConstant();
		return;
	}
	tk_Number = c-'0';
	while(ASCIIToChrCode[(byte)Chr] == CHR_NUMBER)
	{
		tk_Number = 10*tk_Number+(Chr-'0');
		NextChr();
	}
	if(Chr == '.')
	{ // Fixed point
		NextChr(); // Skip period
		EvalFixedConstant(tk_Number);
		return;
	}
	if(Chr == ASCII_UNDERSCORE)
	{
		NextChr(); // Skip underscore
		EvalRadixConstant();
		return;
	}
	tk_Token = TK_NUMBER;
}

//==========================================================================
//
// EvalFixedConstant
//
//==========================================================================
static void EvalFixedConstant(int whole)
{
	double frac;
	double divisor;

	frac = 0;
	divisor = 1;
	while(ASCIIToChrCode[(byte)Chr] == CHR_NUMBER)
	{
		frac = 10*frac+(Chr-'0');
		divisor *= 10;
		NextChr();
	}
	tk_Number = (whole<<16)+(int)(65536.0*frac/divisor);
	tk_Token = TK_NUMBER;
}

//==========================================================================
//
// EvalHexConstant
//
//==========================================================================
static void EvalHexConstant()
{
	tk_Number = 0;
	while(ASCIIToHexDigit[(byte)Chr] != NON_HEX_DIGIT)
	{
		tk_Number = (tk_Number<<4)+ASCIIToHexDigit[(byte)Chr];
		NextChr();
	}
	tk_Token = TK_NUMBER;
}

//==========================================================================
//
// EvalRadixConstant
//
//==========================================================================
static void EvalRadixConstant()
{
	int radix;
	int digitVal;

	radix = tk_Number;
	if(radix < 2 || radix > 36)
	{
		ERR_Error(ERR_BAD_RADIX_CONSTANT, true, NULL);
		radix = 36;
	}
	tk_Number = 0;
	while((digitVal = DigitValue(Chr, radix)) != -1)
	{
		tk_Number = radix*tk_Number+digitVal;
		NextChr();
	}
	tk_Token = TK_NUMBER;
}

//==========================================================================
//
// DigitValue
//
// Returns -1 if the digit is not allowed in the specified radix.
//
//==========================================================================
static int DigitValue(char digit, int radix)
{
	digit = toupper(digit);
	if(digit < '0' || (digit > '9' && digit < 'A') || digit > 'Z')
	{
		return -1;
	}
	if(digit > '9')
	{
		digit = 10+digit-'A';
	}
	else
	{
		digit -= '0';
	}
	if(digit >= radix)
	{
		return -1;
	}
	return digit;
}

//==========================================================================
//
// ProcessQuoteToken
//
//==========================================================================
static void ProcessQuoteToken()
{
	int length = 0;
	bool escaped;
	escaped = false;
	NextChr();
	while(Chr != EOF_CHARACTER)
	{
		if(Chr == ASCII_QUOTE && !escaped) // [JB]
		{
			break;
		}
		if(++length == MAX_QUOTED_LENGTH)
		{
			ERR_Error(ERR_STRING_TOO_LONG, true, NULL);
		}
		if (length < MAX_QUOTED_LENGTH)
		{
			TokenStringBuffer.append(Chr, 1);
		}
		// escape the character after a backslash [JB]
		if(Chr == '\\')
			escaped ^= (Chr == '\\');
		else
			escaped = false;
		NextChr();
	}
	if(Chr == ASCII_QUOTE)
	{
		NextChr();
	}
	tk_Token = TK_STRING;
}

//==========================================================================
//
// ProcessSpecialToken
//
//==========================================================================
static void ProcessSpecialToken()
{
	char c;

	c = Chr;
	NextChr();
	switch(c)
	{
		case '+':
			switch(Chr)
			{
				case '=':
					tk_Token = TK_ADDASSIGN;
					NextChr();
					break;
				case '+':
					tk_Token = TK_INC;
					NextChr();
					break;
				default:
					tk_Token = TK_PLUS;
					break;
			}
			break;
		case '-':
			switch(Chr)
			{
				case '=':
					tk_Token = TK_SUBASSIGN;
					NextChr();
					break;
				case '-':
					tk_Token = TK_DEC;
					NextChr();
					break;
				case '>':
					tk_Token = TK_POINTERTO;
					NextChr();
					break;
				default:
					tk_Token = TK_MINUS;
					break;
			}
			break;
		case '*':
			switch(Chr)
			{
				case '=':
					tk_Token = TK_MULASSIGN;
					NextChr();
					break;
				case '/':
					tk_Token = TK_ENDCOMMENT;
					NextChr();
					break;
				default:
					tk_Token = TK_ASTERISK;
					break;
			}
			break;
		case '/':
			switch(Chr)
			{
				case '=':
					tk_Token = TK_DIVASSIGN;
					NextChr();
					break;
				case '/':
					tk_Token = TK_CPPCOMMENT;
					break;
				case '*':
					tk_Token = TK_STARTCOMMENT;
					NextChr();
					break;
				default:
					tk_Token = TK_SLASH;
					break;
			}
			break;
		case '%':
			if(Chr == '=')
			{
				tk_Token = TK_MODASSIGN;
				NextChr();
			}
			else
			{
				tk_Token = TK_PERCENT;
			}
			break;
		case '=':
			if(Chr == '=')
			{
				tk_Token = TK_EQ;
				NextChr();
			}
			else
			{
				tk_Token = TK_ASSIGN;
			}
			break;
		case '<':
			if(Chr == '=')
			{
				tk_Token = TK_LE;
				NextChr();
			}
			else if(Chr == '<')
			{
				NextChr();
				if(Chr == '=')
				{
					tk_Token = TK_LSASSIGN;
					NextChr();
				}
				else
				{
					tk_Token = TK_LSHIFT;
				}
				
			}
			else
			{
				tk_Token = TK_LT;
			}
			break;
		case '>':
			if(Chr == '=')
			{
				tk_Token = TK_GE;
				NextChr();
			}
			else if(Chr == '>')
			{
				NextChr();
				if(Chr == '=')
				{
					tk_Token = TK_RSASSIGN;
					NextChr();
				}
				else
				{
					tk_Token = TK_RSHIFT;
				}
			}
			else
			{
				tk_Token = TK_GT;
			}
			break;
		case '!':
			if(Chr == '=')
			{
				tk_Token = TK_NE;
				NextChr();
			}
			else
			{
				tk_Token = TK_NOT;
			}
			break;
		case '&':
			if(Chr == '&')
			{
				tk_Token = TK_ANDLOGICAL;
				NextChr();
			}
			else if(Chr == '=')
			{
				tk_Token = TK_ANDASSIGN;
				NextChr();
			}
			else
			{
				tk_Token = TK_ANDBITWISE;
			}
			break;
		case '|':
			if(Chr == '|')
			{
				tk_Token = TK_ORLOGICAL;
				NextChr();
			}
			else if(Chr == '=')
			{
				tk_Token = TK_ORASSIGN;
				NextChr();
			}
			else
			{
				tk_Token = TK_ORBITWISE;
			}
			break;
		case '(':
			tk_Token = TK_LPAREN;
			break;
		case ')':
			tk_Token = TK_RPAREN;
			break;
		case '{':
			tk_Token = TK_LBRACE;
			break;
		case '}':
			tk_Token = TK_RBRACE;
			break;
		case '[':
			tk_Token = TK_LBRACKET;
			break;
		case ']':
			tk_Token = TK_RBRACKET;
			break;
		case ':':
			tk_Token = TK_COLON;
			break;
		case ';':
			tk_Token = TK_SEMICOLON;
			break;
		case ',':
			tk_Token = TK_COMMA;
			break;
		case '.':
			tk_Token = TK_PERIOD;
			break;
		case '#':
			tk_Token = TK_NUMBERSIGN;
			break;
		case '^':
			if(Chr == '=')
			{
				tk_Token = TK_EORASSIGN;
				NextChr();
			}
			else
			{
				tk_Token = TK_EORBITWISE;
			}
			break;
		case '~':
			tk_Token = TK_TILDE;
			break;
		case '?':
			tk_Token = TK_QSTART;
			break;
		case '\'':
			if(Chr == '\\')
			{
				NextChr();
				switch(Chr)
				{
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					tk_Number = OctalChar();
					break;
				case 'x': case 'X':
					NextChr();
					EvalHexConstant();
					if(Chr != '\'')
					{
						ERR_Exit(ERR_BAD_CHARACTER_CONSTANT, true, NULL);
					}
					NextChr();
					break;
				case 'a':
					tk_Number = '\a';
					break;
				case 'b':
					tk_Number = '\b';
					break;
				case 't':
					tk_Number = '\t';
					break;
				case 'v':
					tk_Number = '\v';
					break;
				case 'n':
					tk_Number = '\n';
					break;
				case 'f':
					tk_Number = '\f';
					break;
				case 'r':
					tk_Number = '\r';
					break;
				case '\'':
				case '\\':
					tk_Number = Chr;
					break;
				default:
					ERR_Exit(ERR_BAD_CHARACTER_CONSTANT, true, NULL);
				}
				tk_Token = TK_NUMBER;
			}
			else if(Chr == '\'')
			{
				ERR_Exit(ERR_BAD_CHARACTER_CONSTANT, true, NULL);
			}
			else
			{
				tk_Number = Chr;
				tk_Token = TK_NUMBER;
			}
			NextChr();
			if(Chr != '\'')
			{
				ERR_Exit(ERR_BAD_CHARACTER_CONSTANT, true, NULL);
			}
			NextChr();
			break;
		default:
			ERR_Exit(ERR_BAD_CHARACTER, true, NULL);
			break;
	}
}

//==========================================================================
//
// NextChr
//
//==========================================================================
static void NextChr()
{
	if(Pos + 1 >= File.size)
	{
		Chr = EOF_CHARACTER;
		return;
	}
	if(IncLineNumber)
	{
		tk_Line++;
		IncLineNumber = false;
		BumpMasterSourceLine('x',true); // dummy x
	}
	Chr = File[Pos + 1];
	if(Chr < ASCII_SPACE && Chr >= 0)	// Allow high ASCII characters
	{
		if(Chr == '\n')
		{
			IncLineNumber = true;
		}
		Chr = ASCII_SPACE;
	}
	BumpMasterSourceLine(Chr,false);
}

//==========================================================================
//
// PeekChr // [JB]
//
//==========================================================================
static char PeekChr()
{
	if (Pos + 1 >= File.size)
		return EOF_CHARACTER;
	
	char ch = File[Pos + 1];

	if(ch < ASCII_SPACE && ch >= 0) // Allow high ASCII characters
		ch = ASCII_SPACE;
	
	return ch;
}

//==========================================================================
//
// OctalChar // [JB]
//
//==========================================================================
static int OctalChar() 
{
	int digits = 1;
	int code = Chr - '0';
	while(digits < 4 && PeekChr() >= '0' && PeekChr() <= '7')
	{
		NextChr();
		code = (code << 3) + Chr - '0';
	}
	return code;
}

//==========================================================================
//
// SkipComment
//
//==========================================================================
void SkipComment()
{
	bool hasPiece = false;

	while(Chr != EOF_CHARACTER)
	{
		if (hasPiece && Chr == '/')
		{
			break;
		}
		hasPiece = (Chr == '*');
		NextChr();
	}
	NextChr();
}

//==========================================================================
//
// BumpMasterSourceLine
//
//==========================================================================
void BumpMasterSourceLine(char Chr, bool clear) // master line - Ty 07jan2000
{
	if (ClearMasterSourceLine)  // set to clear last time, clear now for first character
	{
		MasterSourceLine = '\0';
		MasterSourcePos = 0;
		ClearMasterSourceLine = false;
	}
	if (clear)
	{
		ClearMasterSourceLine = true;
	}
	else
	{
		if (MasterSourcePos < MAX_STATEMENT_LENGTH)
			MasterSourceLine[MasterSourcePos++] = Chr;
	}
}

//==========================================================================
//
// TK_SkipLine
//
//==========================================================================
void TK_SkipLine()
{
	string sourcenow = tk_SourceName;
	int linenow = tk_Line;
	do NextChr(); while (tk_Line == linenow && tk_SourceName == sourcenow && Chr != EOF_CHARACTER);
}
