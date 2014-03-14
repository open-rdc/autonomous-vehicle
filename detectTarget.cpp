/*!
 * @file  detectTarget.cpp
 * @brief つくばチャレンジ用探索対象者検出プログラム
 * @date 2013.10.31
 * @author Y.Hayashibara
 */

#include "StdAfx.h"
#include "detectTarget.h"

/*!
 * @brief コンストラクタ
 */
detectTarget::detectTarget(void):
	intensity_data_no(0), slate_point_no(0), search_point_no(0),
	integrated_point_no(0), terminate(0)
{
}

/*!
 * @brief デストラクタ
 */
detectTarget::~detectTarget(void)
{
}

/*!
 * @brief 初期化
 *
 * @return 0
 */
int detectTarget::Init()									// 初期化
{
	mutex = CreateMutex(NULL, FALSE, _T("DETECT_TARGET_RESULT"));

	// 速度制御のスレッドを開始
	DWORD threadId;	
	HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, &threadId); 
	// スレッドの優先順位を上げる
	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

	return 0;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int detectTarget::Close()									// 終了処理
{
	terminate = 1;											// 探索対象検出スレッドの停止
	CloseHandle(mutex);
	return 0;
}

/*!
 * @brief 反射強度データをセット
 * 計算をする前に，必ず入力する．
 *
 * @param[in] p   反射強度データ(m)（現在のワールド座標系）
 * @param[in] num 反射強度データの個数
 *
 * @return 0
 */
int detectTarget::addIntensityData(pos_inten *p, int num)
{
	for(int i = 0; i < num; i ++){
		intensity_data[intensity_data_no ++] = p[i];
		if (intensity_data_no >= MAX_INTENSITY_DATA) break;
	}

	return 0;
}

/*!
 * @brief 探索対象の位置と確率を戻す
 *
 * @param[out] p   探索対象の位置と確率（現在のワールド座標系）
 * @param[out] num 探索対象の個数
 *
 * @return 0
 */
int detectTarget::getTargetPos(pos_slate *p, int *num)
{
	WaitForSingleObject(mutex, INFINITE);
	for(int i = 0; i < slate_point_no; i ++){
		p[i] = slate_point[i];
	}
	*num = slate_point_no;
	ReleaseMutex(mutex);
	return 0;
}

/*!
 * @brief 最近棒の探索対象の位置
 *
 * @param[out] p   最近傍の探索対象の位置（現在のワールド座標系）
 * @param[in] self_loc   ロボットの現在位置（現在のワールド座標系）
 * @param[in] radius   　ロボットの探索範囲(m)
 *
 * @return 0:探索対象無し，1:探索対象有り
 */
int detectTarget::getSearchPos(pos *p, pos self_loc, float radius)
{
	const float MIN_PROBABILITY = 0.2f;		//! ToDo: 確率のしきい値（複数個発見することで，確率が上がる．数値を大きくすると誤認識が減る．）

	int res = 0;

	for(int i = 0; i < search_point_no; i ++){
		if (distance_xy2(search_point[i].pos, self_loc) < radius){
			if (search_point[i].probability > MIN_PROBABILITY){
				*p = search_point[i].pos;
				res = 1;
				break;
			}
		}
	}

	return res;
}


/*!
 * @brief 反射強度から探索対象の候補を計算する．
 *
 * @return 0
 */
int detectTarget::calculateIntensity()
{
	const float INTEGRATE_RADIUS = 1.0;		// 統合する半径(m)
	const int DETECT_MIN_NUM = 5;
	const int DETECT_MAX_NUM = 15;
	
	int num = 0;

	integratePoints(intensity_data, intensity_data_no, integrated_point, &integrated_point_no, INTEGRATE_RADIUS);
	intensity_data_no = 0;

	WaitForSingleObject(mutex, INFINITE);
	for(int i = 0; i < integrated_point_no; i++){
		if (integrated_point[i].count > DETECT_MIN_NUM){
			slate_point[num].pos = integrated_point[i].pos;
			slate_point[num].probability = min(max((float)(integrated_point[i].count - DETECT_MIN_NUM) / 
				(DETECT_MAX_NUM - DETECT_MIN_NUM), 0.0f), 1.0f);
			num ++;
		}
	}
	slate_point_no = num;
	ReleaseMutex(mutex);
	
	return 0;
}

/*!
 * @brief 探索対象ポイントを計算する．
 *
 * @return 0
 */
int detectTarget::calculateSearchPoint()
{
	static const float INTEGRATE_RADIUS = 1.0f;		// 統合する半径(m)
	static const float UP_PROBABILITY   = 0.1f;
	static const float DOWN_PROBABILITY = 0.1f;

	int i,j;

	for(i = 0; i < slate_point_no; i ++){
		for(j = 0; j < search_point_no; j ++){
			if (distance_xy2(slate_point[i].pos, search_point[j].pos) <  INTEGRATE_RADIUS){
				float prob_search = search_point[j].probability;
				float prob_slate  = slate_point[i].probability;
				search_point[j].pos.x = (int)((search_point[j].pos.x * prob_search + slate_point[i].pos.x * prob_slate) / (prob_search + prob_slate));
				search_point[j].pos.y = (int)((search_point[j].pos.y * prob_search + slate_point[i].pos.y * prob_slate) / (prob_search + prob_slate));
				search_point[j].pos.z = (int)((search_point[j].pos.z * prob_search + slate_point[i].pos.z * prob_slate) / (prob_search + prob_slate));
				search_point[j].probability += (DOWN_PROBABILITY + UP_PROBABILITY * slate_point[i].probability);
				break;
			}
		}
		if (j == search_point_no){					// 同じ場所がなかった場合，探索ポイントを追加
			if (search_point_no < MAX_SEARCH_POINT){
				search_point[search_point_no] = slate_point[i];
				search_point[search_point_no].probability = (DOWN_PROBABILITY + UP_PROBABILITY * slate_point[i].probability);
				search_point_no ++;
			}
		}
	}
	qsort(search_point, search_point_no, sizeof(struct pos_slate_T), comp_slate);
	
	for(i = 0; i < search_point_no; i ++){		// 確率を下げて，0以下になったら探索対象から外す
		search_point[i].probability -= DOWN_PROBABILITY;
		if (search_point[i].probability <= 0.0f) break;
	}
	search_point_no = i;

	return 0;
}


/*!
 * @brief 周期的な処理（1s程度）
 *
 * @return 0
 */
int detectTarget::update()
{
	calculateIntensity();
	calculateSearchPoint();

	return 0;
}

/*!
 * @brief 距離を求める
 *
 * @param[in]  p   座標点１
 * @param[in]  p   座標点２
 *
 * @return 距離(m)
 */
float detectTarget::distance_xy2(pos p, pos q){
	float dx = (p.x - q.x) / 1000.0f;
	float dy = (p.y - q.y) / 1000.0f;

	return (dx * dx + dy * dy);
}

/*!
 * @brief pos_inten型のソートのための比較関数
 *
 * @param[in] c1 統合した位置１のポインタ
 * @param[in] c2 統合した位置２のポインタ
 *
 * @return 1:c1の個数が多い，0:個数が同じ，-1:c2の個数が少ない
 */
int detectTarget::comp_inten(const void *c1, const void *c2)
{
	const struct pos_integrate_T *p1 = (struct pos_integrate_T *)c1;
	const struct pos_integrate_T *p2 = (struct pos_integrate_T *)c2;

	if      (p2->count > p1->count) return 1;
	else if (p2->count < p1->count) return -1;
	else return 0;
}


/*!
 * @brief pos_slate型のソートのための比較関数
 *
 * @param[in] c1 統合した位置１のポインタ
 * @param[in] c2 統合した位置２のポインタ
 *
 * @return 1:c1の個数が多い，0:個数が同じ，-1:c2の個数が少ない
 */
int detectTarget::comp_slate(const void *c1, const void *c2)
{
	const struct pos_slate_T *p1 = (struct pos_slate_T *)c1;
	const struct pos_slate_T *p2 = (struct pos_slate_T *)c2;

	if      (p2->probability > p1->probability) return 1;
	else if (p2->probability < p1->probability) return -1;
	else return 0;
}


/*!
 * @brief 反射強度の大きい点を統合する．
 *
 * @param[in]  p   しきい値以上の反射強度データ(m)（現在のワールド座標系）
 * @param[in]  num しきい値以上の反射強度データの個数
 * @param[out] q   統合した反射強度の大きな位置(m)（現在のワールド座標系）
 * @param[out] num_pos_integrate 統合した反射強度の大きな位置の個数
 * @param[in]  radius 統合する半径
 *
 * @return 0
 */
int detectTarget::integratePoints(pos_inten *p, int num, pos_integrate *q, int *num_pos_integrate, float radius)
{
	int i, j, n, k = 0;
	float radius2 = radius * radius;
	
	for(i = 0; i < num; i ++){
		q[i].pos = p[i].pos;
		q[i].count = 1;
	}
	n = num;
	for(i = 0; i < n; i ++){
		if (!q[i].count) continue;
		for(j = i + 1; j < n; j ++){
			if (!q[j].count) continue;
			if (distance_xy2(q[i].pos, q[j].pos) <= radius2){
				int c = q[i].count;
				q[i].pos.x = (q[i].pos.x * c + q[j].pos.x) / (c + 1);
				q[i].pos.y = (q[i].pos.y * c + q[j].pos.y) / (c + 1);
				q[i].pos.z = (q[i].pos.z * c + q[j].pos.z) / (c + 1);
				q[i].count ++;
				q[j].count = 0;
			}
		}
	}

	// 個数によりソーティング
	qsort(q, n, sizeof(struct pos_integrate_T), comp_inten);
	for(i = 0; i < n; i ++){				// データを前に詰める
		if (!q[i].count) break;
	}
	*num_pos_integrate = i;

	return 0;
}

/*!
 * @brief スレッドのエントリーポイント
 *
 * @param[in] lpParameter インスタンスのポインタ
 * 
 * @return S_OK
 */
DWORD WINAPI detectTarget::ThreadFunc(LPVOID lpParameter) 
{
	return ((detectTarget*)lpParameter)->ExecThread();
}

/*!
 * @brief 別スレッドで動作する関数
 * ロボットの制御を別スレッドで行う．
 *
 * @return S_OK
 */
DWORD WINAPI detectTarget::ExecThread()
{
	while(!terminate){
		update();
		Sleep(1000);
	}
	return S_OK; 
}
