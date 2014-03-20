
//**************************************************************************
//**
//** common.h
//**
//**************************************************************************

//TODO: implement strings over char* arrays
//TODO: implement vectors over resizing pointers
//TODO: implement proper data types

#pragma once

// HEADER FILES ------------------------------------------------------------

#include <string>
#include <vector>

// MACROS ------------------------------------------------------------------

// Increased limits - Ty 03jan2000
#define MAX_IDENTIFIER_LENGTH 32	// 32 is okay
#define MAX_QUOTED_LENGTH 32768		// 32k long quoted string should be okay
#define MAX_FILE_NAME_LENGTH 512	// 512 max file name is okay in DOS/Win
#define MAX_SCRIPT_COUNT 1000		// Was 64
#define MAX_MAP_VARIABLES 128		// Was 32
// Left alone--there's something in the docs about this...
// [RH] Bumped up to 20 for fun.
#define MAX_SCRIPT_VARIABLES 20
#define MAX_WORLD_VARIABLES 256		// Was 64
#define MAX_GLOBAL_VARIABLES 64		// [RH] New
#define MAX_STRINGS 32768			// Was 128
#define DEFAULT_OBJECT_SIZE 65536	// This is the default initial file size for loaded acs files
#define MAX_STATEMENT_LENGTH 4096	// Added Ty 07Jan2000 for error details
#define MAX_LANGUAGES 256			// 
#define MAX_FUNCTION_COUNT 256		// 
#define MAX_IMPORTS 256				// 
// Max number of include paths the user can specify
// This includes the "working directory"!
#define MAX_INCLUDE_PATHS 16
#define MAX_TRANSLATIONS 32			// Maximum number of translations that can be used

// [JRT] New internal limits on the added objects
#define MAX_STRUCT_OBJECTS 512		// Will raise this if needed
#define MAX_STRUCT_VARIABLES 128	// "
#define MAX_STRUCT_STATICS 32		// "
#define MAX_STRUCT_METHODS 64		// "

// These are just defs and have not been messed with
#define ASCII_SPACE 32
#define ASCII_QUOTE 34
#define ASCII_UNDERSCORE 95
#define EOF_CHARACTER 127
#ifdef __NeXT__
#define DIRECTORY_DELIMITER "/"
#define DIRECTORY_DELIMITER_CHAR ('/')
#else
#define DIRECTORY_DELIMITER "\\"
#define DIRECTORY_DELIMITER_CHAR ('\\')
#endif

#define MAKE4CC(a,b,c,d)	((a)|((b)<<8)|((c)<<16)|((d)<<24))

// TYPES -------------------------------------------------------------------

//Just for looks, honestly
typedef unsigned char byte;

enum ImportModes : int
{
	IMPORT_None,
	IMPORT_Importing,
	IMPORT_Exporting
};

enum : int
{
	STRLIST_PICS,
	STRLIST_FUNCTIONS,
	STRLIST_MAPVARS,
	STRLIST_NAMEDSCRIPTS,
	STRLIST_STRUCTS,
	STRLIST_METHODS,

	NUM_STRLISTS
};

//[JRT] Slightly modified std::string
// Added method: length (int padding)
// Added char constructor
// Default constructor is an empty string
class string : public std::string
{
public:
	
	using std::string::operator=;
	using std::string::operator+=;
	using std::string::operator[];
	using std::string::length;

	string()				: std::string() {}
	string(std::string s)	: std::string(s) {}
	string(char* c)			: std::string(c) {}
	string(const string& s)	: std::string(s) {}

	int length(int padding)
	{
		return (length() + padding);
	}
	void tolower()
	{
		for (int c = 0; c < this->length(); c++)
		{
			if (this[0][c] >= 'A' && this[0][c] <= 'Z')
				this[0][c] += 32;
		}
	}
	void toupper()
	{
		for (int c = 0; c < this->length(); c++)
		{
			if (this[0][c] >= 'a' && this[0][c] <= 'z')
				this[0][c] -= 32;
		}
	}
};

//[JRT] Slightly modified std::vector
// Added input operator <<
// Added last_index property
// Added nextIndex property
template <class type> //Vector data type
class vector : public std::vector<type>
{
	int last_index;

	typedef type& reference;
	typedef type* pointer;
	typedef const type& const_reference;
	typedef const type* const_pointer;
	typedef int size_type;
	typedef type value_type;

	pointer a_pointer(reference item)
	{
		return *item;
	}

public:

	using std::vector::operator=;
	using std::vector::resize;
	using std::vector::push_back;
	using std::vector::at;
	using std::vector::size;
	using std::vector::data;
	using std::vector::empty;
	using std::vector::capacity;

	vector() : std::vector() {}
	vector(int size) : std::vector(size) {}
	vector(const_reference rItem) : std::vector(rItem) {}
	vector(type rItem) : std::vector(rItem) {}

	void operator<< (type rItem)
	{
		add(rItem);
	}
	reference operator[] (int index)
	{
		if (index >= size())
		{
			resize(index + 1);
		}
		if (index < 0)
		{
			index = 0;
		}
		return at(index);
	}

	int lastIndex()
	{
		return last_index;
	}
	int nextIndex()
	{
		return size();
	}

	reference lastAdded()
	{
		return at(last_index);
	}

	void add(type item)
	{
		push_back(item);
		last_index = size() - 1;
	}

	//Add item to index 'pos', moving the item there to the end if necessary
	void prefer(int pos, type item)
	{
		// Resize is needed
		if (pos >= capacity())
			resize();

		// Check to see if position is filled
		if (pos + 1 < size())
		{
			//Add item in desired place to end,
			//then assign the item to the desired place
			add(at(pos));
			at(pos) = item;
			last_index = pos;
		}
		else
		{
			at(pos) = item;
		}
	}
	
	void resize()
	{
		std::vector::resize(size() * 2);
	}
	void resize(int size)
	{
		std::vector::resize(size);
	}
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PUBLIC DATA DECLARATIONS ------------------------------------------------
