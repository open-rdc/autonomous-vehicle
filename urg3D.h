#pragma once
#include <vector>
#include "URG.h"
#include "rs405cb.h"

class urg3D
{
private:
	static const int SERVO_OFFSET = -5;		// サーボのオフセット(取付け角度を見ながら設定)
	static const int MAX_NUM = 100000;		// (1081点 * 20回)/秒 - 現時点で5秒程度のデータを保存可能な領域

	CURG urg;								// URGのクラス
	int num;
	pos_inten upos_inten[MAX_NUM];
	HANDLE hComm;
	HANDLE mutex;
	int tilt_low, tilt_high;
	float tilt_period;
	int terminate;
	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);	// スレッドのエントリーポイント
	DWORD WINAPI ExecThread();				// 別スレッドで動作する関数
	int Update();							// 定期的(50ms)に呼び出される関数

public:
	urg3D();								// コンストラクタ
	virtual ~urg3D();						// デストラクタ
	int Init();								// 初期設定
	int Close();							// 終了処理
	int GetAllData(pos_inten *p, int max_no);		// URGのデカルト座標系での障害物データを取得
	int GetSelectedData(int low, int high, pos *p, int max_no);
											// 高さを指定してurgのデカルト座標系での障害物データを取得
	int Get2SelectedData(int low1, int high1, pos *p1, int *no1, int max_no1,
		int low2, int high2, pos *p2, int *no2, int max_no2);
											// ２つの高さを指定してurgのデカルト座標系での障害物データを取得
	int Get3SelectedData(int low1, int high1, pos *p1, int *no1, int max_no1,
		int low2, int high2, pos *p2, int *no2, int max_no2,
		int low3, int high3, pos_inten *p3, int *no3, int max_no3, int min_intensity);
											// ３つの高さを指定してurgのデカルト座標系での障害物データを取得
	int SetTiltAngle(int low, int high, float period);
											// チルトアングルの動きの設定
	int ClearData();						// データをクリアする
};
