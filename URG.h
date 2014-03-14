#pragma once
#include "comm.h"
#include "dataType.h"

// CURG
class CURG
{

public:
	CURG();								// コンストラクタ
	virtual ~CURG();					// デストラクタ

public:
	static const int n_data = 641;		// URGで取得するデータの個数 (10～170deg)
	CComm comm;							// 通信のクラス

	int Init(int com_port);				// 初期設定
	int Close();						// 終了処理
	int StartMeasure();					// URGの計測開始
	int GetData(int length[n_data], int intensity[n_data]);
										// 受信バッファにたまった距離データを取得する
	int TranslateCartesian(float tilt, int data[n_data], pos p[n_data]);
										// デカルト座標系への変換
};
