/***********************************************************************************
**
** OptionsMacroManager.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef OPTIONSMACROMANAGER_H
#define OPTIONSMACROMANAGER_H
//----------------------------------------------------------------------------------
#include "../BaseQueue.h"
//----------------------------------------------------------------------------------
//!����� ��������� �������� ��� �����
class COptionsMacroManager : public CBaseQueue
{
public:
	COptionsMacroManager();
	virtual ~COptionsMacroManager();

	/*!
	��������� �� ������ ��������
	@return 
	*/
	void LoadFromMacro();
};
//----------------------------------------------------------------------------------
//!������ �� �������� �������� ��� �����
extern COptionsMacroManager g_OptionsMacroManager;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
