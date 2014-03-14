
// navigation.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CnavigationApp:
// このクラスの実装については、navigation.cpp を参照してください。
//

class CnavigationApp : public CWinAppEx
{
public:
	CnavigationApp();

// オーバーライド
	public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CnavigationApp theApp;