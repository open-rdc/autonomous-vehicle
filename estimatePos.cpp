/*!
 * @file  estimatePos.cpp
 * @brief モンテカルロ自己位置推定プログラム
 * @date 2013.10.31
 * @author Y.Hayashibara
 */

#include "stdafx.h"
#include <math.h>
#include "estimatePos.h"

#define	M_PI	3.14159f

/*!
 * @brief 角度を-PI～PIに変換するための関数
 *
 * @param[in] rad 角度(rad)
 *
 * @return -PI～PIに変換した角度
 */
float maxPI(float rad){
	while(rad >  M_PI) rad -= (2.0f * M_PI);
	while(rad < -M_PI) rad += (2.0f * M_PI);
	return rad;
}

/*!
 * @class estimatePos
 * @brief 自己位置推定を行うためのクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
estimatePos::estimatePos():
odoX(0), odoY(0), odoThe(0), ref_data_no(0), is_ref_data_full(0), data_no(0),
estX(0), estY(0), estThe(0), estVar(0), coincidence(0)
{
}


/*!
 * @brief デストラクタ
 */
estimatePos::~estimatePos()
{
}


/*!
 * @brief 初期化
 *
 * @param[in] x   自己位置のx座標(m)（リファレンスのワールド座標系）
 * @param[in] y   自己位置のy座標(m)（リファレンスのワールド座標系）
 * @param[in] the 自己位置の角度(rad)（リファレンスのワールド座標系）
 *
 * @return 0
 */
int estimatePos::Init(float x, float y, float the)
{
	// 変数の初期化
	odoX = x, odoY = y, odoThe = the;
	estX = x, estY = y, estThe = the;
	ref_data_no = is_ref_data_full = 0;
	data_no = 0;
	estVar = coincidence = 0;
	the = maxPI(the);
	
	// パーティクルの初期化
	for(int i = 0; i < MAX_PARTICLE; i ++){
		particle[i].x    = x  ;
		particle[i].y    = y  ;
		particle[i].the  = the;
		particle[i].eval = 0  ;
	}
	// 参照データのクリア
	memset(refData, 0, sizeof(pos) * MAX_REF_DATA);

	return 0;
}


/*!
 * @brief 終了処理
 *
 * @return 0
 */
int estimatePos::Close()
{
	return 0;
}


/*!
 * @brief オドメトリデータの入力
 * 計算をする前に，必ず入力する．（障害物の位置データの基準となる）
 *
 * @param[in] x   オドメトリのx座標(m)（現在のワールド座標系）
 * @param[in] y   オドメトリのx座標(m)（現在のワールド座標系）
 * @param[in] the オドメトリの角度(rad)（現在のワールド座標系）
 *
 * @return 0
 */
int estimatePos::setOdometory(float x, float y, float the)
{
	odoX = x;
	odoY = y;
	odoThe = maxPI(the);

	return 0;
}


/*!
 * @brief 推定した移動量の入力
 *
 * @param[in] dx   推定した移動量のx座標(m)（リファレンスのワールド座標系）
 * @param[in] dy   推定した移動量のy座標(m)（リファレンスのワールド座標系）
 * @param[in] dthe 推定した移動量の角度(rad)（リファレンスのワールド座標系）
 *
 * @return 0
 */
int estimatePos::setDeltaPosition(float dx, float dy, float dthe)	// m, rad
{
	const float thre = 0.5f;					// 次の世代に生き残る割合
	const float var_fb = 0.01f, var_ang = 0.005f;
	int num = (int)(MAX_PARTICLE * thre);		// 生き残りの数

	for(int i = MAX_PARTICLE - 1; i >= 0; i --){
		particle[i].x   = particle[i % num].x + dx + cos(particle[i % num].the) * gaussian() * var_fb;
		particle[i].y   = particle[i % num].y + dy + sin(particle[i % num].the) * gaussian() * var_fb;
		particle[i].the = maxPI(particle[i % num].the + dthe + gaussian() * var_ang);
	}

	return 0;
}


/*!
 * @brief パーティクルフィルタを使った自己位置推定の処理
 *
 * @return 0
 */
int estimatePos::calcualte()
{
	evaluate();										// パーティクルの評価
	estVar = getVariance(&estX, &estY, &estThe);	// リファレンスのワールド座標系における推定位置を求める．
	
	return 0;
}


/*!
 * @brief リファレンスデータのクリア
 * この関数が呼び出されるまで，リファレンスの距離データは蓄積され続ける．
 *
 * @return 0
 */
int estimatePos::clearRefData()
{	
	ref_data_no = is_ref_data_full = 0;

	return 0;
}


/*!
 * @brief リファレンスデータの追加
 *
 * @param[in] p リファレンスとなる障害物の位置データ（リファレンスのワールド座標系）
 * @param[in] num リファレンスデータの個数
 *
 * @return 0
 */
int estimatePos::addRefData(pos *p, int num)
{
	for(int i = 0; i < num; i ++){
		refData[ref_data_no ++] = p[i];
		if (ref_data_no >= MAX_REF_DATA){
			is_ref_data_full = 1;
			ref_data_no = 0;
		}
	}
	
	return 0;
}


/*!
 * @brief 計測データを設定する
 *
 * @param[in] p 計測した障害物の位置データ（現在のワールド座標系）
 * @param[in] num データの個数
 *
 * @return 0
 */
int estimatePos::setData(pos *p, int num)
{
	data_no = min(num, MAX_DATA);
	for(int i = 0; i < data_no; i ++){
		data[i] = p[i];
	}
	return 0;
}


/*!
 * @brief 推定位置の取得
 *
 * @param[in] x    推定した位置のx座標(m)（リファレンスのワールド座標系）
 * @param[in] y    推定した位置のy座標(m)（リファレンスのワールド座標系）
 * @param[in] the  推定した位置の角度(rad)（リファレンスのワールド座標系）
 * @param[in] var  分散(0.0-1.0)
 * @param[in] coin 一致度(0.0-1.0)
 *
 * @return 0
 */
int estimatePos::getEstimatedPosition(float *x, float *y, float *the, float *var, float *coin)
{
	*x    = estX  ;
	*y    = estY  ;
	*the  = estThe;
	*var  = estVar;
	*coin = coincidence;

	return 0;
}


/*!
 * @brief ガウス分布する乱数を発生
 *
 * @return ガウス分布する乱数
 */
float estimatePos::gaussian()
{
	float x, y, s, t;

	do{
		x = (float)rand()/RAND_MAX;
	} while (x == 0.0);											// xが0になるのを避ける
	y = (float)rand()/RAND_MAX;
	s = sqrt(-2.0f * log(x));
	t = 2.0f * M_PI * y;

	return s * cos(t);											// 標準正規分布に従う擬似乱数
}


/*!
 * @brief パーティクルの評価とソート
 *
 * @return 0
 */
int estimatePos::evaluate()
{
	static const int point_wide = 4;							// 得点を与える隣の数
	static const int point[point_wide + 1] = {16,8,4,2,1};		// 与える得点
	static const int wide = point_wide * 2 + 1;					// テーブルのx方向のサイズ
	static char table[wide][wide];								// 得点のテーブル

	static const int num_x = (search_x1 - search_x0)/dot_per_mm;
	static const int num_y = (search_y1 - search_y0)/dot_per_mm;
	static const int min_x = search_x0 / dot_per_mm;
	static const int max_x = search_x1 / dot_per_mm;
	static const int min_y = search_y0 / dot_per_mm;
	static const int max_y = search_y1 / dot_per_mm;
	static char map[num_y][num_x];

	// テーブルの作成
	for(int i = 0; i < wide; i ++){
		for(int j = 0; j < wide; j ++){
			table[i][j] = point[max(abs(i - point_wide), abs(j - point_wide))];
		}
	}

	// マップの作成 (estX, estY)を中心
	memset(map, 0, num_x*num_y);								// mapのクリア
	int ref_no = ref_data_no;
	if (is_ref_data_full) ref_no = MAX_REF_DATA;				// 参照するデータの数

	for(int i = 0;i < ref_no; i ++){
		int x = (int)((refData[i].x - estX * 1000) / dot_per_mm) - min_x;		// マップ上の位置を計算
		int y = (int)((refData[i].y - estY * 1000) / dot_per_mm) - min_y;		
		if ((x < point_wide)||(x >= (num_x - point_wide))||
			(y < point_wide)||(y >= (num_y - point_wide))) continue;
		for(int yp = 0; yp < wide; yp ++){
			for(int xp = 0; xp < wide; xp ++){
				int xt = x + (xp - point_wide);
				int yt = y + (yp - point_wide);
				map[yt][xt] = max(map[yt][xt], table[yp][xp]);	// より評価の高いものを採用
			}			
		}
	}

	// パーティクルの評価
	pos p;
	for(int i = 0;i < MAX_PARTICLE; i ++){
		float px   = particle[i].x * 1000;				// 現在のパーティクルの位置(m->mm)
		float py   = particle[i].y * 1000;
		float pthe = particle[i].the;
		particle[i].eval = 0;							// 評価を0に戻す
		for(int j = 0; j < data_no; j ++){
			float dx0 = data[j].x - odoX * 1000;
			float dy0 = data[j].y - odoY * 1000;
			float dx  =   dx0 * cos(odoThe) + dy0 * sin(odoThe);		// 現在計測している距離データ（ロボット座標）
			float dy  = - dx0 * sin(odoThe) + dy0 * cos(odoThe);
			// 計測データをパーティクルの位置を基準に変換
			p.x = (int)(dx * cos(pthe) - dy * sin(pthe) + px);
			p.y = (int)(dx * sin(pthe) + dy * cos(pthe) + py);
			int xt = (int)((p.x - estX * 1000) / dot_per_mm) - min_x;	// マップ上の位置を計算
			int yt = (int)((p.y - estY * 1000) / dot_per_mm) - min_y;
			if ((xt >= 0)&&(xt < num_x)&&(yt >= 0)&&(yt < num_y)){
				particle[i].eval += map[yt][xt];
			}
		}
	}
	// 評価によりソーティング
	qsort(particle, MAX_PARTICLE, sizeof(struct particle_T), comp);
	
	// 一致度を計算　(0-1)
	coincidence = 0;
	for(int i = 0;i < MAX_PARTICLE; i ++){
		coincidence += particle[i].eval;
	}
	if (data_no){
		coincidence /= (MAX_PARTICLE * point[0] * data_no);
	} else {
		coincidence = 0;
	}
	return 0;
}

/*!
 * @brief ソートのための比較関数
 *
 * @param[in] c1 パーティクル１のポインタ
 * @param[in] c2 パーティクル２のポインタ
 *
 * @return 1:c1の評価が高い，0:評価が同じ，-1:c2の評価が高い
 */
int estimatePos::comp(const void *c1, const void *c2)
{
	const struct particle_T *p1 = (struct particle_T *)c1;
	const struct particle_T *p2 = (struct particle_T *)c2;

	if      (p2->eval > p1->eval) return 1;
	else if (p2->eval < p1->eval) return -1;
	else return 0;
}

/*!
 * @brief 分散を計算する
 *
 * @param[out] ave_x   xの平均値(m)
 * @param[out] ave_y   yの平均値(m)
 * @param[out] ave_the 角度の平均値(rad)
 *
 * @return 分散の値
 */
float estimatePos::getVariance(float *ave_x, float *ave_y, float *ave_the)
{
	float sum_x = 0, sum_y = 0, sum_t = 0, sumv = 0;
	float ax, ay, at;

	for(int i = 0; i < MAX_PARTICLE; i ++){
		sum_x  += particle[i].x;
		sum_y  += particle[i].y;
		sum_t  += maxPI(particle[i].the - particle[0].the);
		// -PIとPIで平均して，0になることを防ぐ．theが-PI～PIであることが前提
	}
	ax = sum_x / MAX_PARTICLE;
	ay = sum_y / MAX_PARTICLE;
	at = maxPI(sum_t / MAX_PARTICLE + particle[0].the);

	for(int i = 0; i < MAX_PARTICLE; i ++){
		float dx = particle[i].x   - ax;
		float dy = particle[i].y   - ay;
		float dt = maxPI(particle[i].the - at);
		sumv += dx * dx + dy * dy + at * at;	// mとradを混在させても良いか？
	}
	*ave_x = ax, *ave_y = ay, *ave_the = at;

	return sumv / MAX_PARTICLE;
}

/*!
 * @brief パーティクルの取得
 *
 * @param[out] p       パーティクルデータのポインタ
 * @param[out] num     取得したパーティルクの数
 * @param[in]  max_num 最大のパーティクル数
 *
 * @return 0
 */
int estimatePos::getParticle(struct particle_T *p, int *num, int max_num)
{
	*num = min(max_num, MAX_PARTICLE);
	struct particle_T *q = particle;
	for(int i = 0; i < *num; i ++){
		*p ++ = *q ++;
	}

	return 0;
}

/*!
 * @brief 参照エリアの取得
 *
 * @param[out] x_min x軸の最小値(mm)
 * @param[out] y_min y軸の最小値(mm)
 * @param[out] x_max x軸の最大値(mm)
 * @param[out] y_max y軸の最大値(mm)
 * @param[out] dot_per_mm 一つのピクセルの距離(mm)
 *
 * @return 0
 */
int estimatePos::getReferenceArea(int *x_min, int *y_min, int *x_max, int *y_max, int *dot_per_mm)
{
	*x_min = search_x0;
	*y_min = search_y0;
	*x_max = search_x1;
	*y_max = search_y1;
	*dot_per_mm = this->dot_per_mm;

	return 0;
}
