#pragma once

// 共通のデータ型を定義

/*!
 * @struct pos_T
 * @brief 位置データ
 */
struct pos_T{
	int x;				//!< x座標(mm)
	int y;				//!< y座標(mm)
	int z;				//!< z座標(mm)
};
typedef struct pos_T pos;

/*!
 * @struct pos_inten_T
 * @brief 反射強度付き位置データ
 */
struct pos_inten_T{
	pos pos;			// 位置データ(mm)
	int intensity;		// 反射強度（単位なし）
};
typedef struct pos_inten_T pos_inten;

/*!
 * @struct pos_slate_T
 * @brief 探索対象の候補の位置データ
 * 確率(0-1)付き
 */
struct pos_slate_T{
	pos pos;			// 位置データ(mm)
	float probability;	// 確率(0.0-1.0)
};
typedef struct pos_slate_T pos_slate;

/*!
 * @struct particle_T
 * @brief パーティクルのデータ
 */
struct particle_T{
	float x;			//!< x座標(m)
	float y;			//!< y座標(m)
	float the;			//!< 角度(rad)
	int eval;			//!< 評価(0-)
};

/*!
 * @struct odometory
 * @brief オドメトリのデータ
 */
struct odometory{
	float x;			//!< x座標(m)
	float y;			//!< y座標(m)
	float the;			//!< 角度(rad)
};
