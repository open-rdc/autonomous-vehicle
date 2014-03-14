/*!
 * @file  navigationDlg.cpp
 * @brief ダイアログクラス
 * @date 2013.10.31
 * @author Y.Hayashibara
 *
 * 全体を統括する最も基本的なクラス
 * OnInitialize()で初期化
 * OnTimer()で周期的な処理
 * OnClose()で終了処理
 */

#include "stdafx.h"
#include <mmsystem.h>
#include <math.h>
#include "navigation.h"
#include "navigationDlg.h"
#include "logger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define	M_PI	3.14159f

#define USE_URG3D
#define USE_MEGA_ROVER
#define USE_IMU
#define USE_CAMERA

// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

/*!
 * @class CnavigationDlg
 * @brief ダイアログのクラス
 * @author Y.Hayashibara
 */

// CnavigationDlg ダイアログ

CnavigationDlg::CnavigationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CnavigationDlg::IDD, pParent)
	, message(_T("")), is_record(0), is_play(0)
	, is_target_ip(0), coincidence(0)
	, m_check_loop(FALSE), reroute_mode0(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CnavigationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PAINT, navigationView);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, message);
	DDX_Check(pDX, IDC_CHECK_LOOP, m_check_loop);
}

BEGIN_MESSAGE_MAP(CnavigationDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CnavigationDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CnavigationDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_CHECK_LOOP, &CnavigationDlg::OnBnClickedCheckLoop)
END_MESSAGE_MAP()


// CnavigationDlg メッセージ ハンドラ

/*!
 * @brief 初期化
 *
 * @return TRUE
 */
BOOL CnavigationDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	ShowWindow(SW_SHOW);

#ifdef _DEBUG
	char s[100];
	time_t timer = time(NULL);
	struct tm *date = localtime(&timer);
	sprintf(s, "log%04d%02d%02d%02d%02d.txt", date->tm_year+1900, date->tm_mon+1, date->tm_mday, date->tm_hour, date->tm_min);

	logger::Init(s);
	LOG("START\n");
#endif

	timeBeginPeriod(1);
#ifdef USE_URG3D
	urg3d.Init();
	urg3d.SetTiltAngle(0, 45, 1);
#endif
#ifdef USE_MEGA_ROVER 
	if (mega_rover.init()) AfxMessageBox("Connect Mega Rover");
	mega_rover.servoOn(30);			// Gainを設定
	mega_rover.setSpeedControlMode(1);
	mega_rover.setDelta(20,20);		// 加減速の程度
#endif
#ifdef USE_IMU
	IMU.Init(IMU_COM_PORT);
#endif
#ifdef USE_CAMERA
	ip.init();
#endif
	navigation.Init();
	obs_avoid.Init();
	detect_target.Init();

	SetTimer(1, 100, NULL);
	
	return TRUE;					// フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CnavigationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CnavigationDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);			// 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CnavigationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/*!
 * @brief ウィンドウを閉じるときのハンドラ
 */
void CnavigationDlg::OnClose()
{
#ifdef USE_URG3D
	urg3d.Close();
#endif
#ifdef USE_MEGA_ROVER
	mega_rover.close();
#endif
#ifdef USE_IMU
	IMU.Close();
#endif
	navigation.Close();
	obs_avoid.Close();
	detect_target.Close();

	timeEndPeriod(1);

#ifdef _DEBUG
	logger::Close();
#endif
	CDialog::OnClose();
}

/*!
 * @brief 一定周期(100ms)で呼び出されるハンドラ
 * あまり正確な周期では呼び出されない
 */
void CnavigationDlg::OnTimer(UINT_PTR nIDEvent)
{
	// 位置の補正に用いるデータの範囲（ロボット座標）
	static const int search_x0 =      0, search_x1 = 14000;			//! 前後方向の探索範囲(mm)	【ToDo: もっと探索範囲を広げた方が良いのではないか？】
	static const int search_y0 = -14000, search_y1 = 14000;			//! 左右方向の探索範囲(mm)

	static const int search_z0     = 1800, search_z1     = 1900;	//! 上下方向の探索範囲(mm) 屋内用
//	static const int search_z0     = 1900, search_z1     = 2000;	//! 上下方向の探索範囲(mm) 屋外用
	static const int search_obs_z0 =    0, search_obs_z1 = 1000;	//! 障害物を探す上下方向の探索範囲(mm)　URGの高さが基準　【ToDo: 上まで探索したほうが良いのではないか？】
	static const int search_tar_z0 =  200, search_tar_z1 = 400;		//! ターゲットを探す上下方向の探索範囲(mm)　URGの高さが基準
	static const int intensity_thre = 7000;							//! 反射強度のしきい値(単位なし)

	static int is_first = 1;
	float odoX = 0, odoY = 0, odoThe = 0;							//! オドメトリの位置と角度(m, rad)
	float estX = 0, estY = 0, estThe = 0;							//! 推定した位置と角度(m, rad)
	float joyX, joyY;
	int button;
#ifdef MEGA_ROVER_1_1
	static const float f_gain = 0.5f;								//! ジョイスティックを前後に最大限倒した時の速度目標(m/s)
	static const float t_gain = 0.5f;								//! ジョイスティックを左右に最大限倒した時にホイールが逆に回転する速度目標(m/s)
#else
	static const float f_gain = 0.3f;								//! ジョイスティックを前後に最大限倒した時の速度目標(m/s)
	static const float t_gain = 1.0f;								//! ジョイスティックを左右に最大限倒した時にホイールが逆に回転する速度目標(m/s)
#endif
	float target_ip_cf = 0.0f;

#ifdef USE_CAMERA
	is_target_ip = ip.checkTarget(& target_ip_cf);					//! 探索対象かどうかのチェック(true:探索対象，false:探索対象でない）
#endif

#ifdef USE_MEGA_ROVER
	// パーティクルの表示
	static const int MAX_PARTICLE_NUM = 1000;						//! 最大1000個表示
	static struct particle_T particle[MAX_PARTICLE_NUM];			//! パーティクルの配列
	int particle_num;												//! 現在使用しているパーティクル

	float rightSpeed, leftSpeed;
	mega_rover.getSpeed(&rightSpeed, &leftSpeed);
	LOG("rightSpeed:%f, leftSpeed%f\n", rightSpeed, leftSpeed);
	mega_rover.getOdometory(&odoX, &odoY, &odoThe, 0);			// オドメトリを取得
	LOG("odometory:(%f,%f,%f)\n", odoX, odoY, odoThe);
	estX = odoX, estY = odoY, estThe = odoThe;					// パーティクルフイルタを使用していないときは，推定位置とオドメトリは一緒
	mega_rover.getJoyStick(&joyX, &joyY, &button);				// ジョイスティックの値を取得
	if (fabs(joyX) < 0.1f) joyX = 0.0f;
	if (fabs(joyY) < 0.1f) joyY = 0.0f;
	LOG("joystick:(%f,%f),button:%d\n", joyX, joyY, button);
	{
		char temp[256];
		sprintf(temp, "joystick %d", button);
		message = temp;
		UpdateData(FALSE);
	}

	if (!is_record){											// 教示モードでない時に
		if (button & (DOWN_BUTTON | RIGHT_BUTTON | LEFT_BUTTON)){	// ボタン１を押したら
			is_play = 0;										// 停止
			mega_rover.setMotor(0,0);
		} else if (button & UP_BUTTON){							// ボタン２を押したら
			if (!navigation.isPlayMode()) OnBnClickedButtonPlay();
			is_play = 1;										// 再生
		}
	}
	if (navigation.setOdometory(odoX, odoY, odoThe)){			// ナビゲーションにオドメトリを入力
		LOG("goal\n");
		if (m_check_loop){										// 周回する場合
			urg3d.ClearData();
			navigation.Init();									// 各パラメータをリセットする．
			obs_avoid.Init();
			mega_rover.setMotor(0,0);							// モータ停止
			mega_rover.clearOdometory();						// オドメトリのクリア
		} else {												// 周回しない場合は，再生モードを停止して停止する．
			is_play = 0;
			mega_rover.setMotor(0,0);
		}
	};
	LOG("is_play:%d, is_record:%d\n", is_play, is_record);
	if (is_play){												// 自律走行
		float front, radius;
		float tarX, tarY, tarThe, period;
		
		navigation.getEstimatedPosition(&estX, &estY, &estThe);	// 推定位置の取得
		LOG("estimatePosition:(%f,%f,%f)\n", estX, estY, estThe);
		navigation.getTargetPosition(&tarX, &tarY, &tarThe, &period);
																// 目標位置の取得
		LOG("targetPosition:(%f,%f,%f)\n", tarX, tarY, tarThe);
		float slowDownFactor = obs_avoid.getSlowDownFactor();	// 障害物があったときの減速する比率を取得
		navigation.setNeedStop(obs_avoid.isNeedStop());
		
		if (reroute_mode0 && (!navigation.isRerouteMode())){
			obs_avoid.finishReroute();
			reroute_mode0 = 0;
		}

		if (obs_avoid.isReroute()){
			float forward, rotate;
			if (!reroute_mode0){
				navigation.setRerouteMode(obs_avoid.getRerouteDirection());
				reroute_mode0 = 1;
			}
			navigation.getSpeed(&forward, &rotate);
			mega_rover.setSpeed(forward * slowDownFactor, rotate);
			LOG("reroute_mode, forward;%f, rotate:%f, slowDownFactor:%f\n", forward, rotate, slowDownFactor);
		} else if (navigation.isSearchMode()){
			float forward, rotate;
			navigation.getSpeed(&forward, &rotate);
			mega_rover.setSpeed(forward * slowDownFactor, rotate);
			LOG("search_mode, forward;%f, rotate:%f, slowDownFactor:%f\n", forward, rotate, slowDownFactor);
		} else {
			if (slowDownFactor < 1.0f) PlaySound("obstacle.wav", NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
			navigation.getTargetArcSpeed(&front, &radius);		// 目標に移動する速度と半径
			mega_rover.setArcSpeed(front * slowDownFactor, radius);
			LOG("target_arc_speed, front;%f, radius:%f, slowDownFactor:%f\n", front, radius, slowDownFactor);
		}

		navigationView.setOdometory(estX, estY, estThe);		// 推定位置の入力
		navigationView.setTargetPos(tarX, tarY, tarThe);		// 目標位置の入力
		
		// パーティクルの取得と表示
		navigation.getParticle(particle, &particle_num, MAX_PARTICLE_NUM);
		LOG("\n");
		for(int i = 0;i < particle_num; i ++){
			LOG_WITHOUT_TIME("particle:(%f,%f,%f), eval:%d\n", particle[i].x, particle[i].y, particle[i].the, particle[i].eval);
		}
		navigationView.setParticle(particle, particle_num);		// パーティクルの表示
		
		// 一致度の取得と表示
		navigation.getCoincidence(&coincidence);
		navigationView.setCoincidence(coincidence);
	} else {													// 再生モードでない場合
		mega_rover.setSpeed(f_gain * joyY, t_gain * joyX);
		navigationView.setOdometory(odoX, odoY, odoThe);
	}
	navigationView.setStep(navigation.getStep());				// waypointの数の表示

#endif

#ifdef USE_URG3D
	// URG3Dから受信するデータ
	static const int MAX_DATA = 10000;
	static pos p[MAX_DATA];
	int num = 0;

	// URG3Dから受信する障害物のデータ
	static const int MAX_OBS_DATA = 10000;
	static pos op[MAX_OBS_DATA];
	int obs_num = 0;

	// URG3Dから受信するターゲットのデータ
	static const int MAX_TAR_DATA = 10000;
	static pos_inten tp[MAX_TAR_DATA];
	int tar_num = 0;

	// グローバル座標でのターゲットのデータ
	static pos_inten global_tp[MAX_TAR_DATA];

	// 表示用のデータ（追加していく）
	static const int MAX_DRAW_POS = 10000;
	static pos drawPos[MAX_DRAW_POS];
	static int draw_no = 0, is_max = 0;

	// 反射強度表示用のデータ（追加していく）
	static const int MAX_DRAW_INTEN_POS = 10000;
	static pos_inten drawIntenPos[MAX_DRAW_INTEN_POS];
	static int inten_draw_no = 0, is_max_inten = 0;

	// 探索対象の候補のデータ
	static const int MAX_SLATE_POINT = 100;
	static pos_slate slatePoint[MAX_SLATE_POINT];
	static int slate_num = 0;

	// ナビゲーションに送信するデータ
	static const int MAX_NAVI_POS = 10000;
	static pos naviPos[MAX_NAVI_POS];
	int navi_num = 0;

	// 参照データ
	static const int MAX_REF_DATA = 10000;
	static pos rp[MAX_REF_DATA];
	int ref_num = 100;
	
	// URG3Dからデータを取得
	urg3d.Get3SelectedData(search_z0, search_z1, p, &num, MAX_DATA,
		search_obs_z0, search_obs_z1, op, &obs_num, MAX_OBS_DATA,
		search_tar_z0, search_tar_z1, tp, &tar_num, MAX_TAR_DATA, intensity_thre);
	
	// world座標系に変換 (search_x,yで領域を制限)
	LOG("\n");
	for(int i = 0; i < num; i ++){
		if ((p[i].x < search_x0)||(p[i].x > search_x1)||
			(p[i].y < search_y0)||(p[i].y > search_y1)) continue;
		int x = (int)(p[i].x * cos(odoThe) - p[i].y * sin(odoThe) + odoX * 1000.0f);		// ナビゲーションにはオドメトリベースの連続的なデータを送る
		int y = (int)(p[i].x * sin(odoThe) + p[i].y * cos(odoThe) + odoY * 1000.0f);
		naviPos[navi_num].x = x, naviPos[navi_num].y = y, naviPos[navi_num].z = p[i].z;
		navi_num ++;
		
		x = (int)(p[i].x * cos(estThe) - p[i].y * sin(estThe) + estX * 1000.0f);			// 表示には推定位置を基準とした値を用いる
		y = (int)(p[i].x * sin(estThe) + p[i].y * cos(estThe) + estY * 1000.0f);			// 推定していないときは，オドメトリと同じにする．
		LOG_WITHOUT_TIME("urg_global_pos:(%d,%d,%d)\n", x, y, p[i].z);
		drawPos[draw_no].x = x, drawPos[draw_no].y = y, drawPos[draw_no].z = p[i].z;
		draw_no ++;
		if (draw_no >= MAX_DRAW_POS) is_max = 1, draw_no = 0;
	}

	navigation.setData(naviPos, navi_num);													// ナビゲーションにデータを登録

	if (is_max) navigationView.setData(drawPos, MAX_DRAW_POS);								// データの描画
	else        navigationView.setData(drawPos, draw_no     );								// データの描画

	// world座標系に変換 (search_x,yで領域を制限)
	LOG("\n");
	for(int i = 0; i < tar_num; i ++){		
		int x = (int)(tp[i].pos.x * cos(estThe) - tp[i].pos.y * sin(estThe) + estX * 1000.0f);	// 表示には推定位置を基準とした値を用いる
		int y = (int)(tp[i].pos.x * sin(estThe) + tp[i].pos.y * cos(estThe) + estY * 1000.0f);	// 推定していないときは，オドメトリと同じにする．
		LOG_WITHOUT_TIME("intensity_global_pos:(%d,%d,%d),intensity:%d\n", x, y, tp[i].pos.z, tp[i].intensity);
		global_tp[i].pos.x = x, global_tp[i].pos.y = y, global_tp[i].pos.z = tp[i].pos.z;
		global_tp[i].intensity = tp[i].intensity;

		drawIntenPos[inten_draw_no].pos.x = x, drawIntenPos[inten_draw_no].pos.y = y, drawIntenPos[inten_draw_no].pos.z = tp[i].pos.z;
		drawIntenPos[inten_draw_no].intensity = tp[i].intensity;
		inten_draw_no ++;
		if (inten_draw_no >= MAX_DRAW_INTEN_POS) is_max_inten = 1, inten_draw_no = 0;
	}
	detect_target.addIntensityData(global_tp, tar_num);										// しきい値以上の反射強度データをターゲット検出のクラスに設定
	detect_target.getTargetPos(slatePoint, &slate_num);										// 探索対象者の候補点を取得
	{
		const float SEARCH_RADIUS = 5.0f;
		pos search_pos, self_pos;
		self_pos.x = (int)(estX * 1000), self_pos.y = (int)(estY * 1000), self_pos.z = 0;

		if (detect_target.getSearchPos(&search_pos, self_pos, SEARCH_RADIUS)){
			if (navigation.distaceFromPreviousSearchPoint() > SEARCH_RADIUS) navigation.setSearchPoint(search_pos);			// 寄り道をするようにnaviクラスに位置を設定
		}
	}
	navigationView.setSlatePoint(slatePoint, slate_num);									// 探索対象者の候補点を描画するように設定
	LOG("\n");
	for(int i = 0; i < slate_num; i ++){		
		LOG_WITHOUT_TIME("slate_point:(%d,%d,%d),probability:%f\n", slatePoint[i].pos.x, slatePoint[i].pos.y, slatePoint[i].pos.z, slatePoint[i].probability);
	}

	if (is_max_inten) navigationView.setIntensityData(drawIntenPos, MAX_DRAW_INTEN_POS);	// データの描画
	else			  navigationView.setIntensityData(drawIntenPos, inten_draw_no     );	// データの描画

	navigation.getRefData(rp, &ref_num, MAX_REF_DATA);										// リファレンスデータの表示
	navigationView.setRefData(rp, ref_num);

	obs_avoid.setData(op, obs_num);															// 障害物データの代入
#endif

#ifdef USE_IMU
	float xt = 0, yt = 0, zt = 0;

	IMU.GetAngleStart();
	if (IMU.GetAngle(&xt, &yt, &zt) == 0){
		mega_rover.setOdometoryAngle(zt * M_PI / 180.0f);

/*		{
			char temp[256];
			sprintf(temp, "angle xt:%f, yt:%f, zt:%f", xt, yt, zt);
			message = temp;
			UpdateData(FALSE);
		}*/
	}
#endif
	is_first = 0;
	CDialog::OnTimer(nIDEvent);
}

/*!
 * @brief [Record]ボタンを押した時のハンドラ
 */
void CnavigationDlg::OnBnClickedButtonRecord()
{
	if (!is_play)   is_record ^= 1;
	navigation.setTargetFilename();
	navigation.setRecordMode(is_record);
	navigationView.setStatus(is_record, is_play);
}

/*!
 * @brief [Play]ボタンを押した時のハンドラ
 */
void CnavigationDlg::OnBnClickedButtonPlay()
{
	if (!is_record) is_play ^= 1;
	navigation.setTargetFilename("navi.csv");
	navigation.setPlayMode(is_play);
	navigationView.setStatus(is_record, is_play);
}

/*!
 * @brief [loop]のチェックボックスを押した時のハンドラ
 */
void CnavigationDlg::OnBnClickedCheckLoop()
{
	UpdateData(true);
}
