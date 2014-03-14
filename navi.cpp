/*!
 * @file  navi.cpp
 * @brief ナビゲーションのクラス
 * @date 2013.10.31
 * @author Y.Hayashibara
 *
 * 自己位置推定，目標経路の取得，モータの制御を統括するナビゲーションのクラス
 * １）Init()で初期化を行う．
 * ２）setOdometory(x,y,the)で現在の
 * ３）
 */

#include "stdafx.h"
#include <math.h>
#include <mmsystem.h>
#include <time.h>
#include "navi.h"
#include "logger.h"
#include "megaRover.h"

#define	M_PI	3.14159f

/*!
 * @class navi
 * @brief ナビゲーションを行うクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
navi::navi():
step(0), is_record(0), is_play(0), seek(0), step_period(1), time0(0),
tarX(0), tarY(0), tarThe(0), data_no(0), ref_data_no(0),
odoX0(0), odoY0(0), odoThe0(0),
estX0(0), estY0(0), estThe0(0), estX(0), estY(0), estThe(0),
hThread(NULL), coincidence(0),
is_search_object(0), is_search_mode(0), search_mode(0),
searchX(10000.0f), searchY(10000.0f),									// 非常に遠い位置を入れる
is_reroute_mode(0), reroute_direction(RIGHT), reroute_mode(0),
forwardSpeed(0), rotateSpeed(0), is_need_stop(0)
{
}

/*!
 * @brief デストラクタ
 */
navi::~navi()
{
}

/*!
 * @brief 初期化
 * このクラスを使用するときに最初に１回呼び出す．
 * 推定した自己位置を(0,0)，角度0radとしている．
 *
 * @return 0
 */
int navi::Init()
{
//	WaitForSingleObject(hThread, 3000);						// 自己位置の推定を行っている場合は待つ
	step = 0;
	seek = 0;
	time0 = 0;
	tarX = tarY = tarThe = 0;
	ref_data_no = 0;
	odoX0 = odoY0 = odoThe0 = 0;
	data_no = 0;
	estX0 = estY0 = estThe0 = 0;
	estX = estY = estThe = 0;	
	est_pos.Init(0,0,0);									// 自己位置推定の初期化

	return 0;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int navi::Close()
{
	est_pos.Close();										// 自己位置推定の終了処理

	return 0;
}

/*!
 * @brief オドメトリを設定する．
 *
 * オドメトリを入力したタイミングで，以下の処理を行う．
 * [record]
 * 一定周期（現在１秒）ごとに位置を記録する．これをplayでwaypointとして使用する．
 * [play]
 * waypointに到達する(ある程度近づく)ごとに以下の処理を行う．
 * 次のwaypointと参照する障害物の位置データをロードして，それを自己位置推定の関数に渡す．
 * これにより，次のwaypointに到達するまでに，waypointを通過した時の自己位置を推定する．
 * 自己位置推定の確度（距離データの一致度）が低い場合は，オドメトリのみで走行する．
 *
 * @param[in] x   オドメトリのx座標(m)
 * @param[in] y   オドメトリのy座標(m)
 * @param[in] the オドメトリの角度(rad) -PI～PI
 *
 * @return 0:正常終, 1:ゴールに到着
 */
int navi::setOdometory(float x, float y, float the)
{
	if (is_record){
		long time = timeGetTime();									// step_period以上の間隔を開けて保存（ウェイポイントとなる）
		if ((time - time0) > (int)(step_period * 1000)){
			saveNextOdometory(x, y, the);							// オドメトリの位置を保存
			step ++;												// ステップをインクリメント
			time0 = time;
		}
	}
	
	if (is_play){
		float dx0 = x - odoX0, dy0 = y - odoY0, dthe0 = the - odoThe0;	// オドメトリの差分
		float dthe = estThe0 - odoThe0;								// 推定した角度を使って補正
		float dx = dx0 * cos(dthe) - dy0 * sin(dthe);
		float dy = dx0 * sin(dthe) + dy0 * cos(dthe);
		if (is_reroute_mode){
			if (!rerouteProcess()) is_reroute_mode = 0;			
		} else if (is_search_mode){										// 探索対象者を探索するモードの場合
			if (!searchProcess()) is_search_mode = 0;			
		} else if (isPassTarget(estX, estY, estThe)){				// waypointをパスした場合
			if (loadNextOdoAndData(&tarX, &tarY, &tarThe, refData, &ref_data_no, MAX_REF_DATA)){
				return 1;											// ゴールに到着
			}
			step ++;												// waypoint番号のインクリメント
			if (WAIT_TIMEOUT != WaitForSingleObject(hThread, 0)){	// スレッドが終了している場合
																	// 次の目標位置，データのロード
				selfLocalization(x, y, the, dx, dy, dthe0);			// 自己位置推定の取得及び処理の開始
				odoX0 = x, odoY0 = y, odoThe0 = the;				// ウェイポイント通過時のオドメトリを保存（相対的な値を取得するだけなので，ずれていても問題なし）
				estX0 += dx, estY0 += dy, estThe0 += dthe0;			// ウェイポイント通過時の位置の推定値（次回上書きされる）
				dx0 = dy0 = dthe0 = dx = dy = 0;					// 次の自己位置の計算のためにクリア
			}
			data_no = 0;											// データのクリア
		}
		if (is_search_object){
			if (isPassSearchObject(estX, estY, estThe)){			// 探索対象が横に来た時
				is_search_mode = 1;									// 探索モードに移行
				search_mode = 0;									// 探索のシーケンス処理に使用する値をクリアする
				is_search_object = 0;								// 探索対象をクリアする
				retX = estX, retY = estY;							// 戻る位置を保存
			}
		}
		// 位置の推定値を計算
		estX   = dx    + estX0  ;
		estY   = dy    + estY0  ;
		estThe = dthe0 + estThe0;
	}

	return 0;
}


/*!
 * @brief 自己位置推定を行う．
 *
 * 1) 自己位置推定のスレッドが終了しているかをチェック．終了していなければreturn
 * 2) 推定した自己位置を取得．分散と一致度が適正な範囲であれば，その値をwaypoint通過時の推定値とする．
 * 3) estimatePos(自己位置推定のクラス)に，参照する障害物の位置データを渡す．
 * 4) estimatePosに計測した障害物の位置データを渡す．
 * 5) estimatePosに移動した差分を渡す．
 * 6) estimatePosに現在のオドメトリの値を渡す．
 * 7) 自己位置を計算するスレッドの開始
 *
 * @param[in] x    オドメトリのx座標(m)
 * @param[in] y    オドメトリのy座標(m)
 * @param[in] the  オドメトリの角度(rad) -PI～PI
 * @param[in] dx   移動したx座標の値(m)
 * @param[in] dy   移動したy座標の値(m)
 * @param[in] dthe 移動した回転角度(rad) -PI～PI
 *
 * @return -1:自己位置推定計算中のため，処理を行わない．0:処理を開始
 */
int navi::selfLocalization(float x, float y, float the, float dx, float dy, float dthe)
{
	static float max_var = 100000.0f, min_coin = 0.1f;
	
	// スレッドが終了していない場合は，処理をせず-1を戻す
	if (WAIT_TIMEOUT == WaitForSingleObject(hThread, 0)) return -1;

	float ex, ey, ethe, var;
	est_pos.getEstimatedPosition(&ex, &ey, &ethe, &var, &coincidence);
	int lv = (int)(coincidence * 10);					// 確度を10段階にして，音声で出力する．
	if ((lv >= 0)&&(lv <= 10)){
		char fn[10];
		sprintf(fn, "%d.wav", lv);
		PlaySound(fn, NULL, SND_FILENAME | SND_ASYNC);
	}

	if ((var < max_var)&&(coincidence > min_coin)){		// 信頼がおける値の場合は入れ替え
		estX0 = ex, estY0 = ey, estThe0 = ethe;
	}
	// 次の目標位置，データのロード
	est_pos.addRefData(refData, ref_data_no);
	est_pos.setData(data, data_no);
	est_pos.setDeltaPosition(dx, dy, dthe);				// オドメトリの差分として入力する（推定角度で補正後）
	est_pos.setOdometory(x, y, the);					// オドメトリの位置を直接入力する
	// 自己位置の計算のスレッドを開始
	hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, &threadId); 
	// スレッドの優先順位を下げる
	SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
	
	return 0;
}

/*!
 * @brief waypointの番号をセットする　【未チェック関数】
 * 途中から開始するために実装したが，動作するかどうかをチェックしていない．
 *
 * @param[in] num waypointの番号
 *
 * @return -1:playモードではない．0:正常終了
 */
int navi::setStep(int num)
{
	if (!is_play) return -1;
	step = num;
	seek = getSeek(num);
	
	return 0;
}

/*!
 * @brief waypointの番号を取得する
 *
 * @return waypointの番号
 */
int navi::getStep()
{
	return step;
}

/*!
 * @brief 障害物の位置データの設定
 *
 * [record]
 * 障害物の位置データの保存
 * [play]
 * waypoint間の障害物の位置データを保存して，自己位置推定が行えるように準備する
 * MAX_DATA(10000)以上のデータは無視される．
 *
 * @param[in] p 障害物の位置データのポインタ
 * @param[in] num 障害物の位置データの数
 *
 * @return 現在保存しているデータの数(playの場合は0)
 */
int navi::setData(pos *p, int num)
{
	if (is_record) saveNextData(p, num);
	if (is_play){								// MAX_DATAまでデータを追加していく
		num = min(num, MAX_DATA - data_no);
		for(int i = 0; i < num; i ++){
			data[data_no + i] = p[i];
		}
		data_no += num;
	}

	return data_no;
}

/*!
 * @brief 推定した位置の取得
 *
 * @param[out] x   推定したx座標の値(m)
 * @param[out] y   推定したy座標の値(m)
 * @param[out] the 推定した回転角度(rad)
 *
 * @return 0
 */
int navi::getEstimatedPosition(float *x, float *y, float *the)
{
	*x   = estX  ;
	*y   = estY  ;
	*the = estThe;

	return 0;
}

/*!
 * @brief 参照データの取得
 * 表示及び検証のために，現在参照している障害物の位置データを戻す
 *
 * @param[out] p 参照している障害物の位置データ
 * @param[out] num 位置データの数
 *
 * @return 0
 */
int navi::getRefData(pos *p, int *num, int max_num)
{
	*num = min(max_num, MAX_REF_DATA);
	pos *q = refData;
	for(int i = 0; i < *num; i ++){
		*p ++ = *q ++;
	}

	return 0;
}

/*!
 * @brief waypointの取得
 *
 * @param[out] x waypointのx座標の値(m)
 * @param[out] y waypointのy座標の値(m)
 * @param[out] the waypointの角度(rad)
 * @param[out] period 到達までの時間(sec) 【未使用】
 *
 * @return 0
 */
int navi::getTargetPosition(float *x, float *y, float *the, float *period)
{
	*x = tarX;
	*y = tarY;
	*the = tarThe;
	*period = 0;		// 要変更

	return 0;
}


/*!
 * @brief データを保存するファイル名を指定する
 *
 * @param[in] filename ファイル名(NULLの場合自動的にファイル名が生成される)
 *
 * @return 0
 */
int navi::setTargetFilename(char *filename)
{
	if (filename != NULL){
		strcpy(target_filename, filename);
	} else {
		char s[100];
		time_t timer = time(NULL);
		struct tm *date = localtime(&timer);
		sprintf(s, "navi%04d%02d%02d%02d%02d.csv", date->tm_year+1900, date->tm_mon+1, date->tm_mday, date->tm_hour, date->tm_min);
		strcpy(target_filename, s);
	}
	return 0;
}

/*!
 * @brief オドメトリの保存
 *
 * @param[in] x オドメトリのx座標(m)
 * @param[in] y オドメトリのy座標(m)
 * @param[in] the オドメトリの角度(rad) -PI～PI
 *
 * @return 
 */
int navi::saveNextOdometory(float x, float y, float the)
{
	FILE *fp;

	fp = fopen(target_filename, "a");
	fprintf(fp, "o, %d, %d, %d\n", (int)(x * 1000), (int)(y * 1000), (int)(the * 180 / M_PI));
	fclose(fp);

	return 0;
}

/*!
 * @brief 障害物の位置データの保存
 *
 * @param[in] p 障害物の位置データのポインタ
 * @param[in] num 障害物の数
 *
 * @return 0
 */
int navi::saveNextData(pos *p, int num)
{
	FILE *fp;

	fp = fopen(target_filename, "a");
	for(int i = 0; i < num; i ++){
		fprintf(fp, "u, %d, %d, %d\n", p[i].x, p[i].y, p[i].z);
	}
	fclose(fp);

	return 0;
}

/*!
 * @brief 次のwaypointと障害物の距離データを読み込む
 *
 * @param[out] x waypointのx座標(m)
 * @param[out] y waypointのy座標(m)
 * @param[out] the waypointの角度(rad) -PI～PI
 * @param[out] p 障害物の位置データ
 * @param[out] num 障害物の数
 * @param[out] max_num 取得する障害物の最大値 (numはこれ以上の数にならない)
 *
 * @return 0:次の目標位置がある場合，-1:次の目標位置が無い場合（ゴールに到着）
 */
int navi::loadNextOdoAndData(float *x, float *y, float *the, pos *p, int *num, int max_num)
{
	FILE *fp;
	long seek0;
	char c;
	int d0 = 0, d1 = 0, d2 = 0;
	int n = 0, is_first_odo = 1;
	int ret = -1;

	fp = fopen(target_filename, "r");
	fseek(fp, seek, SEEK_SET);
	while(true){
		seek0 = ftell(fp);
		if (EOF == fscanf(fp, "%c, %d, %d, %d", &c, &d0, &d1, &d2)) break;
		ret = 0;
		if (c == 'o'){
			if (is_first_odo == 0) break;			// ２個目のオドメトリで終了
			*x   = (float)d0 / 1000.0f;
			*y   = (float)d1 / 1000.0f;
			*the = (float)d2 * M_PI / 180.0f; 
			is_first_odo = 0;
		} else if (c == 'u'){
			if (n < max_num){
				p[n].x = d0;
				p[n].y = d1;
				p[n].z = d2;
				n ++;
			}
		}
	}
	fclose(fp);

	seek = seek0;									// オドメトリの直前の番地を保存
	*num = n;

	return ret;
}

/*!
 * @brief 指定したwaypointの番号まで，ファイルの読み込み位置を移動する　【未検証】
 *
 * @param[in] num waypointの番号
 *
 * @return ファイルの読み込み位置
 */
long navi::getSeek(int num)
{
	FILE *fp;
	long seek0;
	char s[100];
	int d0, d1, d2;
	int n = 0;

	if (num == 0) return 0;

	fp = fopen(target_filename, "r");
	while(true){
		seek0 = ftell(fp);
		fscanf(fp, "%s, %d, %d, %d\n", s, &d0, &d1, &d2);
		if (!strcmp(s, "o")){
			n ++;
			if (n >= num) break;
		}
	}
	fclose(fp);

	return seek0;
}

/*!
 * @brief 保存モードの設定
 * 保存モードを選択した場合，再生モードは，解除される．
 *
 * @param[in] is_recode 保存モードにするかのフラグ(1:保存モード，0:保存モードを解除)
 *
 * @return 0
 */
int navi::setRecordMode(int is_record)
{
	if (is_record) is_play = 0;				// 保存と再生の排他処理
	this->is_record = is_record;
	
	return 0;
}

/*!
 * @brief 再生モードの設定
 * 再生モードを選択した場合，保存モードは，解除される．
 *
 * @param[in] is_play 再生モードにするかのフラグ(1:再生モード，0:再生モードを解除)
 *
 * @return 0
 */
int navi::setPlayMode(int is_play)
{
	if (is_play) is_record = 0;				// 保存と再生の排他処理
	this->is_play = is_play;

	return 0;
}

/*!
 * @brief 再生モードかどうかを戻す．
 *
 * @return 0:playモードではない．1:playモード
 */
int navi::isPlayMode()
{
	return is_play;
}

/*!
 * @brief waypointに近づいたかどうかをチェックする関数
 *
 * @param[in] x ロボットのx座標の位置(m)
 * @param[in] y ロボットのy座標の位置(m)
 * @param[in] the ロボットの角度(rad)
 *
 * @return 0:近づいていない，1:近づいている
 */
int navi::isPassTarget(float x, float y, float the)
{
	const float MARGIN = -0.5f;				// どの程度前に来たらウェイポイント通過とみなすか
	float dx, dy, tx, ty;

	dx = x - tarX;							// targetからの相対位置に変換
	dy = y - tarY;
				
	// さらにtargetの向きに合わせて変換
	tx =   dx * cos(tarThe) + dy * sin(tarThe);
	ty = - dx * sin(tarThe) + dy * cos(tarThe);

	return (tx >= MARGIN);					// ロボットのx座標が0.3を超えていればパス
}

/*!
 * @brief 探索対象物に近づいたかどうかをチェックする関数
 *
 * @param[in] x ロボットのx座標の位置(m)
 * @param[in] y ロボットのy座標の位置(m)
 * @param[in] the ロボットの角度(rad)
 *
 * @return 0:近づいていない，1:近づいている
 */
int navi::isPassSearchObject(float x, float y, float the)
{
	const float MARGIN = -0.5f;				// どの程度前に来たらウェイポイント通過とみなすか
	float dx, dy, tx, ty;

	dx = x - searchX;						// 探索対象からの相対位置に変換
	dy = y - searchY;
				
	// さらにtargetの向きに合わせて変換
	tx =   dx * cos(tarThe) + dy * sin(tarThe);
	ty = - dx * sin(tarThe) + dy * cos(tarThe);

	return (tx >= MARGIN);					// ロボットのx座標が0.3を超えていればパス
}


/*!
 * @brief スレッドのエントリーポイント
 *
 * @param[in] lpParameter インスタンスのポインタ
 * 
 * @return S_OK
 */
DWORD WINAPI navi::ThreadFunc(LPVOID lpParameter) 
{
	return ((navi*)lpParameter)->ExecThread();
}

/*!
 * @brief 別スレッドで動作する関数
 * パーティクルフィルタの計算を別スレッドで行う．
 *
 * @return S_OK
 */
DWORD WINAPI navi::ExecThread()
{
	for(int i = 0; i < 10;i ++){	// コンピュータの計算能力により調整する
		est_pos.calcualte();
	}

	return S_OK;
}

/*!
 * @brief waypointに向かうロボットの速度と回転半径を求める
 *
 * @param[out] front 前後方向の速度(m/s)
 * @param[out] radius 回転半径(m)
 *
 * @return 0
 */
int navi::getTargetArcSpeed(float *front, float *radius)
{
#ifdef MEGA_ROVER_1_1
	const float MAX_SPEED = 0.5f;
#else
	const float MAX_SPEED = 0.3f;
#endif

	float dx0, dy0, dx, dy;
	
	dx0 = tarX - estX;
	dy0 = tarY - estY;
	
	dx =   dx0 * cos(estThe) + dy0 * sin(estThe);
	dy = - dx0 * sin(estThe) + dy0 * cos(estThe);

	if (dy == 0){
		if (dx != 0){
			dy = dx * 0.001f;
		} else {
			*radius = *front = 0;
			return -1;
		}
	}

	*radius = (dx * dx + dy * dy) / (2.0f * dy);
	*front = min(fabs(*radius) * atan2(dx, fabs(*radius - dy)), MAX_SPEED);

	return 0;
}

/*!
 * @brief パーティクルのデータを取得する
 * 表示と検証用にパーティクルのデータを取得する関数
 *
 * @param[out] particle パーティクルのデータのポインタ
 * @param[out] num パーティクルの数
 * @param[out] max_num パーティクルのデータのポインタ
 *
 * @return 0
 */
int navi::getParticle(struct particle_T *particle, int *num, int max_num)
{
	static const int MAX_PARTICLE = 1000;
	static struct particle_T p[MAX_PARTICLE];
	
	est_pos.getParticle(p, num, MAX_PARTICLE);
	struct particle_T *q = p;
	for(int i = 0; i < *num; i ++){
		*particle ++ = *q ++;
	}

	return 0;
}

/*!
 * @brief 一致度を取得
 *
 * @param[out] coincidence 一致度
 *
 * @return 0
 */
int navi::getCoincidence(float *coincidence)
{
	*coincidence = this->coincidence;

	return 0;
}

/*!
 * @brief 探索対象の候補点を設定
 * この関数を呼び出すと暫くこの点に向かってロボットが移動する．
 *
 * @param[in] p 探索対象の候補点
 *
 * @return 0
 */
int navi::setSearchPoint(pos p)
{
	searchX = p.x / 1000.0f;	// 探索する対象物の位置
	searchY = p.y / 1000.0f;	// 探索する対象物の位置
	is_search_object = 1;		// 探索する対象物の有無

	return 0;
}

/*!
 * @brief 探索のプロセス
 *
 * @return 1:継続中，0:終了
 */
int navi::searchProcess()
{
	const float MARGIN_ANGLE = 0.1f;		// 次のモードに移る角度誤差(rad)
	const float MARGIN_DIST_HUMAN = 1.0f;	// 人に近づく距離(m)
	const float MARGIN_DIST_RETURN = 0.1f;	// 経路に戻るときに次のモードに移る位置誤差(m)

	enum SEQUENCE {
		INIT = 0,
		TURN_TARGET,
		MOVE_TARGET,
		INDICATE_FIND,
		INDICATE_FIND_WAIT,
		TURN_RETURN,
		MOVE_WAYPOINT,
		TURN_WAYPOINT,
		FINISH
	};
	
	int res = 1;

	static long time_mode0 = 0;				// 前回modeを切り替えた時の時間(ms)
	static int search_mode0 = -1;			// 前回のsearch_mode
	if (search_mode != search_mode0){
		search_mode0 = search_mode;
		time_mode0 = timeGetTime();
	}
	float mode_period = (timeGetTime() - time_mode0) / 1000.0f;

	LOG("search_mode:%d\n", search_mode);
	if (mode_period < 1.0f) stop();
	else {
		switch(search_mode){
			case INIT:{
				stop();					// 停止
				if (mode_period >= 1.0f) search_mode ++;
				break;
				   }
			case TURN_TARGET:{
				if (!turnToPos(searchX, searchY, MARGIN_ANGLE)) search_mode ++;
				break;
				   }
			case MOVE_TARGET:{
				if (mode_period >= 10.0f) search_mode ++;				// 10秒間経過しても条件を満たさない場合は，次のモードに移る．
																		// 障害物で停止してしまうことに関する対策
				if (!moveToPos(searchX, searchY, MARGIN_DIST_HUMAN)) search_mode ++;
				break;
				   }
			case INDICATE_FIND:{
				PlaySound("find.wav", NULL, SND_FILENAME | SND_ASYNC);
				search_mode ++;
				break;
				   }
			case INDICATE_FIND_WAIT:{
				stop();					// 停止
				if (mode_period >= 2.0f) search_mode ++;
				break;
				   }
			case TURN_RETURN:{
				if (!turnToPos(retX, retY, MARGIN_ANGLE)) search_mode ++;
				break;
				   }
			case MOVE_WAYPOINT:{
				if (!moveToPos(retX, retY, MARGIN_DIST_RETURN)) search_mode ++;
				break;
				   }
			case TURN_WAYPOINT:{
				if (!turnToPos(tarX, tarY, MARGIN_ANGLE)) search_mode ++;
				break;
				   }
			case FINISH:{
				data_no = 0;											// データのクリア
				is_search_mode = 0;
				break;
				   }
		}
	}

	return res;
}

/*!
 * @brief 停止する（探索モードのみ）
 *
 * @return 0
 */
int navi::stop()
{
	forwardSpeed = 0.0f;
	rotateSpeed  = 0.0f;

	return 0;
}

/*!
 * @brief 目標(x,y)に向き直る
 *
 * @param[in] x 目標のx座標(m) グローバル座標
 * @param[in] y 目標のy座標(m) グローバル座標
 * @param[in] margin_angle 終了する角度誤差(rad)
 *
 * @return 1:継続中，0:終了
 */
int navi::turnToPos(float x, float y, float margin_angle)
{
	const float ROTATE_SPEED = 0.5f;
	
	int ret = 1;
	float ang = atan2(y - estY, x - estX);
	float ang_err = maxPI(ang - estThe);
	
	forwardSpeed = 0.0f;
	if (fabs(ang_err) > margin_angle){
		if (ang_err > 0.0f) rotateSpeed =  ROTATE_SPEED;
		else				rotateSpeed = -ROTATE_SPEED;
	} else {
		rotateSpeed = 0.0f;
		ret = 0;				// タスクが終了
	}

	return ret;
}

/*!
 * @brief 目標(x,y)に近寄る
 *
 * @param[in] x 目標のx座標(m) グローバル座標
 * @param[in] y 目標のy座標(m) グローバル座標
 * @param[in] margin_distance 終了する距離(m)
 *
 * @return 1:継続中，0:終了
 */
int navi::moveToPos(float x, float y, float margin_distance)
{
	const float FORWARD_SPEED = 0.15f;
	const float ROTATE_GAIN = 0.3f;
	
	int ret = 1;
	float dx = x - estX, dy = y - estY;
	float dist = sqrt(dx * dx + dy * dy);
	float ang = atan2(dy, dx);
	float ang_err = maxPI(ang - estThe);
	
	rotateSpeed = ang_err * ROTATE_GAIN;
	if (dist > margin_distance){
		forwardSpeed = FORWARD_SPEED;
	} else {
		forwardSpeed = 0.0f;
		rotateSpeed = 0.0f;
		ret = 0;
	}

	return ret;
}

/*!
 * @brief 探索モードかどうかを戻す(）
 *
 * @return 0:探索モードでない，1:探索モード
 */
int navi::isSearchMode()
{
	return  is_search_mode;
}

/*!
 * @brief 速度の指令値を戻す
 *
 * @param[out] front  前後の速度(m/s)
 * @param[out] radius 回転半径(m)
 *
 * @return 0
 */
int navi::getSpeed(float *forward, float *rotate)
{
	*forward = forwardSpeed;
	*rotate  = rotateSpeed;

	return 0;
}

/*!
 * @brief 前回の探索対象からの距離を戻す
 *
 * @return 前回の探索対象からの距離(m)
 */
float navi::distaceFromPreviousSearchPoint()
{
	float dx = estX - searchX;
	float dy = estY - searchY;

	return sqrt(dx * dx + dy * dy);
}

/*!
 * @brief リルートモードにする．
 *
 * @param[in] direction リルートの方向
 *
 * @return 0
 */
int navi::setRerouteMode(int direction)
{
	is_reroute_mode = 1;
	reroute_direction = direction;
	reroute_mode = 0;

	return 0;
}

int navi::isRerouteMode()
{
	return is_reroute_mode;
}

/*!
 * @brief 探索のプロセス
 *
 * @return 1:継続中，0:終了
 */
int navi::rerouteProcess()
{
	LOG("reroute_process, reroute_mode : %d\n", reroute_mode);


	const float MARGIN_ANGLE = 0.1f;		// 次のモードに移る角度誤差(rad)
	const float MARGIN_DIST_HUMAN = 1.0f;	// 人に近づく距離(m)
	const float MARGIN_DIST_RETURN = 0.1f;	// 経路に戻るときに次のモードに移る位置誤差(m)

	enum SEQUENCE {
		INIT = 0,
		TURN_SIDE,
		MOVE_SIDE,
		TURN_FORWARD,
		MOVE_FORWARD,
		TURN_RETURN,
		MOVE_RETURN,
		TURN_WAYPOINT,
		FINISH
	};
	
	int res = 1;

	static long time_mode0 = 0;				// 前回modeを切り替えた時の時間(ms)
	static int reroute_mode0 = -1;			// 前回のsearch_mode
	if (reroute_mode != reroute_mode0){
		reroute_mode0 = reroute_mode;
		time_mode0 = timeGetTime();
	}
	float mode_period = (timeGetTime() - time_mode0) / 1000.0f;

	LOG("reroute_mode:%d\n", reroute_mode);
	if (mode_period > 6.0f) reroute_mode ++;
	if (mode_period < 1.0f){
		stop();
		rerouteX0 = estX, rerouteY0 = estY, rerouteThe0 = estThe;
	} else {
		switch(reroute_mode){
			case INIT:{
				reroute_mode ++;
				break;
					}
			case TURN_SIDE:{
				if (!turn90deg(reroute_direction, rerouteThe0, MARGIN_ANGLE)) reroute_mode ++;
				break;
					}
			case MOVE_SIDE:{
				if (!moveForward(0.282, rerouteX0, rerouteY0)) reroute_mode ++;
				break;
					}
			case TURN_FORWARD:{
				if (!turn90deg(-reroute_direction, rerouteThe0, MARGIN_ANGLE)) reroute_mode ++;
				break;
					}
			case MOVE_FORWARD:{
				if (is_need_stop) reroute_mode = TURN_SIDE;
				if (!moveForward(0.5, rerouteX0, rerouteY0)) reroute_mode ++;
				break;
					}
			case TURN_RETURN:{
				if (!turn90deg(-reroute_direction, rerouteThe0, MARGIN_ANGLE)) reroute_mode ++;
				break;
					}
			case MOVE_RETURN:{
				if (!moveForward(0.282, rerouteX0, rerouteY0)) reroute_mode ++;
				break;
					}
			case TURN_WAYPOINT:{
				if (!turn90deg(reroute_direction, rerouteThe0, MARGIN_ANGLE)) reroute_mode ++;
				break;
					}
			case FINISH:{
				data_no = 0;
				is_reroute_mode = 0;
				res = 0;
				break;
					}
		}
	}

	return res;
}

/*!
 * @brief 90度回転する．
 *
 * @return 1:継続中，0:終了
 */
int navi::turn90deg(int direction, float the0, float margin_angle)
{
	const float ROTATE_SPEED = 0.5f;
	
	int ret = 1;
	float ang;
	if (direction == RIGHT){
		ang = the0 - M_PI / 2.0f;
	} else {
		ang = the0 + M_PI / 2.0f;
	}
	float ang_err = maxPI(ang - estThe);
	
	forwardSpeed = 0.0f;
	if (fabs(ang_err) > margin_angle){
		if (ang_err > 0.0f) rotateSpeed =  ROTATE_SPEED;
		else				rotateSpeed = -ROTATE_SPEED;
	} else {
		rotateSpeed = 0.0f;
		ret = 0;				// タスクが終了
	}

	return ret;
}

/*!
 * @brief ある距離だけ走行
 *
 * @return 1:継続中，0:終了
 */
int navi::moveForward(float length, float x0, float y0)
{
	const float FORWARD_SPEED = 0.15f;
	
	int ret = 1;
	float dx = estX - x0, dy = estY - y0;
	float dist = sqrt(dx * dx + dy * dy);
	
	rotateSpeed = 0;
	if (dist < length){
		forwardSpeed = FORWARD_SPEED;
	} else {
		forwardSpeed = 0.0f;
		rotateSpeed = 0.0f;
		ret = 0;
	}

	return ret;
}

/*!
 * @brief 減速の係数をセットする．
 *
 * @return 0
 */
int navi::setNeedStop(int is_need_stop)
{
	this->is_need_stop = is_need_stop;

	return 0;
}

