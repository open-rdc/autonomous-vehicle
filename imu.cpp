// IMUに独自仕様の基板とプロトコルで通信している

#include "stdafx.h"
#include "imu.h"

/*!
 * @class imu
 * @brief IMUを使うためのクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
imu::imu()
{
}

/*!
 * @brief デストラクタ
 */
imu::~imu()
{
}

/*!
 * @brief 初期化
 *
 * @param[in] com_port 通信ポートの番号(1-)
 *
 * @return true:成功，false:失敗
 */
int imu::Init(int com_port)
{
	int res = comm.Open(com_port, 115200);
	comm.Send("0");

	return res;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int imu::Close()
{
	comm.Close();
	return 0;
}

/*!
 * @brief IMUのリセット
 * IMUのオフセットのリセット (7秒間停止)
 *
 * @return 0
 */
int imu::Reset()
{
	comm.Send("a");
	Sleep(7000);			// resetに6.6秒かかる
	comm.ClearRecvBuf();	// 受信用バッファをクリア

	return 0;
}

/*!
 * @brief 角度の取得の開始
 *
 * @return 0
 */
int imu::GetAngleStart()
{
	comm.Send("e");

	return 0;
}

/*!
 * @brief 角度の取得
 *
 * @param[in] x x軸方向の角度(rad)
 * @param[in] y y軸方向の角度(rad)
 * @param[in] z z軸方向の角度(rad)
 *
 * @return 0
 */
int imu::GetAngle(float *x, float *y, float *z)
{
	const float COEF = -0.00836181640625f;
	const int max_no = 100;
	static char buf[max_no];
	char w[10], *e;
	int x1, y1, z1;

	if (comm.Recv(buf, max_no) <= 0) return -1;
	strncpy(w, buf  , 4);
	x1 = strtol(w, &e, 0x10);
	if (x1 >= 0x8000) x1 -= 0x10000;
	strncpy(w, buf+4, 4);
	y1 = strtol(w, &e, 0x10);
	if (y1 >= 0x8000) y1 -= 0x10000;
	strncpy(w, buf+8, 4);
	z1 = strtol(w, &e, 0x10);
	if (z1 >= 0x8000) z1 -= 0x10000;

	*x = COEF * x1;
	*y = COEF * y1;
	*z = COEF * z1;

	return 0;
}
