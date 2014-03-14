/*!
 * @file  estimatePos.h
 * @brief モンテカルロ自己位置推定プログラム
 * @date 2013.10.31
 * @author Y.Hayashibara
 */

#pragma once
#include "dataType.h"

float maxPI(float rad);									// 角度を-PI～PIに変換するための関数

class estimatePos
{
public:
	estimatePos();										// コンストラクタ
	virtual ~estimatePos();								// デストラクタ

private:
	// 位置の補正に用いるデータの範囲（ロボットを原点とするワールド座標）
	static const int search_x0 = -14000, search_x1 = 14000;		// 前後方向の探索範囲(mm)
	static const int search_y0 = -14000, search_y1 = 14000;		// 左右方向の探索範囲(mm)
	static const int dot_per_mm = 100;							// 一つのピクセルの距離(mm)*/

	// リファレンスデータ
	static const int MAX_REF_DATA = 10000;
	int ref_data_no, is_ref_data_full;
	pos refData[MAX_REF_DATA];
	float odoX, odoY, odoThe;							// 与えられた位置(m, rad)
	float estX, estY, estThe, estVar;					// 計算して求めた位置(m, rad, 分散)
	float coincidence;

	// 自律走行時に計測したデータ
	static const int MAX_DATA = 10000;
	int data_no;
	pos data[MAX_DATA];
	
	static const int MAX_PARTICLE = 500;
	struct particle_T particle[MAX_PARTICLE];
	float gaussian();									// ガウス分布する乱数を発生
	int evaluate();										// パーティクルの評価とソート
	static int comp(const void *c1, const void *c2);	// ソートのための比較関数
	float getVariance(float *ave_x, float *ave_y, float *ave_the);
														// 分散を計算する

public:
	int Init(float x, float y, float the);				// 初期化
	int Close();										// 終了処理

	int setOdometory(float x, float y, float the);		// オドメトリデータの入力
	int setDeltaPosition(float dx, float dy, float dthe);
														// 推定した移動量の入力
	int clearRefData();									// リファレンスデータのクリア
	int addRefData(pos *p, int num);					// リファレンスデータの追加
	int setData(pos *p, int num);						// 計測データを設定する
	int getEstimatedPosition(float *x, float *y, float *the, float *var, float *coin);
														// 推定位置の取得
	int calcualte();									// パーティクルフィルタを使った自己位置推定の処理
	int getParticle(struct particle_T *particle, int *num, int max_num);
														// パーティクルの取得
	int getReferenceArea(int *x_min, int *y_min, int *x_max, int *y_max, int *dot_per_mm);
														// 参照エリアの取得
};

/* 使い方　（１秒毎程度の周期）　時間がかかるので，優先度の低いスレッドで実行
 * 1) Init(x,y,the)で現在の位置を指定
 * 2) addRefData(p, num)で参照データを入力
 * 3) setData(p, num)で計測データを入力
 * 4) setDeltaPosition(dx, dy, dthe)で移動量(推定した距離と角度)を入力
 * 5) setOdometory(x, y, the)でオドメトリを入力
 * 6) calculate()で自己位置を推定
 * 7) getEstimatedPosition(&ex, &ey, &ethe, &var, &coincidence)で推定した位置を取得
 * 8) varもしくはcoinが適切な中に収まっている場合は，推定した自己位置を変更
 * 9) 2)に戻る
 */
