#pragma once
#include "comm.h"

#define	IMU_COM_PORT	4

class imu
{
public:
	imu();										// コンストラクタ
	virtual ~imu();								// デストラクタ

public:
	CComm comm;									// 通信ポートのクラス

	int Init(int com_port);						// 初期化
	int Close();								// 終了処理
	int Reset();								// IMUのリセット
	int GetAngleStart();						// 角度の取得の開始
	int GetAngle(float *x, float *y, float *z);	// 角度の取得
};
