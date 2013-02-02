//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "trex.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

bool do_active_commenting;
TRex *c_tr;
//TRex *cpp_tr;

std::string doc_start;
std::string doc_line;
std::string doc_end;

//
// Initialize your plugin data here
// It will be called while plugin loading
void pluginInit(HANDLE hModule)
{
	const TRexChar *error = NULL;
	c_tr = trex_compile("(\\w+)[*]*\\s+[*]*(\\w+)\\s*\\((.*)\\)", &error);
	if(!c_tr)
	{
		::MessageBox(NULL, TEXT("Regular expression compilation failed"), TEXT("DoxyIt"), MB_OK);
	}
	do_active_commenting = true;

	//doc_start = "/**";
	doc_start = "/**************************************************************************************//**";
	doc_line  = " *  ";
	doc_end   = " ******************************************************************************************/";
	//doc_end   = " */";
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
	trex_free(c_tr);
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
	ShortcutKey *sk = new ShortcutKey();
	sk->_isAlt = TRUE;
	sk->_isCtrl = TRUE;
	sk->_isShift = TRUE;
	sk->_key = 'D';

	setCommand(0, TEXT("DoxyIt - Function"), doxyItFunction, sk, false);
	setCommand(1, TEXT("DoxyIt - File"), doxyItFile, NULL, false);
	setCommand(2, TEXT("Active commenting"), activeCommenting, NULL, do_active_commenting);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
	delete funcItem[0]._pShKey;
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
{
	if (index >= nbFunc || !pFunc) return false;

	lstrcpy(funcItem[index]._itemName, cmdName);
	funcItem[index]._pFunc = pFunc;
	funcItem[index]._init2Check = check0nInit;
	funcItem[index]._pShKey = sk;

	return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void doxyItFunction()
{
	char buffer[256];
	int which = -1;
	HWND curScintilla;
	const TRexChar *begin,*end;

	// Get the current scintilla
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1) return;
	curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	
	int curPos = (int) ::SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);
	int curLine = (int) ::SendMessage(curScintilla, SCI_LINEFROMPOSITION, curPos, 0);
	int lineLen = (int) ::SendMessage(curScintilla, SCI_GETLINE, curLine + 1, (LPARAM) buffer);
	buffer[lineLen] = '\0';

	if(trex_search(c_tr, buffer, &begin, &end))
	{
		std::ostringstream doc_block;
		TRexMatch return_match;
		TRexMatch func_match;
		TRexMatch params_match;

		trex_getsubexp(c_tr, 1, &return_match);
		trex_getsubexp(c_tr, 2, &func_match);
		trex_getsubexp(c_tr, 3, &params_match);

		doc_block << doc_start << "\r\n";
		doc_block << doc_line << "\\brief [description]\r\n";
		doc_block << doc_line << "\r\n";
		
		// For each param
		doc_block << doc_line << "\\param [in] ";
		doc_block.write(params_match.begin, params_match.len);
		doc_block << doc_line << " [description]\r\n";

		// Return value
		doc_block << doc_line << "\\return \\em ";
		doc_block.write(return_match.begin, return_match.len); doc_block << "\r\n";
		doc_block << doc_line << "\r\n";

		doc_block << doc_line << "\\revision 1 [date]\r\n";
		doc_block << doc_line << "\\history <b>Rev. 1 [date]</b> [description]\r\n";
		doc_block << doc_line << "\r\n";
		doc_block << doc_line << "\\details [description]\r\n";
		doc_block << doc_end;
		
		::SendMessage(curScintilla, SCI_REPLACESEL, 0, (LPARAM) doc_block.str().c_str());
	}
	else
	{
		::MessageBox(NULL, TEXT("Cannot parse function definition"), TEXT("Error"), MB_OK);
	}
}

void doxyItFile()
{
}

void activeCommenting()
{
	do_active_commenting = !do_active_commenting;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[1]._cmdID, (LPARAM) do_active_commenting);
}
