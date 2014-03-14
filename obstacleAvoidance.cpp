/*!
 * @class obstacleAvoidance
 * @brief 障害物回避を行うクラス
 * @date 2013.11.09
 * @author Y.Hayashibara
 *
 * 目の前に障害物があればslow_down_factorの値を小さく(1->0)して停止する．
 */

#include "stdafx.h"
#include "obstacleAvoidance.h"
#include <mmsystem.h>
#include <time.h>

/*!
 * @brief コンストラクタ
 */
obstacleAvoidance::obstacleAvoidance():
is_obstacle(0), slow_down_factor(1.0f),
obstacle_detect_time(0), obstacle_detect_period(0),
is_reroute(0), reroute_direction(0), is_need_stop(0)
{
}

/*!
 * @brief デストラクタ
 */
obstacleAvoidance::~obstacleAvoidance()
{
}

/*!
 * @brief 初期化
 *
 * @return 0
 */
int obstacleAvoidance::Init()
{
	return 0;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int obstacleAvoidance::Close()
{
	return 0;
}

/*!
 * @brief 障害物があるかどうかを戻す
 *
 * @return 0:障害物無し,1:障害物有り
 */
int obstacleAvoidance::isObstacle()
{
	return is_obstacle;
}

/*!
 * @brief 減速の比率を戻す
 * 減速の比率（STOP_LENGTH以下で0, SLOW_DOWN_LENGTHで1）
 * これを目標速度にかけることで，徐々に停止する
 *
 * @return 減速の比率(0～1)
 */
float obstacleAvoidance::getSlowDownFactor()
{
	return slow_down_factor;
}

/*!
 * @brief 障害物の位置データを設定
 *
 * @param[in] p   障害物の位置データ
 * @param[in] num 障害物の位置データの数
 *
 * @return 0
 */
int obstacleAvoidance::setData(pos *p, int num)
{
	obs_num = min(num, MAX_OBS_NUM);
	pos *q = obs;
	for(int i = 0; i < obs_num; i ++){
		*q ++ = *p ++;
	}
	Update();

	return 0;
}

/*!
 * @brief リルートするかどうかを戻す．
 *
 * @param[out] direction 回避する方向
 *
 * @return 0:リルートする，1:リルートしない．
 */
int obstacleAvoidance::isReroute()
{
	return is_reroute;
}

int obstacleAvoidance::getRerouteDirection()
{
	return reroute_direction;
}


/*!
 * @brief 障害物回避の処理を行う．(setData毎に呼び出される)
 *
 * @return 0
 */
int obstacleAvoidance::Update()
{
	static const int FLAG_CLEAR_PERIOD = 2000;
	static long time0 = 0;
	static int min_len0 = SLOW_DOWN_LENGTH;		//! 障害物の最短距離を保存する変数．FLAG_CLEAR_PERIOD(2秒)だけ障害物を発見しないとクリア

	int min_len;
	is_obstacle = isDetectObstacle(CENTER, &min_len);	// 前の障害物の検出
	
	float factor = (float)(min_len - STOP_LENGTH)/(SLOW_DOWN_LENGTH - STOP_LENGTH);
	if (is_obstacle && (factor < 0.1f)){
		is_need_stop = 1;
	} else {
		is_need_stop = 0;
	}

	// 障害物の検出，スキャンを繰り返すため，一度検出したフラグを暫く保持
	if (is_obstacle){	// 回避すべき障害物がある場合
		// slow_down_factor STOP_LENGTH以下で0, SLOW_DOWN_LENGTHで1
		if (min_len < min_len0) min_len0 = min_len;
		slow_down_factor = (float)(min_len0 - STOP_LENGTH)/(SLOW_DOWN_LENGTH - STOP_LENGTH);
		slow_down_factor = min(max(slow_down_factor, 0),1);
		
		// 障害物を検知している時間を求める
		if (slow_down_factor < 0.1){
			if (obstacle_detect_time == 0){
				obstacle_detect_time = timeGetTime();
			} else {
				obstacle_detect_period = (float)(timeGetTime() - obstacle_detect_time)/1000;
			}
		} else {
			obstacle_detect_time = 0;
			obstacle_detect_period = 0;
		}

		// リルート
		if ((!is_reroute) && (obstacle_detect_period > REROUTE_PERIOD)){
			obstacle_detect_time = 0;
			is_reroute = 1;
			if (!isDetectObstacle(RIGHT, &min_len)){
				reroute_direction = RIGHT;
			} else if (!isDetectObstacle(LEFT , &min_len)){
				reroute_direction = LEFT;
			} else {
				reroute_direction = RIGHT;
			}
		}
		time0 = timeGetTime();
	} else {						// 障害物が無い場合
		long time = timeGetTime();							// 2秒以上の間隔をあけてフラグのクリア
		if ((time - time0) > FLAG_CLEAR_PERIOD){
			is_obstacle = 0;
			slow_down_factor = 1;
			obstacle_detect_time = 0;
			obstacle_detect_period = 0;
			min_len0 = SLOW_DOWN_LENGTH;
		}
	}
	
	return 0;
}

/*!
 * @brief ある領域に入る障害物の個数の計算
 * (x_min,y_min)-(x_max,y_max)に含まれる
 *
 * @param[in] x_min     探索するx座標の最小値(mm) - 後
 * @param[in] y_min     探索するy座標の最小値(mm) - 右
 * @param[in] x_max     探索するx座標の最大値(mm) - 前
 * @param[in] y_max     探索するy座標の最大値(mm) - 左
 * @param[in] nearest_x 探索するy座標の最大値(mm) - 前
 *
 * @return 障害物の個数
 */
int obstacleAvoidance::getDataNum(int x_min, int y_min, int x_max, int y_max, int *nearest_x)
{
	int point_num = 0, min_len = 10000;
	for(int i = 0; i < obs_num; i ++){
		if ((obs[i].x >= x_min)&&(obs[i].x < x_max)&&(obs[i].y >= y_min)&&(obs[i].y <= y_max)){
			point_num ++;
			if (obs[i].x < min_len) min_len = obs[i].x;
		}
	}
	*nearest_x = min_len;

	return point_num;
}

/*!
 * @brief 右，中央，左に障害物があるかを計測する
 *
 * @param[in] right_center_left 右と中央と左の指定(RIGHT=-1,CENTER=0,LEFT=+1)
 *
 * @return 障害物の個数
 */
int obstacleAvoidance::isDetectObstacle(int right_center_left, int *min_x)
{
	static const int MIN_POINT = 5;		// 障害物とみなす最小のデータ数（１回のスキャンで）
	int point_num = 0, min_len = 10000, res = 0;
	
	if (right_center_left == CENTER){
		point_num = getDataNum(100, -(TREAD/2+MARGIN)      , SLOW_DOWN_LENGTH, (TREAD/2+MARGIN)      , &min_len);	// 中央で１台分探索したときに障害物を発見するか．
	} else if (right_center_left == RIGHT){
		point_num = getDataNum(100, -(TREAD/2+MARGIN)-TREAD, SLOW_DOWN_LENGTH, (TREAD/2+MARGIN)-TREAD, &min_len);	// 右に１台分探索したときに障害物を発見するか．	
	} else if (right_center_left == LEFT){
		point_num = getDataNum(100, -(TREAD/2+MARGIN)+TREAD, SLOW_DOWN_LENGTH, (TREAD/2+MARGIN)+TREAD, &min_len);	// 左に１台分探索したときに障害物を発見するか．
	}
	if (point_num > MIN_POINT){
		res = 1;
		*min_x = min_len;
	}

	return res;
}

/*!
 * @brief リルートを終了する．
 *
 * @return 0
 */
int obstacleAvoidance::finishReroute()
{
	is_reroute = 0;

	return 0;
}

/*!
 * @brief 停止が必要な状況かどうかを戻す
 *
 * @return 1:停止が必要，0:必要がない
 */
int obstacleAvoidance::isNeedStop()
{
	return is_need_stop;
}
