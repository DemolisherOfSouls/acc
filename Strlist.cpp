//**************************************************************************
//**
//** strlist.cpp
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "common.h"
#include "strlist.h"
#include "error.h"
#include "misc.h"
#include "pcode.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct StringInfo;
struct LanguageInfo;
using StringList = vector<StringInfo>;
using StringTable = vector<StringList>;
using LangList = vector<LanguageInfo>;

class StringInfo
{
	using List = vector<StringInfo>*;

public:

	string name = "";				// The string itself
	int index = INVALID_INDEX;		// Location in list
	int address = NULL;				// Address when writing pCodes
	List list = NULL;				// Link to stored list

	string operator=(const StringInfo& rValue)
	{
		return rValue.name;
	}
	StringInfo operator=(const string& rValue)
	{
		return StringInfo(rValue);
	}

	// Basic constructor
	StringInfo(string name)
	{
		set(name);
	}
	// Adds itself to the list!
	StringInfo(string name, List list)
	{
		set(name);
		set(list);
		addListItem();
	}
	
	//Public caller
	void addListItem(List list)
	{
		list->add(*this);
		index = (list->lastAdded()).index = list->lastIndex();
	}

protected:

	void addListItem()
	{
		list->add(*this);
		index = (list->lastAdded()).index = list->lastIndex();
	}
};

struct LanguageInfo
{
	string name;
	StringList list;

	LanguageInfo(string name)
	{
		set(name);
		list = StringList();
	}
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int STR_PutStringInSomeList(StringList &list, string name);
static int STR_FindInSomeList(StringList &list, string name);
static int STR_FindInSomeListInsensitive(StringList &list, string name);
static void DumpStrings(StringList &list, int lenadr, bool quad, bool crypt);
static void Encrypt(void *data, int key, int len);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static LangList str_LanguageList = LangList(MAX_LANGUAGES);
static StringTable str_StringStorage = StringTable(NUM_STRLISTS);

// CODE --------------------------------------------------------------------

//==========================================================================
//
// STR_Init
//
//==========================================================================
void STR_Init()
{
	//TODO: verify that using an empty string is ok
	str_LanguageList.add(LanguageInfo(""));	// Default language is always number 0
}

//==========================================================================
//
// STR_FindLanguage
// Returns the index of the language, and creates it if needed
//
//==========================================================================
int STR_FindLanguage(string name)
{
	int i = 0;

	if (name.empty() && NumLanguages > 0)
		return 0;

	else
		for (LanguageInfo &info : str_LanguageList)
		{
			if (info.name.empty())
				break;
			if (name.compare(info.name) == 0)
				return i;
			i++;
		}
	if (i == str_LanguageList.size())
	{
		str_LanguageList.add(LanguageInfo(name));

		if (str_LanguageList.size() > 1)
			pCode_HexenEnforcer();
	}
	return i;
}
 
//==========================================================================
//
// STR_Find
//
//==========================================================================
int STR_Find(string name)
{
	return STR_FindInLanguage(0, name);
}

//==========================================================================
//
// STR_FindInLanguage
//
//==========================================================================
int STR_FindInLanguage(int language, string name)
{
	return STR_FindInSomeList (&str_LanguageList[language].list, name);
}

//==========================================================================
//
// STR_FindInList
//
//==========================================================================
int STR_FindInList(StringListType list, string name)
{
	if (str_StringStorage[list].empty())
	{
		//str_StringStorage[list] = StringList();
		//NumStringLists++;
		pCode_HexenEnforcer();
	}
	return STR_FindInSomeList(&str_StringStorage[list], name);
}

//==========================================================================
//
// STR_FindInSomeList
//
//==========================================================================
static int STR_FindInSomeList(StringList &list, string name)
{
	int i = 0;
	for(StringInfo &info : list)
	{
		if(info.name.compare(name) == 0)
			return i;
		i++;
	}
	// Add to list
	return STR_PutStringInSomeList(list, name);
}

//==========================================================================
//
// STR_FindInListInsensitive
//
//==========================================================================
int STR_FindInListInsensitive(StringListType list, string name)
{
	if (str_StringStorage[list].empty())
	{
		//NumStringLists++;
		pCode_HexenEnforcer();
	}
	return STR_FindInSomeListInsensitive(str_StringStorage[list], name);
}

//==========================================================================
//
// STR_FindInSomeListInsensitive
//
//==========================================================================
static int STR_FindInSomeListInsensitive(StringList &list, string name)
{
	for (auto &s : list)
		if (s.name.compare(name) == 0)
			return s.index;
	
	return STR_PutStringInSomeList(list, name);
}

//==========================================================================
//
// STR_GetString
//
//==========================================================================
string* STR_GetString(StringListType list, int index)
{
	if (str_StringStorage[list].empty())
	{
		return NULL;
	}
	if (index < 0 || index >= str_StringStorage[list].size())
	{
		return NULL;
	}
	return &str_StringStorage[list][index].name;
}

//==========================================================================
//
// STR_AppendToList
//
//==========================================================================
int STR_AppendToList(int list, string name)
{
	if (str_StringStorage[list].empty())
	{
		//NumStringLists++;
		pCode_HexenEnforcer();
	}
	return STR_PutStringInSomeList(str_StringStorage[list], name);
}

//==========================================================================
//
// STR_PutStringInSomeList
//
//==========================================================================
static int STR_PutStringInSomeList(StringList *list, string name)
{
	if(list->size() >= MAX_STRINGS)
	{
		ERR_Error(ERR_TOO_MANY_STRINGS, true, MAX_STRINGS);
		return INVALID_INDEX;
	}

	Message(MSG_DEBUG, "Adding string " + list->size() + string(":"));
	Message(MSG_DEBUG, "  \"" + name + "\"");

	list->add(StringInfo(name));

	return list->lastIndex();
}

//==========================================================================
//
// str_ListSize
//
//==========================================================================
int str_ListSize(int list)
{
	return str_LanguageList[list].list.size();
}

//==========================================================================
//
// STR_WriteStrings
//
// Writes all the strings to the p-code buffer.
//
//==========================================================================
void STR_WriteStrings()
{
	Message(MSG_DEBUG, "---- STR_WriteStrings ----");

	for (StringInfo &item : str_LanguageList[0].list)
	{
		item.address = pCode_current;
		pCode_Append(item.name);
	}

	// Need to align
	if(pCode_current % 4 != 0)
		for (int i = 0; i < 4 - (pCode_current % 4); i++)
			pCode_AppendShrink(0);
}

//==========================================================================
//
// STR_WriteList
//
//==========================================================================
void STR_WriteList()
{
	Message(MSG_DEBUG, "---- STR_WriteList ----");

	pCode_Append(str_ListSize(0));

	for (StringInfo &info : str_LanguageList[0].list)
		pCode_Append(info.address);
}

//==========================================================================
//
// STR_WriteChunk
//
//==========================================================================
void STR_WriteChunk(int language, bool encrypt)
{
	LanguageInfo *lang = &str_LanguageList[language];
	int lenadr;

	Message(MSG_DEBUG, "---- STR_WriteChunk " + string(language) + " ----");
	pCode_Append(encrypt ? "STRE" : "STRL");
	lenadr = pCode_current;
	PC_SkipInt();
	pCode_Append(lang->name.substr(0, 4));
	pCode_Append((int)lang->list.size());
	pCode_Append(0);	// Used in-game for stringing lists together

	DumpStrings (lang->list, lenadr, false, encrypt);
}

//==========================================================================
//
// STR_WriteListChunk
//
//==========================================================================
void STR_WriteListChunk(int list, int id, bool quad)
{
	int lenadr;

	if (!str_StringStorage[list].empty())
	{
		MS_Message(MSG_DEBUG, "---- STR_WriteListChunk %d %c%c%c%c----\n", list,
			id&255, (id>>8)&255, (id>>16)&255, (id>>24)&255);
		pCode_Append(id);
		lenadr = pCode_current;
		pCode_Skip(4);
		pCode_Append((int)str_StringStorage[list].size());
		if (quad && pCode_current % 8 != 0)
		{ // If writing quadword indices, align the indices to an
		  // 8-byte boundary.
			pCode_AppendPadding(8 - pCode_current % 8);
		}
		DumpStrings(str_StringStorage[list], lenadr, quad, false);
	}
}

//==========================================================================
//
// DumpStrings
//
//==========================================================================

static void DumpStrings(stringList_t *list, int lenadr, bool quad, bool crypt)
{
	int i, ofs, startofs;

	startofs = ofs = pc_Address - lenadr - 4 + list->stringCount*(quad?8:4);

	for(i = 0; i < list->stringCount; i++)
	{
		if (list->strings[i].name != NULL)
		{
			PC_AppendInt(ofs);
			ofs += strlen(list->strings[i].name) + 1;
		}
		else
		{
			PC_AppendInt(0);
		}
		if (quad)
		{
			PC_AppendInt(0);
		}
	}

	ofs = startofs;

	for(i = 0; i < list->stringCount; i++)
	{
		if(list->strings[i].name != NULL)
		{
			int stringlen = strlen(list->strings[i].name) + 1;
			if(crypt)
			{
				int cryptkey = ofs*157135;

				Encrypt(list->strings[i].name, cryptkey, stringlen);
				PC_Append(list->strings[i].name, stringlen);
				ofs += stringlen;
				Encrypt(list->strings[i].name, cryptkey, stringlen);
			}
			else
			{
				PC_AppendString(list->strings[i].name);
			}
		}
	}
	if(pc_Address%4 != 0)
	{ // Need to align
		int pad = 0;
		PC_Append((void *)&pad, 4-(pc_Address%4));
	}

	PC_WriteInt(pc_Address - lenadr - 4, lenadr);
}

static void Encrypt(void *data, int key, int len)
{
	int p = (byte)key, i;

	for(i = 0; i < len; ++i)
	{
		((byte *)data)[i] ^= (byte)(p+(i>>1));
	}
}
