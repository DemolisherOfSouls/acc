//**************************************************************************
//**
//** acc.cpp
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <cstdio>
#include <cstdlib>

#include "common.h"
#include "token.h"
#include "error.h"
#include "symbol.h"
#include "misc.h"
#include "pcode.h"
#include "parse.h"
#include "strlist.h"

using std::set_new_handler;

// MACROS ------------------------------------------------------------------

#define VERSION_TEXT "1.6"
#define COPYRIGHT_YEARS_TEXT "1995"

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void Init();
static void DisplayBanner();
static void DisplayUsage();
static void OpenDebugFile(string name);
static void ProcessArgs();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

bool acs_BigEndianHost;
bool acs_VerboseMode;
bool acs_DebugMode;
ofstream acs_DebugFile;
string acs_SourceFileName;
string acs_ErrorFileName;		// User defined error file name
								// TODO: Maybe add the ability to add a path?
								// TODO: Add error checking.

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int ArgCount;
static char **ArgVector;
static string ObjectFileName;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// main
//
//==========================================================================
int main(int argc, char **argv)
{
	set_new_handler(ERR_BadAlloc);

	ArgCount = argc;
	ArgVector = argv;

	VecStr args = VecStr(argc);

	for (int i = 0; i < argc; i++)
	{

	}

	DisplayBanner();
	Init();
	TK_OpenSource(acs_SourceFileName);
	PC_OpenObject(ObjectFileName, DEFAULT_OBJECT_SIZE, 0);
	PA_Parse();
	PC_CloseObject();
	TK_CloseSource();

	line();
	cerr << "\"" << acs_SourceFileName << "\":" << endl
		<< " " << tk_Line << " line" << ((tk_Line == 1) ? "" : "s")
		<< " (" << tk_IncludedLines << " included)" << endl;

	cerr << "  " << pCode_FunctionCount << " function"
		<< ((pCode_FunctionCount == 1) ? "" : "s") << endl
		<< " " << pCode_ScriptCount << ((pa_ScriptCount == 1) ? "" : "s");



	for (int i = 0; pa_TypedScriptCounts[i].TypeName; i++)
	{
		if (pa_TypedScriptCounts[i].TypeCount > 0)
		{
			cerr << pa_TypedScriptCounts[i].TypeCount << " " << pa_TypedScriptCounts[i].TypeName;
		}
	}
	cerr << "  " << pa_GlobalVarCount << " global variable" << (pa_GlobalVarCount == 1 ? "" : "s") << endl
		<< "  " << pa_WorldVarCount << " world variable" << (pa_WorldVarCount == 1 ? "" : "s") << endl
		<< "  " << pa_MapVarCount << " map variable" << (pa_MapVarCount == 1 ? "" : "s") << endl
		<< "  " << pa_GlobalArrayCount << " global array" << (pa_GlobalArrayCount == 1 ? "" : "s") << endl
		<< "  " << pa_WorldArrayCount << " world array" << (pa_WorldArrayCount == 1 ? "" : "s") << endl;
	cerr << "  object \"" << ObjectFileName << "\": " << pCode_Buffer.size() << " bytes" << endl;
	ERR_RemoveErrorFile();
	return 0;
}

//==========================================================================
//
// DisplayBanner
//
//==========================================================================
static void DisplayBanner()
{
	line();
	line("Original ACC Version 1.10 by Ben Gokey");
	line("Copyright (c) " + string(COPYRIGHT_YEARS_TEXT) + " Raven Software, Corp.");
	line();
	line("This is version " + string(VERSION_TEXT) + " (" + string(__DATE__) + ")");
	line("This software is not supported by Raven Software or Activision");
	line("ZDoom changes and language extensions by Randy Heit");
	line("Further changes by Brad Carney");
	line("Even more changes by James Bentler");
	line("Some additions by Michael \"Necromage\" Weber");
	line("Error reporting improvements and limit expansion by Ty Halderman");
	line("Include paths added by Pascal vd Heiden");
	line("Additional operators, structs, and methods added by J. Ryan \"DemolisherOfSouls\" Taylor");
}

//==========================================================================
//
// Init
//
//==========================================================================
static void Init()
{
	short endianTest = 1;

	if (*(char *)&endianTest)
		acs_BigEndianHost = false;
	else
		acs_BigEndianHost = true;
	acs_VerboseMode = true;
	acs_DebugMode = false;
	acs_DebugFile = ofstream();
	TK_Init();
	sym_Init();
	STR_Init();
	ProcessArgs();
	MS_Message(MSG_NORMAL, "Host byte order: %s endian\n",
		acs_BigEndianHost ? "BIG" : "LITTLE");
}

//==========================================================================
//
// ProcessArgs
//
// Pascal 12/11/08
// Allowing space after options (option parameter as the next argument)
//
//==========================================================================
static void ProcessArgs()
{
	int count = 0, i = 1;
	string text;
	char option;
	bool grabErrorFile = false;
	
	while(i < ArgCount)
	{
		text = ArgVector[i];
		auto iter = text.begin();

		if(*iter == '-')
		{
			// If incorrect or ends, display usage
			if (++iter == text.end())
				DisplayUsage();

			// Option
			option = *iter;
			switch(option)
			{
				case 'I':
					if((i + 1) < ArgCount)
					{
						TK_AddIncludePath(ArgVector[++i]);
					}
					break;
					
				case 'D':
					acs_DebugMode = true;
					acs_VerboseMode = true;
					if(iter != text.end())
					{
						OpenDebugFile(text);
					}
					break;
					
				case 'H':
					pCode_NoShrink = true;
					pCode_HexenCase = true;
					pCode_EnforceHexen = toupper(*iter) != 'H';
					pCode_WarnNotHexen = toupper(*iter) == 'H';
					break;
				case 'F':
					grabErrorFile = true;
					if (iter != text.end())
					{
						acs_ErrorFileName = text.substr(2, text.length(-2));
					}
					break;
				default:
					DisplayUsage();
					break;
			}
		}
		else
		{
			// Input/output file
			count++;
			switch(count)
			{
				case 1:
					acs_SourceFileName = text;
					MS_SuggestFileExt(acs_SourceFileName, ".acs");
					break;
					
				case 2:
					ObjectFileName = text;
					MS_SuggestFileExt(ObjectFileName, ".o");
					break;
					
				default:
					DisplayUsage();
					break;
			}
		}
		
		// Next arg
		i++;
	}
	
	if(count == 0)
	{
		DisplayUsage();
	}

	TK_AddIncludePath(".");
#ifdef unix
	TK_AddIncludePath("/usr/local/share/acc/");
#endif
	TK_AddProgramIncludePath(ArgVector[0]);
	
	if(count == 1)
	{
		ObjectFileName = acs_SourceFileName;
		MS_StripFileExt(ObjectFileName);
		MS_SuggestFileExt(ObjectFileName, ".o");
	}
}

//==========================================================================
//
// DisplayUsage
//
//==========================================================================
static void DisplayUsage()
{
	line("\nUsage: ACC [options] source[.acs] [object[.o]]\n");
	line("-i [path]  Add include path to find include files");
	line("-d[file]   Output debugging information");
	line("-h         Create pcode compatible with Hexen and old ZDooms");
	line("-hh        Like -h, but use of new features is only a warning");
	line("-e         Use single line error and warning messages");
	line("-f[file]   Output error information to the specified file");
	line("-w0        Ignore all warnings"); //TODO: add warnings
	line("-w#        Sets the desired warning level, where '#' is 1-4");
	line("-we        Treat all warnings as errors");
	line("-cs        Enforces case sensitivity"); //TODO: Add case sensitivity
	exit(1);
}

//==========================================================================
//
// OpenDebugFile
//
//==========================================================================
static void OpenDebugFile(string name)
{
	acs_DebugFile.open(name, ios::out, ios::trunc);

	if(!acs_DebugFile.is_open)
		ERR_Exit(ERR_CANT_OPEN_DBGFILE, false, "File: \"%s\".", name);
}