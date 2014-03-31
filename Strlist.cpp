
//**************************************************************************
//**
//** strlist.c
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

struct StringInfo
{
	using List = StringList;

	string name;
	int index;
	List &list;

	string operator=(const StringInfo& rValue)
	{
		return rValue.name;
	}
	StringInfo operator=(const string& rValue)
	{
		return StringInfo(rValue);
	}

	StringInfo(string name)
	{
		set(name);

	}
	StringInfo(string name, List &list)
	{
		set(name);
		set(list);
		addListItem();
	}
	StringInfo(string name, int index)
	{
		set(name);
		set(index);
	}

protected:

	void addListItem()
	{
		list.add((StringInfo)this);
		index = ((StringInfo)list.lastAdded()).index = list.lastIndex();
	}
};

struct LanguageInfo
{
	string name;
	StringList list;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int STR_PutStringInSomeList(StringList *list, int index, string name);
static int STR_FindInSomeList(StringList *list, string name);
static int STR_FindInSomeListInsensitive(StringList *list, string name);
static void DumpStrings(StringList *list, int lenadr, bool quad, bool crypt);
static void Encrypt(void *data, int key, int len);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static languageInfo *LanguageInfo[MAX_LANGUAGES];
static StringList *StringLists[NUM_STRLISTS];

// CODE --------------------------------------------------------------------

//==========================================================================
//
// STR_Init
//
//==========================================================================
void STR_Init()
{
	STR_FindLanguage("");	// Default language is always number 0
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
		for each (languageInfo_t *info in LanguageInfo)
		{
			if (info = NULL)
				break;
			if (name.compare(info->name) == 0)
				return i;
			i++;
		}
	if (i == NumLanguages)
	{
		LanguageInfo[i] = new languageInfo_t(name, stringList_t());
		LanguageInfo[i]->name = name;
		LanguageInfo[i]->list.stringCount = 0;
		NumLanguages++;
		if (NumLanguages > 1 && pCode_EnforceHexen)
			ERR_Error(ERR_HEXEN_COMPAT, true);
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
	return STR_FindInSomeList (&LanguageInfo[language]->list, name);
}

//==========================================================================
//
// STR_FindInList
//
//==========================================================================
int STR_FindInList(int list, string name)
{
	if (StringLists[list] == NULL)
	{
		StringLists[list] = new stringList_t;
		StringLists[list]->stringCount = 0;
		NumStringLists++;
		if(pc_EnforceHexen)
		{
			ERR_Error(ERR_HEXEN_COMPAT, true);
		}
	}
	return STR_FindInSomeList (StringLists[list], name);
}

//==========================================================================
//
// STR_FindInSomeList
//
//==========================================================================
static int STR_FindInSomeList(stringList_t *list, string name)
{
	int i = 0;
	for each(stringInfo_t info in list->strings)
	{
		if(info.name.compare(name) == 0)
			return i;
		i++;
	}
	// Add to list
	return STR_PutStringInSomeList(list, i, name);
}

//==========================================================================
//
// STR_FindInListInsensitive
//
//==========================================================================
int STR_FindInListInsensitive(int list, char *name)
{
	if (StringLists[list] == NULL)
	{
		StringLists[list] = new stringList_t;
		StringLists[list]->stringCount = 0;
		NumStringLists++;
		if(pc_EnforceHexen)
		{
			ERR_Error(ERR_HEXEN_COMPAT, true);
		}
	}
	return STR_FindInSomeListInsensitive (StringLists[list], name);
}

//==========================================================================
//
// STR_FindInSomeListInsensitive
//
//==========================================================================

static int STR_FindInSomeListInsensitive(StringList &list, string name)
{
	for (auto s : list)
		if (s.compare(name) == 0)
			return i;

	for(int i = 0; i < list.size(); i++)
		if(list[i].compare(name) == 0)
			return i;
	
	return STR_PutStringInSomeList(list, i, name);
}

//==========================================================================
//
// STR_GetString
//
//==========================================================================

const char *STR_GetString(int list, int index)
{
	if (StringLists[list] == NULL)
	{
		return NULL;
	}
	if (index < 0 || index >= StringLists[list]->stringCount)
	{
		return NULL;
	}
	return StringLists[list]->strings[index].name;
}

//==========================================================================
//
// STR_AppendToList
//
//==========================================================================

int STR_AppendToList(int list, string name)
{
	if (StringLists[list] == NULL)
	{
		StringLists[list] = new stringList_t;
		StringLists[list]->stringCount = 0;
		NumStringLists++;
		if(pc_EnforceHexen)
		{
			ERR_Error(ERR_HEXEN_COMPAT, true);
		}
	}
	return STR_PutStringInSomeList(StringLists[list], StringLists[list]->stringCount, name);
}

//==========================================================================
//
// STR_PutStringInSomeList
//
//==========================================================================

static int STR_PutStringInSomeList(stringList_t *list, int index, string name)
{
	if(index >= MAX_STRINGS)
	{
		ERR_Error(ERR_TOO_MANY_STRINGS, true, MAX_STRINGS);
		return 0;
	}

	MS_Message(MSG_DEBUG, "Adding string %d:\n  \"%s\"\n", list->stringCount, name);

	if(index >= list->stringCount)
	{
		for each(stringInfo_t info in list->strings)
			info.name = "";
		list->stringCount = index + 1;
	}

	if(!name.empty)
		list->strings[index].name = name;
	else
		list->strings[index].name.clear();

	return index;
}

//==========================================================================
//
// STR_ListSize
//
//==========================================================================

int STR_ListSize(int list)
{
	return LanguageInfo[list]->list.stringCount;
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
	int i;
	int pad;

	MS_Message(MSG_DEBUG, "---- STR_WriteStrings ----\n");
	for(i = 0; i < LanguageInfo[0]->list.stringCount; i++)
	{
		LanguageInfo[0]->list.strings[i].address = pc_Address;
		PC_AppendString(LanguageInfo[0]->list.strings[i].name);
	}
	if(pc_Address%4 != 0)
	{ // Need to align
		pad = 0;
		PC_Append((void *)&pad, 4-(pc_Address%4));
	}
}

//==========================================================================
//
// STR_WriteList
//
//==========================================================================

void STR_WriteList()
{
	int i;

	MS_Message(MSG_DEBUG, "---- STR_WriteList ----\n");
	PC_AppendInt(LanguageInfo[0]->list.stringCount);
	for(i = 0; i < LanguageInfo[0]->list.stringCount; i++)
	{
		PC_AppendInt(LanguageInfo[0]->list.strings[i].address);
	}
}

//==========================================================================
//
// STR_WriteChunk
//
//==========================================================================

void STR_WriteChunk(int language, bool encrypt)
{
	languageInfo_t *lang = LanguageInfo[language];
	int lenadr;

	MS_Message(MSG_DEBUG, "---- STR_WriteChunk %d ----\n", language);
	PC_Append(encrypt ? "STRE" : "STRL", 4);
	lenadr = pc_Address;
	PC_SkipInt();
	PC_Append(&lang->name, 4);
	PC_AppendInt(lang->list.stringCount);
	PC_AppendInt(0);	// Used in-game for stringing lists together

	DumpStrings (&lang->list, lenadr, false, encrypt);
}

//==========================================================================
//
// STR_WriteListChunk
//
//==========================================================================

void STR_WriteListChunk(int list, int id, bool quad)
{
	int lenadr;

	if (StringLists[list] != NULL && StringLists[list]->stringCount > 0)
	{
		MS_Message(MSG_DEBUG, "---- STR_WriteListChunk %d %c%c%c%c----\n", list,
			id&255, (id>>8)&255, (id>>16)&255, (id>>24)&255);
		PC_AppendInt(id);
		lenadr = pc_Address;
		PC_SkipInt();
		PC_AppendInt(StringLists[list]->stringCount);
		if (quad && pc_Address%8 != 0)
		{ // If writing quadword indices, align the indices to an
		  // 8-byte boundary.
			int pad = 0;
			PC_Append (&pad, 4);
		}
		DumpStrings(StringLists[list], lenadr, quad, false);
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
