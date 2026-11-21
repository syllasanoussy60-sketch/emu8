
// emu8gui.h : fichier d'en-tête principal de l'application PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "incluez 'pch.h' avant d'inclure ce fichier pour PCH"
#endif

#include "resource.h"		// symboles principaux


// Cemu8guiApp :
// Consultez emu8gui.cpp pour l'implémentation de cette classe
//

class Cemu8guiApp : public CWinApp
{
public:
	Cemu8guiApp();

// Substitutions
public:
	virtual BOOL InitInstance();

// Implémentation

	DECLARE_MESSAGE_MAP()
};

extern Cemu8guiApp theApp;
