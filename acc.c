
//**************************************************************************
//**
//** acc.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "token.h"
#include "error.h"
#include "symbol.h"
#include "misc.h"
#include "pcode.h"
#include "parse.h"
#include "strlist.h"

using std::fstream;
using std::ios;
using std::cerr;
using std::endl;

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
static void OpenDebugFile(char *name);
static void ProcessArgs();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

bool acs_BigEndianHost;
bool acs_VerboseMode;
bool acs_DebugMode;
string acs_DebugFile;
string acs_SourceFileName;

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
	std::set_new_handler(ERR_BadAlloc);

	int i;

	ArgCount = argc;
	ArgVector = argv;
	DisplayBanner();
	Init();
	TK_OpenSource(acs_SourceFileName);
	PC_OpenObject(ObjectFileName, DEFAULT_OBJECT_SIZE, 0);
	PA_Parse();
	PC_CloseObject();
	TK_CloseSource();

	cerr << endl << "\"" << acs_SourceFileName << "\":" << endl
		<< " " << tk_Line << " line" << ((tk_Line == 1) ? "" : "s")
		<< " (" << tk_IncludedLines << " included)" << endl;

	cerr << "  " << pCode_FunctionCount << " function"
		<< ((pCode_FunctionCount == 1) ? "" : "s") << endl
		<< " " << pCode_ScriptCount << ((pa_ScriptCount == 1) ? "" : "s");



	for (i = 0; pa_TypedScriptCounts[i].TypeName; i++)
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
	cerr << 
	fprintf(stderr, "\nOriginal ACC Version 1.10 by Ben Gokey\n");
	fprintf(stderr, "Copyright (c) "COPYRIGHT_YEARS_TEXT
		" Raven Software, Corp.\n\n");
	fprintf(stderr, "This is version "VERSION_TEXT" ("__DATE__")\n");
	fprintf(stderr, "This software is not supported by Raven Software or Activision\n");
	fprintf(stderr, "ZDoom changes and language extensions by Randy Heit\n");
	fprintf(stderr, "Further changes by Brad Carney\n");
	fprintf(stderr, "Even more changes by James Bentler\n");
	fprintf(stderr, "Some additions by Michael \"Necromage\" Weber\n");
	fprintf(stderr, "Error reporting improvements and limit expansion by Ty Halderman\n");
	fprintf(stderr, "Include paths added by Pascal vd Heiden\n");
	fprintf(stderr, "Additional operators, structs, and methods added by J. Ryan \"DemolisherOfSouls\" Taylor");
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
	acs_DebugFile = NULL;
	TK_Init();
	SY_Init();
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
	int i = 1;
	int count = 0;
	char *text;
	char option;
	
	while(i < ArgCount)
	{
		text = ArgVector[i];
		
		if(*text == '-')
		{
			// Option
			text++;
			if(*text == 0)
			{
				DisplayUsage();
			}
			option = toupper(*text++);
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
					if(*text != 0)
					{
						OpenDebugFile(text);
					}
					break;
					
				case 'H':
					pc_NoShrink = true;
					pc_HexenCase = true;
					pc_EnforceHexen = toupper(*text) != 'H';
					pc_WarnNotHexen = toupper(*text) == 'H';
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
					strcpy(acs_SourceFileName, text);
					MS_SuggestFileExt(acs_SourceFileName, ".acs");
					break;
					
				case 2:
					strcpy(ObjectFileName, text);
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
		strcpy(ObjectFileName, acs_SourceFileName);
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
	puts("\nUsage: ACC [options] source[.acs] [object[.o]]\n");
	puts("-i [path]  Add include path to find include files");
	puts("-d[file]   Output debugging information");
	puts("-h         Create pcode compatible with Hexen and old ZDooms");
	puts("-hh        Like -h, but use of new features is only a warning");
	exit(1);
}

//==========================================================================
//
// OpenDebugFile
//
//==========================================================================

static void OpenDebugFile(char *name)
{
	if((acs_DebugFile = fopen(name, "w")) == NULL)
	{
		ERR_Exit(ERR_CANT_OPEN_DBGFILE, false, "File: \"%s\".", name);
	}
}

//==========================================================================
//
// OptionExists
//
//==========================================================================

/*
static bool OptionExists(char *name)
{
	int i;
	char *arg;

	for(i = 1; i < ArgCount; i++)
	{
		arg = ArgVector[i];
		if(*arg == '-')
		{
			arg++;
			if(MS_StrCmp(name, arg) == 0)
			{
				return true;
			}
		}
	}
	return false;
}
*/
