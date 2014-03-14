// navigationDlg.h : ヘッダー ファイル
//

#pragma once

#include "urg3D.h"
#include "megaRover.h"
#include "imu.h"
#include "navi.h"
#include "navigationview.h"
#include "obstacleAvoidance.h"
#include "imageProcessing.h"
#include "detectTarget.h"

// CnavigationDlg ダイアログ
class CnavigationDlg : public CDialog
{
// コンストラクション
public:
	CnavigationDlg(CWnd* pParent = NULL);	// 標準コンストラクタ

// ダイアログ データ
	enum { IDD = IDD_NAVIGATION_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート

// 実装
protected:
	HICON m_hIcon;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	urg3D urg3d;
	megaRover mega_rover;
	imu IMU;
	navi navigation;
	obstacleAvoidance obs_avoid;
	imageProcessing ip;
	detectTarget detect_target;

	int is_record;
	int is_play;
	int is_target_ip;
	float coincidence;
	
	int reroute_mode0;

public:

		
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CnavigationView navigationView;
	CString message;
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedCheckLoop();
	BOOL m_check_loop;
};
