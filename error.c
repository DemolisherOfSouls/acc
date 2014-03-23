
//**************************************************************************
//**
//** error.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <iostream>
#include <fstream>
#include "common.h"
#include "error.h"
#include "token.h"
#include "misc.h"

using std::fstream;
using std::ios;
using std::cerr;
using std::endl;
using std::streambuf;
using std::to_string;

// MACROS ------------------------------------------------------------------

#define ERROR_FILE_NAME "acs.err"

// TYPES -------------------------------------------------------------------

class estream : public fstream
{
	using fstream::operator<<;
	

public:
	
	estream(string file, int openmode = 3, int prot = 64) : fstream(file.c_str(), openmode, prot) {} ;

	streambuf *log = cerr.rdbuf();

	estream& operator << (string s)
	{
		log->sputn(s.c_str(), s.length());
		rdbuf()->sputn(s.c_str(), s.length());
	}
	estream& operator << (const char * c)
	{
		log->sputn(c, string(c).length());
		rdbuf()->sputn(c, string(c).length());
	}
	estream& operator << (int i)
	{
		operator<<(to_string(i));
	}
	estream& operator << (bool b)
	{
		operator<<(to_string(b));
	}
	estream& operator << (char c)
	{
		log->sputc(c);
		rdbuf()->sputc(c);
	}
};

enum errorInfo_e : int
{
	ERRINFO_GCC,
	ERRINFO_VCC
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static string ErrorText(int error);
static string ErrorFileName();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern char acs_SourceFileName[MAX_FILE_NAME_LENGTH];

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static struct errorName_t
{
	error_t number;
	string name;
} ErrorNames[] =
{
	{ ERR_MISSING_SEMICOLON, "Missing semicolon." },
	{ ERR_MISSING_LPAREN, "Missing '('." },
	{ ERR_MISSING_RPAREN, "Missing ')'." },
	{ ERR_MISSING_LBRACE, "Missing '{'." },
	{ ERR_MISSING_SCRIPT_NUMBER, "Missing script number." },
	{ ERR_IDENTIFIER_TOO_LONG, "Identifier too long." },
	{ ERR_STRING_TOO_LONG, "String too long." },
	{ ERR_FILE_NAME_TOO_LONG, "File name too long.\nFile: \"%s\"" },
	{ ERR_BAD_CHARACTER, "Bad character in script text." },
	{ ERR_BAD_CHARACTER_CONSTANT, "Bad character constant in script text." },
	{ ERR_ALLOC_PCODE_BUFFER, "Failed to allocate PCODE buffer." },
	{ ERR_PCODE_BUFFER_OVERFLOW, "PCODE buffer overflow." },
	{ ERR_TOO_MANY_SCRIPTS, "Too many scripts." },
	{ ERR_TOO_MANY_FUNCTIONS, "Too many functions." },
	{ ERR_SAVE_OBJECT_FAILED, "Couldn't save object file." },
	{ ERR_MISSING_LPAREN_SCR, "Missing '(' in script definition." },
	{ ERR_INVALID_IDENTIFIER, "Invalid identifier." },
	{ ERR_REDEFINED_IDENTIFIER, "%s : Redefined identifier." },
	{ ERR_MISSING_COMMA, "Missing comma." },
	{ ERR_BAD_VAR_TYPE, "Invalid variable type." },
	{ ERR_BAD_RETURN_TYPE, "Invalid return type." },
	{ ERR_TOO_MANY_SCRIPT_ARGS, "Too many script arguments." },
	{ ERR_MISSING_LBRACE_SCR, "Missing opening '{' in script definition." },
	{ ERR_MISSING_RBRACE_SCR, "Missing closing '}' in script definition." },
	{ ERR_TOO_MANY_MAP_VARS, "Too many map variables." },
	{ ERR_TOO_MANY_SCRIPT_VARS, "Too many script variables." },
	{ ERR_TOO_MANY_FUNCTION_VARS, "Too many function variables." },
	{ ERR_MISSING_WVAR_INDEX, "Missing index in world variable declaration." },
	{ ERR_MISSING_GVAR_INDEX, "Missing index in global variable declaration." },
	{ ERR_BAD_WVAR_INDEX, "World variable index out of range." },
	{ ERR_MISSING_WVAR_COLON, "Missing colon in world variable declaration." },
	{ ERR_MISSING_GVAR_COLON, "Missing colon in global variable declaration." },
	{ ERR_MISSING_SPEC_VAL, "Missing value in special declaration." },
	{ ERR_MISSING_SPEC_COLON, "Missing colon in special declaration." },
	{ ERR_MISSING_SPEC_ARGC, "Missing argument count in special declaration." },
	{ ERR_CANT_READ_FILE, "Couldn't read file.\nFile: \"%s\"" },
	{ ERR_CANT_OPEN_FILE, "Couldn't open file.\nFile: \"%s\"" },
	{ ERR_CANT_OPEN_DBGFILE, "Couldn't open debug file." },
	{ ERR_INVALID_DIRECTIVE, "Invalid directive." },
	{ ERR_BAD_DEFINE, "Non-numeric constant found in #define." },
	{ ERR_INCL_NESTING_TOO_DEEP, "Include nesting too deep.\nUnable to include file \"%s\"." },
	{ ERR_STRING_LIT_NOT_FOUND, "String literal not found." },
	{ ERR_INVALID_DECLARATOR, "Invalid declarator." },
	{ ERR_BAD_LSPEC_ARG_COUNT, "Incorrect number of special arguments." },
	{ ERR_BAD_ARG_COUNT, "Incorrect number of arguments." },
	{ ERR_UNKNOWN_IDENTIFIER, "%s : Identifier has not been declared." },
	{ ERR_MISSING_COLON, "Missing colon." },
	{ ERR_BAD_EXPR, "Syntax error in expression." },
	{ ERR_BAD_CONST_EXPR, "Syntax error in constant expression." },
	{ ERR_NO_DIRECT_VER, "Internal function has no direct version." },
	{ ERR_ILLEGAL_EXPR_IDENT, "%s : Illegal identifier in expression." },
	{ ERR_EXPR_FUNC_NO_RET_VAL, "Function call in expression has no return value." },
	{ ERR_MISSING_ASSIGN_OP, "Missing assignment operator." },
	{ ERR_INCDEC_OP_ON_NON_VAR, "'++' or '--' used on a non-variable." },
	{ ERR_MISSING_RBRACE, "Missing '}' at end of compound statement." },
	{ ERR_INVALID_STATEMENT, "Invalid statement." },
	{ ERR_BAD_DO_STATEMENT, "Do statement not followed by 'while' or 'until'." },
	{ ERR_BAD_SCRIPT_DECL, "Bad script declaration." },
	{ ERR_CASE_OVERFLOW, "Internal Error: Case stack overflow." },
	{ ERR_BREAK_OVERFLOW, "Internal Error: Break stack overflow." },
	{ ERR_CONTINUE_OVERFLOW, "Internal Error: Continue stack overflow." },
	{ ERR_STATEMENT_OVERFLOW, "Internal Error: Statement overflow." },
	{ ERR_MISPLACED_BREAK, "Misplaced BREAK statement." },
	{ ERR_MISPLACED_CONTINUE, "Misplaced CONTINUE statement." },
	{ ERR_CASE_NOT_IN_SWITCH, "CASE must appear in switch statement." },
	{ ERR_DEFAULT_NOT_IN_SWITCH, "DEFAULT must appear in switch statement." },
	{ ERR_MULTIPLE_DEFAULT, "Only 1 DEFAULT per switch allowed." },
	{ ERR_EXPR_STACK_OVERFLOW, "Expression stack overflow." },
	{ ERR_EXPR_STACK_EMPTY, "Tried to POP empty expression stack." },
	{ ERR_UNKNOWN_CONST_EXPR_PCD, "Unknown PCD in constant expression." },
	{ ERR_BAD_RADIX_CONSTANT, "Radix out of range in integer constant." },
	{ ERR_BAD_ASSIGNMENT, "Syntax error in multiple assignment statement." },
	{ ERR_OUT_OF_MEMORY, "Out of memory." },
	{ ERR_TOO_MANY_STRINGS, "Too many strings. Current max is %d" },
	{ ERR_UNKNOWN_PRTYPE, "Unknown cast type in print statement." },
	{ ERR_SCRIPT_OUT_OF_RANGE, "Script number must be between 1 and 32767." },
	{ ERR_MISSING_PARAM, "Missing required argument." },
	{ ERR_SCRIPT_ALREADY_DEFINED, "Script already has a body." },
	{ ERR_FUNCTION_ALREADY_DEFINED, "Function already has a body." },
	{ ERR_PARM_MUST_BE_VAR, "Parameter must be a variable." },
	{ ERR_LANGCODE_SIZE, "Language code must be 2 or 3 characters long." },
	{ ERR_MISSING_LBRACE_LOC, "Missing opening '{' in localization definition." },
	{ ERR_MISSING_RBRACE_LOC, "Missing closing '}' in localization definition." },
	{ ERR_MISSING_LOCALIZED, "Missing localized string." },
	{ ERR_BAD_LANGCODE, "Language code must be all letters." },
	{ ERR_MISSING_LANGCODE, "Missing language code in localization definiton." },
	{ ERR_MISSING_FONT_NAME, "Missing font name." },
	{ ERR_MISSING_LBRACE_FONTS, "Missing opening '{' in font list." },
	{ ERR_MISSING_RBRACE_FONTS, "Missing closing '}' in font list." },
	{ ERR_NOCOMPACT_NOT_HERE, "#nocompact must appear before any scripts." },
	{ ERR_MISSING_ASSIGN, "Missing '='." },
	{ ERR_PREVIOUS_NOT_VOID, "Previous use of function expected a return value." },
	{ ERR_MUST_RETURN_A_VALUE, "Function must return a value." },
	{ ERR_MUST_NOT_RETURN_A_VALUE, "Void functions cannot return a value." },
	{ ERR_SUSPEND_IN_FUNCTION, "Suspend cannot be used inside a function." },
	{ ERR_TERMINATE_IN_FUNCTION, "Terminate cannot be used inside a function." },
	{ ERR_RESTART_IN_FUNCTION, "Restart cannot be used inside a function." },
	{ ERR_RETURN_OUTSIDE_FUNCTION, "Return can only be used inside a function." },
	{ ERR_FUNC_ARGUMENT_COUNT, "Function %s should have %d arguments." },
	{ ERR_EOF, "Unexpected end of file." },
	{ ERR_UNDEFINED_FUNC, "Function %s is used but not defined." },
	{ ERR_TOO_MANY_ARRAY_DIMS, "Too many array dimensions." },
	{ ERR_TOO_MANY_ARRAY_INIT, "Too many initializers for array." },
	{ ERR_MISSING_LBRACKET, "Missing '['." },
	{ ERR_MISSING_RBRACKET, "Missing ']'." },
	{ ERR_ZERO_DIMENSION, "Arrays cannot have a dimension of zero." },
	{ ERR_TOO_MANY_DIM_USED, "%s only has %d dimensions." },
	{ ERR_TOO_FEW_DIM_USED, "%s access needs %d more dimensions." },
	{ ERR_ARRAY_MAPVAR_ONLY, "Only map variables can be arrays." },
	{ ERR_NOT_AN_ARRAY, "%s is not an array." },
	{ ERR_MISSING_LBRACE_ARR, "Missing opening '{' in array initializer." },
	{ ERR_MISSING_RBRACE_ARR, "Missing closing '}' in array initializer." },
	{ ERR_LATENT_IN_FUNC, "Latent functions cannot be used inside functions." },
	{ ERR_LOCAL_VAR_SHADOWED, "A global identifier already has this name." },
	{ ERR_MULTIPLE_IMPORTS, "You can only #import one file." },
	{ ERR_IMPORT_IN_EXPORT, "You cannot #import from inside an imported file." },
	{ ERR_EXPORTER_NOT_FLAGGED, "A file that you #import must have a #library line." },
	{ ERR_TOO_MANY_IMPORTS, "Too many files imported." },
	{ ERR_NO_NEED_ARRAY_SIZE, "Only map arrays need a size." },
	{ ERR_NO_MULTIDIMENSIONS, "Only map arrays can have more than one dimension." },
	{ ERR_NEED_ARRAY_SIZE, "Missing array size." },
	{ ERR_DISCONNECT_NEEDS_1_ARG, "Disconnect scripts must have 1 argument." },
	{ ERR_UNCLOSED_WITH_ARGS, "Most special scripts must not have arguments." },
	{ ERR_NOT_A_CHAR_ARRAY, "%s has %d dimensions. Use %d subscripts to get a char array." },
	{ ERR_CANT_FIND_INCLUDE, "Couldn't find include file \"%s\"." },
	{ ERR_SCRIPT_NAMED_NONE, "Scripts may not be named \"None\"." },
	{ ERR_HEXEN_COMPAT, "Attempt to use feature not supported by Hexen." },
	{ ERR_NOT_HEXEN, "Cannot save; new features are not compatible with Hexen." },
	{ ERR_SPECIAL_RANGE, "Line specials with values higher than 255 require #nocompact." },
	//[JRT] New errors
	{ ERR_BAD_CONSTRUCTOR, "%s: Constructor is not properly formed." },
	{ ERR_BAD_METHOD, "Method %s is not found within %s." },
	{ ERR_NO_STRUCT_ARRAY_INIT, "%s: Cannot auto-initialize an array of structs, must be initialized to default constructor."},
	{ ERR_INVALID_ARRAY_SIZE, "Invalid array size, dimensions must be positive integers." },
	//[JRT] End new errors
	{ ERR_NONE, "" }
};

static estream ErrorFile;
static int ErrorCount = 0;
static errorInfo_e ErrorFormat;
static string ErrorSourceName;
static int ErrorSourceLine;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// ERR_ErrorAt
//
//==========================================================================
void ERR_ErrorAt(string source, int line)
{
	ErrorSourceName = source;
	ErrorSourceLine = line;
}

//==========================================================================
//
// ERR_Error
//
//==========================================================================
void ERR_Error(int error, bool info, ...)
{
	va_list args;
	va_start(args, info);
	ERR_ErrorV(error, info, args);
	va_end(args);
}

//==========================================================================
//
// ERR_Exit
//
//==========================================================================
void ERR_Exit(int error, bool info, ...)
{
	va_list args;
	va_start(args, info);
	ERR_ErrorV(error, info, args);
	va_end(args);
	ERR_Finish();
}

//==========================================================================
//
// ERR_Finish
//
//==========================================================================
void ERR_Finish()
{
	if (ErrorFile.is_open())
	{
		ErrorFile.flush();
		ErrorFile.close();
	}
	
	if(ErrorCount)
		exit(1);
}

//==========================================================================
//
// ShowError
//
//==========================================================================
void ERR_ErrorV(int error, bool info, va_list args)
{
	ErrorFile = estream(ErrorFileName(), ios::out | ios::trunc);
	bool showLine = false;
	static bool showedInfo = false;

	if(!ErrorFile.is_open())
		ErrorFile.open(ErrorFileName(), ios::out| ios::trunc);
	
	if (ErrorCount == 0)
		cerr << endl << "**** ERROR ****" << endl;
	
	else if(ErrorCount >= 100)
	{
		ErrorFile << "More than 100 errors. Can't continue.\n";
		ERR_Finish();
	}
	ErrorCount++;
	if(info)
	{
		string source;
		int line;

		if(!ErrorSourceName.empty())
		{
			source = ErrorSourceName;
			line = ErrorSourceLine;
			ErrorSourceName.clear();
		}
		else
		{
			source = tk_SourceName;
			line = tk_Line;
			showLine = true;
		}
		if(!showedInfo)
        { // Output info compatible with older ACCs
          // for editors that expect it.
			showedInfo = true;
			ErrorFile << "Line " << line << " in file \"" << source << "\" ...\n";
		}
		if(ErrorFormat == ERRINFO_GCC)
		{
			ErrorFile << source << ":" << line << ": ";
		}
		else
		{
			ErrorFile << source << "(" << line << ") : ";
			if(error != ERR_NONE)
			{
				ErrorFile << "error " << error << ": ";
			}
		}
	}
	if(error != ERR_NONE)
	{
		string text = ErrorText(error);
		if(!text.empty())
		{
			veprintf(text, args);
		}
		ErrorFile << '\n';
		if(showLine)
		{
			// deal with master source line and position indicator - Ty 07jan2000
			MasterSourceLine = MasterSourceLine.substr(0, MasterSourcePos); // pre-incremented already

			ErrorFile << "> " << MasterSourceLine << '\n';
			ErrorFile << ">" << string(' ', MasterSourcePos - 1) << "^" << '\n';
		}
	}
}

//==========================================================================
//
// ERR_RemoveErrorFile
//
//==========================================================================
void ERR_RemoveErrorFile()
{
	remove(ErrorFileName().c_str());
}

//==========================================================================
//
// ErrorFileName
//
//==========================================================================
static string ErrorFileName()
{
	string errFileName;

	errFileName = acs_SourceFileName;

	if(!MS_StripFilename(errFileName))
		errFileName = ERROR_FILE_NAME;
	else
		errFileName += ERROR_FILE_NAME;

	return errFileName;
}

//==========================================================================
//
// ERR_BadAlloc - [JRT]
// new_handler
//
//==========================================================================
void ERR_BadAlloc()
{
	ERR_Exit(ERR_OUT_OF_MEMORY, false);
}

//==========================================================================
//
// ErrorText
//
//==========================================================================
static string ErrorText(int error)
{
	for each (errorName_t err in ErrorNames)
		if (error == err.number)
			return err.name;
	return "";
}
