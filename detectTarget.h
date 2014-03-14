#pragma once
#include "dataType.h"

class detectTarget
{
public:
	detectTarget(void);								// コンストラクタ
	~detectTarget(void);							// デストラクタ

private:
	// 反射強度データ
	static const int MAX_INTENSITY_DATA = 10000;	//! 反射強度のデータの最大個数
	int intensity_data_no;							//! 反射強度のデータの個数
	pos_inten intensity_data[MAX_INTENSITY_DATA];	//! 反射強度のデータ

	// 探索対象の候補
	static const int MAX_SLATE_POINT = 100;			//! 探索対象の候補の最大個数
	int slate_point_no;								//! 探索対象の候補の個数
	pos_slate slate_point[MAX_SLATE_POINT];			//! 探索対象の候補

	// 探索対象
	static const int MAX_SEARCH_POINT = 100;		//! 探索点の最大個数
	int search_point_no;							//! 探索点の個数
	pos_slate search_point[MAX_SEARCH_POINT];		//! 探索点のデータ

	/*!
	 * @struct pos_integrate_T
	 * @brief 座標点統合用構造体
	 */
	struct pos_integrate_T{
		pos pos;									// 位置データ
		int count;									// 統合した点の個数
	};
	typedef struct pos_integrate_T pos_integrate;	// 座標点統合用の型
	pos_integrate integrated_point[MAX_INTENSITY_DATA];	// 座標点統合用のデータ
	int integrated_point_no;						// 座標点統合用のデータの個数

	float distance_xy2(pos p, pos q);				// 点の距離を求める(m)
	static int comp_inten(const void *c1, const void *c2);	// pos_inten型のソートのための比較関数
	static int comp_slate(const void *c1, const void *c2);	// pos_slate型のソートのための比較関数
	int integratePoints(pos_inten *p, int num, pos_integrate *q, int *num_pos_integrate, float radius);
													// 反射強度の大きい点を統合する

	int terminate;									//! スレッドを破棄（1:破棄, 0:継続）
	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);		// スレッドのエントリーポイント
	DWORD WINAPI ExecThread();						// 別スレッドで動作する関数
	HANDLE mutex;									//! 排他制御のハンドル

public:
	int Init();										// 初期化
	int Close();									// 終了処理
	int addIntensityData(pos_inten *p, int num);	// 反射強度データのセット
	int getTargetPos(pos_slate *p, int *num);		// 探索対象の位置と確率セット
	int getSearchPos(pos *p, pos self_loc, float radius);			// 探索範囲内の探索対象の位置（0:探索対象無し，1:有り）
	int calculateIntensity();						// 反射強度データのセットから探索対象の候補を選定
	int calculateSearchPoint();						// 探索対象ポイントを計算
	int update();									// 周期的な処理（1s程度）
};
