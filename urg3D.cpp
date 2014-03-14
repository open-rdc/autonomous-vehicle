// URG.cpp : 実装ファイル

#include "stdafx.h"
#include "urg3D.h"
#include "logger.h"

#define URG_PORT 2

/*!
 * @class urg3D
 * @brief URGを使用して3D障害物データを検出するクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
urg3D::urg3D():
tilt_low(0), tilt_high(0), tilt_period(1.0), num(0),terminate(0)
{
}

/*!
 * @brief デストラクタ
 */
urg3D::~urg3D()
{
}

/*!
 * @brief 初期化（最初に１回呼び出す）
 *
 * @return 0
 */
int urg3D::Init()
{
	// 排他処理
	mutex = CreateMutex(NULL, FALSE, NULL);

	// URGの初期設定
	if (!urg.Init(URG_PORT)) AfxMessageBox("Cannot communicate URG0");// UHGの初期設定

	// RS405CBの初期設定
	hComm = CommOpen( COM_PORT );				// 通信ポートを開く
	RSTorqueOnOff( hComm, 1 );					// トルクをONする
	Sleep(100);									// 少し待つ
	RSMove( hComm, SERVO_OFFSET * 10, 200 );	// 2秒かけて初期姿勢に移動
	Sleep(3000);								// 3秒待つ

	// 速度制御のスレッドを開始
	DWORD threadId;								// スレッド ID	
	HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, &threadId); 

	return 0;
}

/*!
 * @brief 終了処理（最後に１回呼び出す）
 *
 * @return 0
 */
int urg3D::Close()
{
	terminate = 1;
	CloseHandle(mutex);

	// RS405CBの終了処理
	RSMove( hComm, SERVO_OFFSET * 10, 200 );	// 2秒かけて初期姿勢に移動
	Sleep(3000);								// 3秒待つ
	RSTorqueOnOff( hComm, 0 );					// トルクをOFFする
	CommClose( hComm );							// 通信ポートを閉じる

	// URGの終了処理
	urg.Close();

	return 0;
}

/*!
 * @brief URGのデカルト座標系での障害物データを取得
 *
 * @param[in] p 位置データを保存する配列
 * @param[in] max_no データの最大個数（位置データの配列の最大値）
 * 
 * @return データの個数
 */
int urg3D::GetAllData(pos_inten *p, int max_no)
{
	WaitForSingleObject(mutex, INFINITE); 
	int no = min(num, max_no);
	for(int i = 0; i < no; i++){
		p[i].pos.x     = upos_inten[i].pos.x;
		p[i].pos.y     = upos_inten[i].pos.y;
		p[i].pos.z     = upos_inten[i].pos.z;
		p[i].intensity = upos_inten[i].intensity;
	}
	num -= no;
	ReleaseMutex(mutex);

	return no;
}

/*!
 * @brief 高さを指定してurgのデカルト座標系での障害物データを取得
 *
 * @param[in] low 取得するデータの最小高さ
 * @param[in] high 取得するデータの最大高さ
 * @param[in] p 位置データを保存する配列
 * @param[in] max_no データの最大個数（位置データの配列の最大値）
 * 
 * @return データの個数
 */
int urg3D::GetSelectedData(int low, int high, pos *p, int max_no)
{
	int ret = 0, i;

	WaitForSingleObject(mutex, INFINITE); 
	for(i = 0; ((i < num)&&(ret < max_no)); i++){
		if ((upos_inten[i].pos.z >= low)&&(upos_inten[i].pos.z <= high)){
			p[ret ++] = upos_inten[i].pos;
		}
	}
	num -= i;
	ReleaseMutex(mutex);

	return ret;
}

/*!
 * @brief ２つの高さを指定してurgのデカルト座標系での障害物データを取得
 *
 * @param[in] low1 取得するデータ１の最小高さ
 * @param[in] high1 取得するデータ１の最大高さ
 * @param[in] low2 取得するデータ２の最小高さ
 * @param[in] high2 取得するデータ２の最大高さ
 * @param[in] p1 位置データ１を保存する配列
 * @param[in] p2 位置データ２を保存する配列
 * @param[out] no1 データ１の個数
 * @param[out] no2 データ２の個数
 * @param[in] max_no1 データ１の最大個数（位置データの配列の最大値）
 * @param[in] max_no2 データ２の最大個数（位置データの配列の最大値）
 * 
 * @return データの個数
 */
int urg3D::Get2SelectedData(int low1, int high1, pos *p1, int *no1, int max_no1,
							int low2, int high2, pos *p2, int *no2, int max_no2)
{
	int n1 = 0, n2 = 0, i;

	WaitForSingleObject(mutex, INFINITE); 
	for(i = 0; ((i < num)&&(n1 < max_no1)&&(n2 < max_no2)); i++){
		if ((upos_inten[i].pos.z >= low1)&&(upos_inten[i].pos.z <= high1)){
			p1[n1 ++] = upos_inten[i].pos;
		}
		if ((upos_inten[i].pos.z >= low2)&&(upos_inten[i].pos.z <= high2)){
			p2[n2 ++] = upos_inten[i].pos;
		}
	}
	*no1 = n1, *no2 = n2;
	num -= i;
	ReleaseMutex(mutex);

	return 0;
}

/*!
 * @brief ３つの高さを指定してurgのデカルト座標系での障害物データを取得
 * １つに関しては，反射強度も取得する．
 *
 * @param[in] low1 取得するデータ１の最小高さ
 * @param[in] high1 取得するデータ１の最大高さ
 * @param[in] p1 位置データ１を保存する配列
 * @param[out] no1 データ１の個数
 * @param[in] max_no1 データ１の最大個数（位置データの配列の最大値）
 * @param[in] low2 取得するデータ２の最小高さ
 * @param[in] high2 取得するデータ２の最大高さ
 * @param[in] p2 位置データ２を保存する配列
 * @param[out] no2 データ２の個数
 * @param[in] max_no2 データ２の最大個数（位置データの配列の最大値）
 * @param[in] low3 取得するデータ３の最小高さ
 * @param[in] high3 取得するデータ４の最大高さ
 * @param[in] p3 位置データ３を保存する配列（反射強度も含む）
 * @param[out] no3 データ３の個数
 * @param[in] max_no3 データ３の最大個数（位置データの配列の最大値）
 * @param[in] min_intensity 反射強度の最小値
 * 
 * @return データの個数
 */
int urg3D::Get3SelectedData(int low1, int high1, pos *p1, int *no1, int max_no1,
							int low2, int high2, pos *p2, int *no2, int max_no2,
							int low3, int high3, pos_inten *p3, int *no3, int max_no3, int min_intensity)
{
	int n1 = 0, n2 = 0, n3 = 0, i;

	WaitForSingleObject(mutex, INFINITE); 
	for(i = 0; ((i < num)&&(n1 < max_no1)&&(n2 < max_no2)); i++){
		if ((upos_inten[i].pos.z >= low1)&&(upos_inten[i].pos.z <= high1)){
			p1[n1 ++] = upos_inten[i].pos;
		}
		if ((upos_inten[i].pos.z >= low2)&&(upos_inten[i].pos.z <= high2)){
			p2[n2 ++] = upos_inten[i].pos;
		}
		if ((upos_inten[i].pos.z >= low3)&&(upos_inten[i].pos.z <= high3)&&
			(upos_inten[i].intensity > min_intensity)){
			p3[n3 ++] = upos_inten[i];
		}
	}
	*no1 = n1, *no2 = n2, *no3 = n3;
	num -= i;
	ReleaseMutex(mutex);

	return 0;
}

/*!
 * @brief チルトアングルの動きの設定
 *
 * @param[in] low 最小角度(deg) -90～90
 * @param[in] high 最大角度(deg) -90～90
 * @param[in] period 時間(sec) low->high, high->lowに要する時間
 * 
 * @return 0
 */
int urg3D::SetTiltAngle(int low, int high, float period)
{
	if ((low > high)||(period < 0.0f)||(low < -90)||(high > 90)) return -1;
	tilt_low    = low;		// deg
	tilt_high   = high;		// deg
	tilt_period = period;	// sec

	return 0;
}

/*!
 * @brief スレッドのエントリーポイント
 *
 * @param[in] lpParameter インスタンスのポインタ
 * 
 * @return S_OK
 */
DWORD WINAPI urg3D::ThreadFunc(LPVOID lpParameter) 
{
	return ((urg3D*)lpParameter)->ExecThread();
}

/*!
 * @brief 別スレッドで動作する関数
 * Update()を50ms毎に呼び出している．
 *
 * @return S_OK
 */
DWORD WINAPI urg3D::ExecThread()
{
	while(!terminate){
		Update();
		Sleep(50);
	}
	return S_OK; 
}


/*!
 * @brief 定期的(50ms)に呼び出される関数
 * 1)サーボモータの制御
 * 2)サーボモータの角度の取得
 * 3)URGのデータを取得
 * 4)デカルト座標系の占有データに変換して配列に保存
 *
 * @return 0
 */
int urg3D::Update()
{
	static int is_first = 1;
	static int length[urg.n_data], intensity[urg.n_data];
	static pos p[urg.n_data];
	static int is_up = 1, count = 1000;

	count ++;
	if (count >= (tilt_period * 1000 / 50)){
		if (is_up){
			RSMove(hComm, (tilt_high + SERVO_OFFSET) * 10, (int)(tilt_period * 100));
		} else {
			RSMove(hComm, (tilt_low  + SERVO_OFFSET) * 10, (int)(tilt_period * 100));
		}
		RSStartGetAngle(hComm);				// tilt角度の取得を始める．
		is_up ^= 1;
		count = 0;
		return 0;
	}

	if (is_first){
		RSStartGetAngle(hComm);				// tilt角度の取得を始める．
		urg.StartMeasure();
		is_first = 0;
		return 0;
	}
	
	int n = 0;
	float tilt_angle = RSGetAngle(hComm)/10.0f - SERVO_OFFSET;	// deg
	if (urg.n_data == urg.GetData(length, intensity)){
		n = urg.TranslateCartesian(tilt_angle, length, p);
	}
	WaitForSingleObject(mutex, INFINITE);	// mutexの開始
	pos *q = p;
	int *r = intensity;
	for(int i = 0; (i < n)&&(num < MAX_NUM);i ++){
		if ((q->x != 0)||(q->y != 0)||(q->z != 0)){
			upos_inten[num   ].pos       = *q ++;
			upos_inten[num ++].intensity = *r ++;
		} else {
			q ++;
		}
	}
	ReleaseMutex(mutex);					// mutexの終了

	// Logに書き出し
	LOG("tilt_angle:%f\n", tilt_angle);
	LOG("\n");								// 時間を表示するため 
	for(int i = 0; i < num ;i ++){
		LOG_WITHOUT_TIME("urg:(%d,%d,%d),intensity:%d\n",
			upos_inten[i].pos.x, upos_inten[i].pos.y, upos_inten[i].pos.z, upos_inten[i].intensity); 
	}

	RSStartGetAngle(hComm);					// tilt角度の取得を開始
	
	return 0;
}

/*!
 * @brief URG3Dのデータをクリアする．
 * 
 * @return 0
 */
int urg3D::ClearData()
{
	WaitForSingleObject(mutex, INFINITE); 
	num = 0;
	ReleaseMutex(mutex);
	
	return 0;
}