#include "stdafx.h"
#include <math.h>
#include <mmsystem.h>
#include "memoryMap.h"
#include "megaRover.h"
#include "Library/WRC003LVHID.h"

#define	M_PI	3.14159f

#pragma	comment(lib,"Library/CWRC003LVHID.lib")

/*!
 * @class megaRover
 * @brief Mega Roverを制御するためのクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
megaRover::megaRover():
is_speed_control_mode(0), refSpeedRight(0), refSpeedLeft(0),terminate(0),
odoX(0), odoY(0), odoThe(0),deltaL(0),deltaR(0), mutex(NULL), comMutex(NULL),
#ifdef MEGA_ROVER_1_1
	MAX_SPEED(0.625f)
#else
	MAX_SPEED(0.3)
#endif
{
}

/*!
 * @brief デストラクタ
 */
megaRover::~megaRover()
{
}

/*!
 * @brief 初期化
 *
 * @return 0
 */
int megaRover::init()
{
	if(!CWRC_Connect()) return -1;

	// 排他処理
	mutex = CreateMutex(NULL, FALSE, _T("MEGA_ROVER_ODOMETORY"));
	comMutex = CreateMutex(NULL, FALSE, _T("MEGA_ROVER_COM_MUTEX"));

	// エンコーダの初期化
	SetMem_UByte(Flag, 0x02);	// エンコーダをONにする
	CWRC_WriteExecute(FALSE);

	SetMem_SWord(EncoderA, 0);	// 現在のエンコーダ値を0に戻す
	SetMem_SWord(EncoderB, 0);
	CWRC_WriteExecute(FALSE);

	// 速度制御のスレッドを開始
	DWORD threadId;	
	HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, &threadId); 
	// スレッドの優先順位を上げる
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	
	return 0;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int megaRover::close()
{
	terminate = 1;				// 速度制御スレッドの停止
	CloseHandle(mutex);
	CloseHandle(comMutex);
	setMotor(0, 0);				// ロボットを止める
	servoOn(0);					// モータをOFFにする
	CWRC_Disconnect();			// ロボットとの通信を切断する

	return 0;
}

/*!
 * @brief サーボON/OFF
 *
 * @param[in] gain 0:OFF, 1-255:on (servo gain)
 *
 * @return 0
 */
int megaRover::servoOn(int gain)
{
	// モータをONにするためには、メモリマップの0x04（Mode）を1に、0x08,0x09（左右車輪のゲイン）を一定以上の値に書き換えます
	SetMem_UByte(CpuMode     , gain > 0);
	SetMem_UByte(MotorGainCh1, gain);
	SetMem_UByte(MotorGainCh2, gain);
	if (comMutex == NULL) return -1;
	WaitForSingleObject(comMutex, INFINITE); 
	CWRC_WriteExecute(FALSE);
	ReleaseMutex(comMutex);

	return 0;
}

/*!
 * @brief 加速度の設定
 * 急な加減速を防止するため
 *
 * @param[in] right 右ホイールの加速度(m/s^2)
 * @param[in] left  左ホイールの加速度(m/s^2)
 *
 * @return 0
 */
int megaRover::setDelta(int right, int left)
{
	deltaR = right;
	deltaL = left ;

	return 0;
}

/*!
 * @brief モータ速度の設定
 *
 * @param[in] right 右ホイールのトルク　(-1000~1000)
 * @param[in] left  左ホイールのトルク　(-1000~1000)
 *
 * @return 0
 */
int megaRover::setMotor(int right, int left)
{
	static const int limit = 1000;
	static int right0, left0;
	if (deltaR != 0){
		if ((right - right0) > 0){
			right = min(right0 + deltaR, right);
		} else if ((right - right0) < 0){
			right = max(right0 - deltaR, right);
		}
	}
	if (deltaL != 0){
		if ((left - left0) > 0){
			left = min(left0 + deltaL, left);
		} else if ((left - left0) < 0){
			left = max(left0 - deltaL, left);
		}
	}
	
	right = min(max(right, -limit), limit);
	left  = min(max(left,  -limit), limit);

	if (comMutex == NULL) return -1;
	WaitForSingleObject(comMutex, INFINITE); 
//	SetMem_SWord(MotorSpeedCh1, (int)(right * 0.93f));	// ToDo バランスを取る
	SetMem_SWord(MotorSpeedCh1, (int)(right * 1.00f));	// ToDo バランスを取る
	SetMem_SWord(MotorSpeedCh2, (int)(left  * 1.00f));
	CWRC_WriteExecute(FALSE);
	ReleaseMutex(comMutex);

	right0 = right;
	left0  = left;

	return 0;
}

/*!
 * @brief エンコーダの値の取得
 *
 * @param[out] right 右ホイールのエンコーダの値
 * @param[out] left  左ホイールのエンコーダの値
 *
 * @return 0
 */
int megaRover::getEncoder(unsigned int *right, unsigned int *left)
{
	// エンコーダの現在値を取得
	if (comMutex == NULL) return -1;
	WaitForSingleObject(comMutex, INFINITE); 
	CWRC_ReadMemMap(EncoderA, 4);
	CWRC_ReadExecute();

	*right = (unsigned short)GetMem_SWord(EncoderA);
	*left  = (unsigned short)GetMem_SWord(EncoderB);
	ReleaseMutex(comMutex);

	return 0;
}

/*!
 * @brief 速度制御モードの設定
 *
 * @param[in] is_on 1:速度制御モード，0:解除
 *
 * @return 0
 */
int megaRover::setSpeedControlMode(int is_on)
{
	is_speed_control_mode = is_on;

	return 0;
}

/*!
 * @brief 速度の制御
 *
 * @param[in] front 前後方向の速度(m/s)
 * @param[in] rotate 角速度(rad/s)
 *
 * @return 0
 */
int megaRover::setSpeed(float front, float rotate)
{
	refSpeedRight = front + rotate * (float)TREAD / 1000.0f / 2.0f;
	refSpeedLeft  = front - rotate * (float)TREAD / 1000.0f / 2.0f;

	float maxSpeed = max(fabs(refSpeedRight), fabs(refSpeedLeft));
	if (maxSpeed > MAX_SPEED){
		refSpeedRight *= (MAX_SPEED / maxSpeed);						// 超える場合は，回転半径を変えないように両方同じ比率で速度を下げる．
		refSpeedLeft  *= (MAX_SPEED / maxSpeed);
	}

	return 0;
}

/*!
 * @brief 前後の速度と回転半径
 *
 * @param[in] front  前後の速度(m/s)
 * @param[in] radius 回転半径(m)
 *
 * @return 0
 */
int megaRover::setArcSpeed(float front, float radius)
{
	if (radius != 0.0f){
		float w = front / radius;
		refSpeedRight = (radius + (float)TREAD / 1000.0f / 2.0f) * w;
		refSpeedLeft  = (radius - (float)TREAD / 1000.0f / 2.0f) * w;
	}

	float maxSpeed = max(fabs(refSpeedRight), fabs(refSpeedLeft));
	if (maxSpeed > MAX_SPEED){
		refSpeedRight *= (MAX_SPEED / maxSpeed);					// 超える場合は，回転半径を変えないように両方同じ比率で速度を下げる．
		refSpeedLeft  *= (MAX_SPEED / maxSpeed);
	}

	return 0;
}

/*!
 * @brief オドメトリの取得
 *
 * @param[out] x   オドメトリのx座標(m)
 * @param[out] y   オドメトリのy座標(m)
 * @param[out] the オドメトリの角度(rad)
 * @param[in]  is_clear オドメトリの値をクリアするかのフラグ(1:クリア,0:クリアしない)
 *
 * @return 0
 */
int megaRover::getOdometory(float *x, float *y, float *the, int is_clear)
{
	if (mutex == NULL) return -1;
	WaitForSingleObject(mutex, INFINITE);
	*x   = odoX;
	*y   = odoY;
	*the = odoThe;
	if (is_clear){
		odoX = odoY = odoThe = 0;
	}
	ReleaseMutex(mutex);

	return 0;
}

/*!
 * @brief 左右のホイールの速度の取得
 *
 * @param[out] rightSpeed	右のホイールの速度(m)
 * @param[out] leftSpeed	左のオイールの速度(m)
 *
 * @return 0
 */
int megaRover::getSpeed(float *rightSpeed, float *leftSpeed)
{
	*rightSpeed = speedRight;
	*leftSpeed  = speedLeft ;
	return 0;
}

/*!
 * @brief スレッドのエントリーポイント
 *
 * @param[in] lpParameter インスタンスのポインタ
 * 
 * @return S_OK
 */
DWORD WINAPI megaRover::ThreadFunc(LPVOID lpParameter) 
{
	return ((megaRover*)lpParameter)->ExecThread();
}

/*!
 * @brief 別スレッドで動作する関数
 * ロボットの制御を別スレッドで行う．
 *
 * @return S_OK
 */
DWORD WINAPI megaRover::ExecThread()
{
	while(!terminate){
		Update();
		Sleep(20);
	}
	return S_OK; 
}

/*!
 * @brief 定期的(20ms)に呼び出す関数
 *
 * @return 0
 */
int megaRover::Update()
{
	const float coef = 1.0f;					// 要調整個体差あり

	const int MAX_SHORT = 256 * 256;
	static unsigned int r0 = 0, l0 = 0;
	static unsigned long time0 = 0;
	static const float kp = 50000.0f, ki = 1000.0f;
	unsigned int r = 0, l = 0;
	int rd, ld;
	float right, left;
	float v, w;

	getEncoder(&r, &l);
	rd =    r - r0;
	ld  = -(int)(l - l0);	// 左はマイナスが前進
	r0 = r, l0 = l;
	if (rd >= ( MAX_SHORT / 2)) rd -= MAX_SHORT;
	if (rd <= (-MAX_SHORT / 2)) rd += MAX_SHORT;
	if (ld >= ( MAX_SHORT / 2)) ld -= MAX_SHORT;
	if (ld <= (-MAX_SHORT / 2)) ld += MAX_SHORT;
#ifdef MEGA_ROVER_1_1
	right = (float)rd / (13 * 71.2f * 4) * 150 * M_PI / 1000.0f;		// 右車輪の移動量(m)
	left  = (float)ld / (13 * 71.2f * 4) * 150 * M_PI / 1000.0f * coef;	// 左車輪の移動量(m)
#else
	right = (float)rd / (48 * 104 * 4) * 150 * M_PI / 1000.0f;			// 右車輪の移動量(m)
	left  = (float)ld / (48 * 104 * 4) * 150 * M_PI / 1000.0f * coef;	// 左車輪の移動量(m)
#endif
	v = (right + left) / 2.0f;						// 前後の速度
	w = (right - left) / (TREAD / 1000.0f);			// 回転速度

	if (mutex == NULL) return -1;
	WaitForSingleObject(mutex, INFINITE);
	odoX += v * cos(odoThe);
	odoY += v * sin(odoThe);
	odoThe += w;

	if (odoThe >  M_PI) odoThe -= 2.0f*M_PI;
	if (odoThe < -M_PI) odoThe += 2.0f*M_PI;
	ReleaseMutex(mutex);

	
	if (is_speed_control_mode){
		static int is_first = 1;
		if (is_first){
			time0 = timeGetTime();
			is_first = 0;
		} else {
			unsigned long time = timeGetTime();
			static float errIntRight = 0, errIntLeft = 0; 
			float period = (time - time0) / 1000.0f;
			time0 = time;
			speedRight = right / period;
			speedLeft  = left  / period;
			float errRight = (refSpeedRight - speedRight);
			float errLeft  = (refSpeedLeft  - speedLeft );
			errIntRight += (errRight * period);
			errIntLeft  += (errLeft  * period);
			if ((refSpeedRight != 0.0f) || (refSpeedLeft != 0.0f)){
				float rightTorque =   kp * errRight + ki * errIntRight + refSpeedRight / MAX_SPEED * 1000;
				float leftTorque  = - kp * errLeft  - ki * errIntLeft  - refSpeedLeft  / MAX_SPEED * 1000;
				setMotor((int)rightTorque, (int)leftTorque);
			} else {
				setMotor(0, 0);
			}
		}
	}

	return 0;
}

/*!
 * @brief MegaRoverのジョイスティックの状態を取得する
 * x, y (0-1)
 * ゲームパッドのボタン(左,下,右,上,ｽﾀｰﾄ,R3,L3,ｾﾚｸﾄ,□,×,○,△,R1,L1,R2,L2)
 *
 * @param[in] x ジョイスティックのx軸の値(-1～1)
 * @param[in] y ジョイスティックのy軸の値(-1～1)
 * @param[in] b ジョイスティックのボタンの値
 *
 * @return 0
 */
int megaRover::getJoyStick(float *x, float *y, int *b)
{
	if (comMutex == NULL) return -1;
	WaitForSingleObject(comMutex, INFINITE); 
	CWRC_ReadMemMap(GamePadButton, 10);
	CWRC_ReadExecute();

	*b = GetMem_SWord(GamePadButton);
	*x = (float)GetMem_SWord(GamePadLJoyUD)/128.0f;
	*y = (float)GetMem_SWord(GamePadRJoyLR)/128.0f;
	ReleaseMutex(comMutex);

	return 0;
}


int megaRover::getReferenceSpeed(float *right, float *left){
	*right = refSpeedRight;
	*left  = refSpeedLeft ;

	return 0;
}


/*!
 * @brief オドメトリの角度の設定
 * ジャイロオドメトリのために方位を設定する(rad)
 *
 * @param[in] angle オドメトリの角度(rad)
 *
 * @return 0
 */
int megaRover::setOdometoryAngle(float angle)
{
	if (mutex == NULL) return -1;
	WaitForSingleObject(mutex, INFINITE);
	odoThe = angle;
	if (odoThe >  M_PI) odoThe -= 2.0f*M_PI;
	if (odoThe < -M_PI) odoThe += 2.0f*M_PI;
	ReleaseMutex(mutex);
	
	return 0;
}

/*!
 * @brief オドメトリの値をクリアする．
 *
 * @return 0
 */
int megaRover::clearOdometory()
{
	if (mutex == NULL) return -1;
	WaitForSingleObject(mutex, INFINITE);
	odoX = odoY = odoThe = 0;
	ReleaseMutex(mutex);		

	return 0;
}

