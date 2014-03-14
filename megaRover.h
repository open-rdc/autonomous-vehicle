#pragma once

#include <windows.h>

#define MEGA_ROVER_1_1

#define UP_BUTTON		0x10
#define DOWN_BUTTON		0x40
#define RIGHT_BUTTON	0x20
#define LEFT_BUTTON		0x80

class megaRover
{
private:
	static const int TREAD = 280;				//! トレッド（ホイールの距離）(mm)
	const float MAX_SPEED;						//! 最大速度(m/s)

	int is_speed_control_mode;					//! 速度制御モード（1:速度制御モード，0:その他）
	float refSpeedRight, refSpeedLeft;			//! 左右ホイールの目標速度(m/s)
	float speedRight, speedLeft;				//! 左右ホイールの速度(m/s)
	float odoX, odoY, odoThe;					//! オドメトリの位置(m)，姿勢(rad)
	int deltaR, deltaL;							//! 左右ホイールの加速度(m/s^2)
	int terminate;								//! スレッドの破棄（1:破棄, 0:継続）
	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);	// スレッドのエントリーポイント
	DWORD WINAPI ExecThread();					// 別スレッドで動作する関数
	int Update();								// 周期的に行う処理
	HANDLE mutex, comMutex;						// COMポートの排他制御
	int getEncoder(unsigned int *right, unsigned int *left);
												// エンコーダの値の取得
public:
	megaRover();								// コンストラクタ
	~megaRover();								// デストラクタ

	int init();									// 初期化
	int close();								// 終了処理
	int servoOn(int gain);						// サーボオン(gain:100)
	int setDelta(int right, int left);			// 変化量を指定
	int setMotor(int right, int left);			// モータへのトルク入力(-127-127)

	// speed control mode
	int setSpeedControlMode(int is_on);			// 速度制御モードへの切り替え(1:速度制御，0:オフ)
	int setSpeed(float front, float rotate);	// ホイールの目標速度(m/s)
	int setArcSpeed(float front, float radius);	// ロボットの目標速度(前後，回転)
	int getOdometory(float *x, float *y, float *the, int is_clear);	// オドメトリの取得(m, rad) 1:クリア
	int getJoyStick(float *x, float *y, int *b);
												// ジョイスティック情報の取得
	int getReferenceSpeed(float *right, float *left);
	int setOdometoryAngle(float angle);			// ジャイロオドメトリのために方位を設定する(rad)
	int getSpeed(float *rightSpeed, float *leftSpeed);
	int megaRover::clearOdometory();			// オドメトリの値をクリアする．
};
