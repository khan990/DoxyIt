//This file is part of DoxyIt.
//
//Copyright (C)2013 Justin Dailey <dail8859@yahoo.com>
//
//DoxyIt is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <WindowsX.h>
#include "SettingsDialog.h"
#include "Parsers.h"
#include "PluginDefinition.h"

const TCHAR *msg = TEXT("An option is blank (or all whitespace). If this is desired, it is recommended that you disable Active Commenting! Continue anyways?");

void SettingsDialog::init(HINSTANCE hInst, NppData nppData)
{
	m_nppData = nppData;
	Window::init(hInst, nppData._nppHandle);
}

void SettingsDialog::doDialog()
{
	if (!isCreated()) create(IDD_SETTINGSDLG);
	goToCenter();

	initParserDefinitions();
	loadParserDefinition();
	updatePreview();
}

void SettingsDialog::initParserDefinitions()
{
	int len = sizeof(parsers) / sizeof(parsers[0]);

	parserDefinitions.clear();
	for(int i = 0; i < len; ++i)
		parserDefinitions[parsers[i].language_name] = parsers[i].pd;
}

void SettingsDialog::saveParserDefinition(int index)
{
	TCHAR prev_name[32];
	TCHAR text[256]; // Edit_LimitText is used to limit to 255 chars
	TCHAR *dtext;
	int len;

	// Save the text from the edit controls for the previous selection
	ComboBox_GetLBText(GetDlgItem(_hSelf, IDC_CMB_LANG), index, prev_name);
	ParserDefinition *prev_pd = &parserDefinitions[prev_name];

	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_START), text, 256);
	prev_pd->doc_start = toString(text);

	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_LINE), text, 256);
	prev_pd->doc_line = toString(text);

	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_END), text, 256);
	prev_pd->doc_end = toString(text);

	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_PREFIX), text, 256);
	prev_pd->command_prefix = toString(text);

	len = Edit_GetTextLength(GetDlgItem(_hSelf, IDC_EDIT_FORMAT)) + 1;
	dtext = new TCHAR[len];
	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), dtext, len);
	if(Button_GetCheck(GetDlgItem(_hSelf, IDC_RAD_FUNCTION)) == BST_CHECKED)
		prev_pd->function_format = toString(dtext);
	else
		prev_pd->file_format = toString(dtext);
	delete[] dtext;

	prev_pd->align = (Button_GetCheck(GetDlgItem(_hSelf, IDC_CHB_ALIGN)) == BST_CHECKED ? true : false);
}

// Note: Setting the text of edit boxes causes notifications to be generated, which update the 
// preview multiple times when calling loadParserDefinition(). The 'updating' flag is used to 
// temporarily ignore notifications.
void SettingsDialog::loadParserDefinition()
{
	TCHAR name[32];

	// Load the edit controls with the new parsers settings
	ComboBox_GetText(GetDlgItem(_hSelf, IDC_CMB_LANG), name, 32);
	ParserDefinition pd = parserDefinitions[name];

	m_updating = true;
	Button_SetCheck(GetDlgItem(_hSelf, IDC_CHB_ALIGN), pd.align); // Cannot be last!  Doesn't update preview
	Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_START), toWideString(pd.doc_start).c_str());
	Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_LINE), toWideString(pd.doc_line).c_str());
	Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_END), toWideString(pd.doc_end).c_str());
	Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_PREFIX), toWideString(pd.command_prefix).c_str());
	if(Button_GetCheck(GetDlgItem(_hSelf, IDC_RAD_FUNCTION)) == BST_CHECKED)
		Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), toWideString(pd.function_format).c_str());
	else
		Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), toWideString(pd.file_format).c_str());
	m_updating = false;
}

bool SettingsDialog::validateText(std::string text, int idc)
{
	if(text.length() != 0 && !isWhiteSpace(text))
		return true;

	SetFocus(GetDlgItem(_hSelf, idc));
	return false;
}
bool SettingsDialog::validateSettings()
{
	int len = sizeof(parsers) / sizeof(parsers[0]);
	for(int i = 0; i < len; ++i)
	{
		bool ret = true;
		const ParserDefinition *pd = &parserDefinitions[parsers[i].language_name];

		if(!validateText(pd->doc_start, IDC_EDIT_START)) ret = false;
		if(!validateText(pd->doc_line, IDC_EDIT_LINE)) ret = false;
		if(!validateText(pd->doc_end, IDC_EDIT_END)) ret = false;
		if(!validateText(pd->command_prefix, IDC_EDIT_PREFIX)) ret = false;

		if(!ret)
		{
			ComboBox_SelectString(GetDlgItem(_hSelf, IDC_CMB_LANG), -1, parsers[i].language_name.c_str());
			loadParserDefinition();
			return false;
		}
	}

	return true;
}

void SettingsDialog::saveSettings()
{
	int len = sizeof(parsers) / sizeof(parsers[0]);
	for(int i = 0; i < len; ++i)
		parsers[i].pd = parserDefinitions[parsers[i].language_name];
}

void SettingsDialog::updatePreview()
{
	std::string block;
	ParserDefinition *pd;
	TCHAR name[32];
	int prev_eol_mode;

	// Get the name of the language that is selected
	ComboBox_GetText(GetDlgItem(_hSelf, IDC_CMB_LANG), name, 32);
	pd = &parserDefinitions[name];

	// Set eol mode to "\r\n" so it will display correctly in the dialogbox
	prev_eol_mode = SendScintilla(SCI_GETEOLMODE);
	SendScintilla(SCI_SETEOLMODE, SC_EOL_CRLF);

	if(Button_GetCheck(GetDlgItem(_hSelf, IDC_RAD_FUNCTION)) == BST_CHECKED)
	{
		// Get the parser and have it parse the example
		const Parser *p = getParserByName(name);
		block = FormatFunctionBlock(p, pd, p->example.c_str());
		block += "\r\n" + p->example;
	}
	else // IDC_RAD_FILE is set
	{
		block = FormatFileBlock(pd);
	}

	// Set the preview
	Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_PREVIEW), toWideString(block).c_str());

	// Restore the eol mode
	SendScintilla(SCI_SETEOLMODE, prev_eol_mode);
}

void SettingsDialog::swapFormat()
{
	TCHAR name[32];
	TCHAR *text;
	unsigned int len;
	ParserDefinition *pd;
	static int last_rad = IDC_RAD_FUNCTION; // In case the selected radio button is clicked again
	
	// Get the parserDefinition
	ComboBox_GetText(GetDlgItem(_hSelf, IDC_CMB_LANG), name, 32);
	pd = &parserDefinitions[name];

	// Get the text length, dynamically allocate it
	len = Edit_GetTextLength(GetDlgItem(_hSelf, IDC_EDIT_FORMAT)) + 1;
	text = new TCHAR[len];
	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), text, len);

	if(Button_GetCheck(GetDlgItem(_hSelf, IDC_RAD_FUNCTION)) == BST_CHECKED && last_rad == IDC_RAD_FILE)
	{
		pd->file_format = toString(text);
		Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), toWideString(pd->function_format).c_str());
		last_rad = IDC_RAD_FUNCTION;
	}
	else if(Button_GetCheck(GetDlgItem(_hSelf, IDC_RAD_FILE)) == BST_CHECKED && last_rad == IDC_RAD_FUNCTION)
	{
		pd->function_format = toString(text);
		Edit_SetText(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), toWideString(pd->file_format).c_str());
		last_rad = IDC_RAD_FILE;
	}

	delete[] text;
}

BOOL CALLBACK SettingsDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(m_updating) return true;

	switch(message) 
	{
	case WM_INITDIALOG:
		{
			HWND cmb = GetDlgItem(_hSelf, IDC_CMB_LANG);
			long lfHeight = -MulDiv(10, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
			int len = sizeof(parsers) / sizeof(parsers[0]);

			for(int i = 0; i < len; ++i)
				ComboBox_AddString(cmb, parsers[i].language_name.c_str());
			ComboBox_SetCurSel(cmb, m_previousSelection);
			
			// I have no idea what these values do, but these work
			m_monoFont = CreateFont(lfHeight,0,0,0,0,0,0,0,0,0,0,0,0,TEXT("Courier New"));
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_START), m_monoFont, false);
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_LINE), m_monoFont, false);
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_END), m_monoFont, false);
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_PREFIX), m_monoFont, false);
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_PREVIEW), m_monoFont, false);
			SetWindowFont(GetDlgItem(_hSelf, IDC_EDIT_FORMAT), m_monoFont, false);

			// Limit the input boxes
			Edit_LimitText(GetDlgItem(_hSelf, IDC_EDIT_START), 255);
			Edit_LimitText(GetDlgItem(_hSelf, IDC_EDIT_LINE), 255);
			Edit_LimitText(GetDlgItem(_hSelf, IDC_EDIT_END), 255);
			Edit_LimitText(GetDlgItem(_hSelf, IDC_EDIT_PREFIX), 1);

			Button_SetCheck(GetDlgItem(_hSelf, IDC_RAD_FUNCTION), BST_CHECKED);

			return true;
		}
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			saveParserDefinition(m_previousSelection);
			loadParserDefinition();
			updatePreview();
			m_previousSelection = ComboBox_GetCurSel(GetDlgItem(_hSelf, IDC_CMB_LANG));
			return true;
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case IDOK:
				m_previousSelection = ComboBox_GetCurSel(GetDlgItem(_hSelf, IDC_CMB_LANG));
				saveParserDefinition(m_previousSelection);
				if(validateSettings())
				{
					saveSettings();
					display(false);
				}
				else if(MessageBox(_hSelf, msg, NPP_PLUGIN_NAME, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
				{
					saveSettings();
					display(false);
				}
				return true;
			case IDCANCEL:
				display(false);
				return true;
			case IDC_CHB_ALIGN:
				saveParserDefinition(ComboBox_GetCurSel(GetDlgItem(_hSelf, IDC_CMB_LANG)));
				updatePreview();
				return true;
			case IDC_RAD_FUNCTION:
			case IDC_RAD_FILE:
				swapFormat();
				updatePreview();
				return true;
			}
		case EN_CHANGE:
			saveParserDefinition(ComboBox_GetCurSel(GetDlgItem(_hSelf, IDC_CMB_LANG)));
			updatePreview();
			return true;
		case EN_MAXTEXT:
			MessageBeep(MB_ICONERROR);
			return true;
		}
	default:
		return StaticDialog::dlgProc(m_HSource, message, wParam, lParam);
	}

	return false;
}
