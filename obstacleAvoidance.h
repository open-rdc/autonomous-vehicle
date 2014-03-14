#pragma once
#include "dataType.h"

class obstacleAvoidance
{
public:
	obstacleAvoidance();
	virtual ~obstacleAvoidance();

private:
	static const int MARGIN = 200;				//! 障害物として検出する領域のロボットの幅を基準とする左右のマージン (mm)
	static const int TREAD  = 282;				//! トレッド（左右のタイヤの幅） (mm)
	static const int STOP_LENGTH = 500;			//! 停止する距離 (mm)
	static const int SLOW_DOWN_LENGTH = 1000;	//! 減速を開始する距離 (mm)
	static const int REROUTE_PERIOD = 10;		//! 障害物を検出してからリルートするまでの時間 (s)

	// 障害物のデータを受け取るデータ領域
	static const int MAX_OBS_NUM = 1000;		//! 障害物の最大値
	pos obs[MAX_OBS_NUM];						//! 障害物の位置データ
	int obs_num;								//! 障害物の個数

	int is_obstacle;							//! 障害物の有無（CENTER）
	int is_need_stop;
	float slow_down_factor;						//! 減速の比率（STOP_LENGTH以下で0, SLOW_DOWN_LENGTHで1）
												//! これを目標速度にかけることで，徐々に停止する
	long obstacle_detect_time;					//! 障害物を検出した時間(ms)
	float obstacle_detect_period;				//! 障害物の検出している時間(s)
	int is_reroute;								//! リルート中かどうか
	int reroute_direction;						//! リルートの方向　(左：+1, 右 -1)

	int getDataNum(int x_min, int y_min, int x_max, int y_max, int *nearest_x);
													// (x_min,y_min)-(x_max,y_max)に含まれる障害物の個数を計算
													// nearest_xに最も近いx座標を戻す
	int isDetectObstacle(int right_center_left, int *min_x);
													// 右，中央，左に障害物があるかを計測する
	static const int RIGHT = -1, CENTER = 0, LEFT = +1;	//! 右，中央，左の定数

public:

	int Init();										// 初期化
	int Close();									// 終了処理
	int isObstacle();								// 障害物があるかどうかを戻す
	float getSlowDownFactor();						// 減速の比率を戻す
	int setData(pos *p, int num);					// 障害物の位置データを設定
	int isReroute();					// リルート中かどうか
	int getRerouteDirection();						// リルートの方向
	int finishReroute();							// リルートを終了する
	int Update();									// 障害物回避の処理を行う．(setData毎に呼び出される)
	int isNeedStop();								// 停止が必要な状況かどうかを戻す　(1:停止が必要，0:必要がない）
};

/*
 * ■手順
 * 1) 障害物を検出（SLOW_DOWN_LENGTHから減速開始，STOP_LENGTHで停止）
 * 2) 暫く時間が経過したらリルート開始(障害物検出からREROUTE_PERIOD経過)　【ToDo: ここから未実装】
 * 3) 左右の空き状況を検出して空いていればそちらにリルート（右優先）
 * 4) 空いている方向に90°ターン(forceControl) 完全に制御を奪う
 * 5) 障害物が無いかチェック（あれば90°戻して逆方向にターン）
 * 6) ルートにオフセットを足して通常の制御を行う．(getRerouteOffset)
 * 7) 障害物を検出しなくなったら元の方向に90°旋回
 * 8) 通常通り走行
 *
 */
