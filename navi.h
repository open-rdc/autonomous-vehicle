#pragma once

#include "estimatePos.h"

class navi
{
public:
	navi();							// コンストラクタ
	virtual ~navi();				// デストラクタ

private:
	int is_record;					//! 保存モードかどうかのフラグ
	int is_play;					//! 再生モードかどうかのフラグ
	int step;						//! waypointの番号
	float step_period;				//! 次のwaypointに到達するまでの時間
	long time0;						//! 前回waypointを通過した時間(ms)

	// ファイル入出力関連
	char target_filename[256];		//! データを保存するファイル名
	long seek;						//! データを参照している場所
	long getSeek(int num);			// 指定したwaypointの番号まで，ファイルの読み込み位置を移動する
	int saveNextOdometory(float x, float y, float the);	// オドメトリの保存
	int saveNextData(pos *p, int num);					// 障害物の位置データの保存
	int loadNextOdoAndData(float *x, float *y, float *the, pos *p, int *num, int max_num);
									// 次のwaypointと障害物の距離データを読み込む
	
	// target関連
	float tarX, tarY, tarThe;		// 次のwaypointの位置(m,rad)
	int isPassTarget(float x, float y, float the);
									// waypointに近づいたかどうかをチェックする関数

	// 計測した障害物の位置データの一時保管場所
	static const int MAX_DATA = 10000;	//! 障害物の位置データの最大個数
	int data_no;						//! 障害物の位置データの個数
	pos data[MAX_DATA];					//! 所外物のデータ

	// 参照する障害物の位置データの一時保管場所
	static const int MAX_REF_DATA = 10000;	//! 参照する障害物の位置データの最大個数
	int ref_data_no;						//! 参照する障害物の位置データの個数
	pos refData[MAX_REF_DATA];				//! 参照する障害物の位置データ

	// オドメトリ
	float odoX0, odoY0, odoThe0;	//! 一つ前のウェイポイントを通過した時のオドメトリ(m, rad)

	// 推定値
	float estX0, estY0, estThe0;	//! 一つ前のウェイポイントを通過した時の位置の推定値(m,rad)
	float estX,  estY,  estThe ;	//! 現在の位置の推定値

	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);	// 計測のスレッド
	DWORD WINAPI ExecThread();
	DWORD threadId;					//! スレッド ID	
	HANDLE hThread;					//! スレッドのハンドル

	estimatePos est_pos;			//! 自己位置推定のクラスのインスタンス
	float coincidence;				//! 一致度 (0-1)
	int selfLocalization(float x, float y, float the, float dx, float dy, float dthe);
									// 自己位置推定
	// 探索用の処理
	int is_search_object;			//! 探索対象者の有無
	int is_search_mode;				//! 探索対象者を探索するモード
	int search_mode;				//! 探索のモード（シーケンスの処理に使用する）
	float searchX, searchY;			//! 探索する位置
	float retX, retY;				//! 探索モードから戻る位置（グローバル座標）
	float forwardSpeed, rotateSpeed;//! 速度目標(m/s)
	int isPassSearchObject(float x, float y, float the);	// 探索対象物に近づいたかどうかをチェックする関数
	int searchProcess();									// 探索のプロセス
	int stop();												// 停止する（探索モードのみ）
	int turnToPos(float x, float y, float margin_angle);	// 目標(x,y)に向き直る
	int moveToPos(float x, float y, float margin_distance);	// 目標(x,y)に近寄る

	int is_reroute_mode;
	int reroute_direction;
	int reroute_mode;
	float rerouteX0, rerouteY0, rerouteThe0;				// リルートを始めた時のx,y,the
	int rerouteProcess();
	int turn90deg(int direction, float the0, float margin_angle);
	int moveForward(float length, float x0, float y0);

	static const int RIGHT = -1, CENTER = 0, LEFT = +1;	//! 右，中央，左の定数
	int is_need_stop;

public:
	int Init();						// 初期化
	int Close();					// 終了処理
	int setOdometory(float x, float y, float the);	// オドメトリを設定する
	int setStep(int num);			// waypointの番号をセットする
	int getStep();					// waypointの番号を取得する
	int setData(pos *p, int num);	// 障害物の位置データの設定
	int getEstimatedPosition(float *x, float *y, float *the);				// 推定した位置の取得
	int getTargetPosition(float *x, float *y, float *the, float *period);	// waypointの取得
	int getTargetArcSpeed(float *front, float *radius);						// waypointに向かうロボットの速度と回転半径を求める
	int getParticle(struct particle_T *particle, int *num, int max_num);	// パーティクルのデータを取得する
	int getCoincidence(float *coincidence);									// 一致度を取得
	int setTargetFilename(char *filename = NULL);							// データを保存するファイル名を指定する
	int setRecordMode(int is_record);										// 保存モードの設定
	int setPlayMode(int is_play);											// 再生モードの設定
	int isPlayMode();														// PlayModeかどうかを戻す
	int getRefData(pos *p, int *num, int max_num);							// 参照データの取得
	int setSearchPoint(pos p);												// 探索対象の候補点を設定
	int isSearchMode();														// 探索モードかどうかを戻す(0:探索モードでない，1:探索モード）
	int getSpeed(float *forward, float *rotate);							// 直接ホイールの速度を司令する．(探索モードで使用)
	float distaceFromPreviousSearchPoint();									// 前回の探索対象からの距離を戻す(m)
	int setRerouteMode(int direction);										// リルートモードにする．
	int isRerouteMode();
	int setNeedStop(int is_need_stop);
};

/*
 * 使い方
 * 
 * ■保存(record)
 * 1) setTargetFilename("temp.csv")などでファイル名を指定（時間などを自動的に入れるようにしたい）
 * 2) setRecordMode(1)で保存モードにする．
 * 3) データを取得するごとに
 *    setOdometory(x,y,the), setData(p,num)でファイルを保存（共にワールド座標）
 * 4) setRecordMode(0)で保存モードを解除する．（必ずしも必要ない）
 * 
 * 途中からの保存には対応していない（要検討）
 *
 * ■再生(play)
 * 1) setTargetFilename("temp.csv")などでファイル名を指定（時間などを自動的に入れるようにしたい）
 * 2) setPlayMode(1)で再生モードにする．
 * 3) setOdometory(x,y,the)で現在位置を入力（ワールド座標系）
 * 4) setData(p, num)で計測データを入力（ワールド座標系）
 * 5) getEstimatedPosition(&x,&y,&the)で現在位置を取得（ワールド座標系）
 * 6) getTargetPosition(&x,&y,&the,&period)で目標位置を取得（ワールド座標系）
 * 7) getTargetArcSpeed(&front,&radius)を取得，これに従いホイールを制御
 * 8) 3)に戻る
 * 9) setPlayMode(0)で再生モードを解除する，（かなずしも必要ない）
 *
 */

/*
 * ■再生(play)時の処理手順
 *
 * ●ゴールのラインを通過する毎に
 * 1)前々回のゴールライン通過時の推定位置をベースとして現在位置を算出する
 * 2)次のゴールに到達するための，目標速度，旋回半径を求める．
 *
 * 3) 前回のオドメトリの【推定位置】をsetPosition(float x, float y, float the)に入れる
 * 4) 前回から保存したデータを最新のオドメトリ【位置】の相対座標に変換
 * 5) estimatePos::setDataにデータを入力（ロボット座標)
 * 6) 前回からのオドメトリの差分をestimatePos::setOdometoryに入力（ワールド座標）
 *    【注意】移動距離は車輪から，方向はオドメトリの【推定位置】から計算する
 * 7) estimatePos::calculate()で計算
 * 8) estimatePos::getOdometory(&x,&y,&the,&var,&coincidence)で推定位置を取得
 * 9) varが小さく，coincidenceが大きい場合は，推定したオドメトリの位置を修正
 *    そうでない場合は，単純にオドメトリの【推定位置】に差分を足す
 *
 *    これを自己位置として，ゴールに向かうための目標軌道を生成して次のゴールラインまで制御する
 */

/*
 * オドメトリの種類
 * 1) record時のオドメトリ
 * 2) play時のオドメトリ
 * 3) play時にrecord時のオドメトリに合わせたオドメトリ
 */
