
//**************************************************************************
//**
//** common.h
//**
//**************************************************************************

//TODO: implement strings over char* arrays (Almost Done)
//TODO: implement vectors over resizing pointers (Almost Done)
//TODO: implement proper data types (Almost Done)

#pragma once

// HEADER FILES ------------------------------------------------------------

#include <string>
#include <vector>
#include <algorithm>

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

// [JRT] Convenience macros
#define set(member) this->member = member
#define lib(member) list[index].member = member
#define libset(member) list[index].member = set(member)

// TYPES -------------------------------------------------------------------

//Just for looks, honestly
using byte = unsigned char;

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

// [JRT] Slightly modified std::string
// Added method: length (int padding)
// Added methods: tolower/toupper
class string : public std::string
{
public:
	
	using std::string::operator=;
	using std::string::operator+=;
	using std::string::operator[];
	using std::string::length;

	string()				: std::string()  {}
	string(std::string s)	: std::string(s) {}
	string(char* c)			: std::string(c) {}
	string(const string& s)	: std::string(s) {}
	string(char c, int n)	: std::string(n, c) {}

	int length(int padding)
	{
		return (length() + padding);
	}
	void tolower()
	{
		for (int c = 0; c < length(); c++)
		{
			char *r = &at(c);
			if (*r >= 'A' && *r <= 'Z')
				*r += 32;
		}
	}
	void toupper()
	{
		for (int c = 0; c < length(); c++)
		{
			char *r = &at(c);
			if (*r >= 'a' && *r <= 'z')
				*r -= 32;
		}
	}
};

//[JRT] Slightly modified std::vector
// Added input operator <<
// Added last_index property
// Added nextIndex method
template <class type> //Vector data type
class vector : public std::vector<type>
{
protected:
	int last_index;

	using move_reference = type&&;
	using reference = type&;
	using pointer = type*;
	using const_reference = const reference;
	using const_pointer = const pointer;
	using size_type = int;
	using value_type = type;

	void setLast(int value = size() - 1)
	{
		last_index = value;
	}
	void canReSize(int pos = size())
	{
		if (pos > capacity())
			resize(pos + 1);
		else if (pos == capacity())
			resize();
	}
	
	// Will not drop below zero
	int tread(int index)
	{
		return ((index + abs(index)) / 2);
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
	//vector(const_reference rItem) : std::vector(rItem) {}
	//vector(type rItem) : std::vector(rItem) {}

	void operator<< (move_reference rItem)
	{
		add(rItem);
	}
	void operator<< (const_reference rItem)
	{
		add(rItem);
	}
	reference operator[] (int index)
	{
		canReSize(index);
		return at(tread(index));
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

	void add(move_reference item)
	{
		push_back(item);
		setLast();
	}
	void add(const_reference item)
	{
		push_back(item);
		setLast();
	}

	//Add item to index 'pos', moving the item there to the end if necessary
	void prefer(int pos, move_reference item)
	{
		canReSize(pos);			// Resize is needed

		pointer spot = at(pos);

		if (pos + 1 < size())	// Check to see if position is filled
		{
			add(*spot);			// Add item in desired place to end,
			*spot = item;		// then assign the item to the desired place
			setLast(pos);
		}
		else
		{
			*spot = item;		// Assign the item to the desired place
			setLast(pos);
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
