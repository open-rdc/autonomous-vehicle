// URG.cpp : 実装ファイル
//

#include "stdafx.h"
#include "URG.h"
#include <math.h>
#include "logger.h"

// CURG

#define	M_PI	3.14159f

/*!
 * @class CURG
 * @brief URGを使用するためのクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
CURG::CURG()
{
}

/*!
 * @brief デストラクタ
 */
CURG::~CURG()
{
}

/*!
 * @brief 初期設定
 *
 * @param[in] om_port URGが接続されているCOMポート(1-)
 *
 * @return true:成功，false:失敗
 */
int CURG::Init(int com_port)
{
	int res = comm.Open(com_port);
	Sleep(100);
//	comm.Send("BM\n");

	return res;
}

/*!
 * @brief 終了処理
 *
 * @return 0
 */
int CURG::Close()
{
	comm.Close();
	return 0;
}

/*!
 * @brief URGの計測開始
 * これを呼び出してからしばらくして，GetDataを呼び出す．
 *
 * @return 0
 */
int CURG::StartMeasure(){
	comm.ClearRecvBuf();
	comm.Send("ME0220086001000\n");	// 距離データ(3byte)と反射強度(3byte)の出力
									// ME,方向:0220~0860(4+4),まとめ:01(2),間引き:0(1),送信回数:00(2)垂れ流し
									// 10～170deg
	return 0;
}

/*!
 * @brief 受信バッファにたまった距離データを取得する
 *
 * @param[out] data 距離データ
 *
 * @return 1以上：正常終了（データ数），0以下：異常終了
 */
int CURG::GetData(int length[n_data], int intensity[n_data]){
	const int max_no = n_data * 6 + 1000;
	static char recv_buf[max_no], buf[max_no * 3], buf2[max_no];
	static int pointer = 0;
	int recv_num, i = 0, j = 0, num_lf = 0;

	do{
		recv_num = comm.Recv(recv_buf, max_no);
		for(int k = 0; k < recv_num; k ++){
			buf[pointer ++] = recv_buf[k];
		}
		{
			// 最後のデータ以外はすべて破棄する．
			int dp = 0, dpp = 0;
			for(int k = 0; k < pointer - 1; k ++){
				if ((buf[k] == '\n')&&(buf[k+1] == '\n')){
					dpp = dp;		// 一つ前のデリミタの位置を保存する．
					dp = k + 2;
				}
			}
			if (dpp != 0){
				pointer -= dpp;
				for (int k = 0; k < pointer; k ++){
					buf[k] = buf[k + dpp];
				}
			}
		}
	} while (recv_num > 0);

	while(i < pointer){		// データの最後になるまで繰り返す
		if (num_lf < 3){	// 始めの4ラインのデータは無視（エラー処理無し）
			if (buf[i] == '\n') num_lf ++;
			i ++;
			continue;
		}
		buf2[j ++] = buf[i ++];
		if (buf[i+1] == '\n'){
			if (buf[i+2] == '\n'){
				pointer -= (i + 3);
				for(int k = 0; k < pointer; k ++){
					buf[k] = buf[(i + 3) + k];
				}
				break;
			}
			else i += 2;
		}
	}
	if (j != (n_data * 6)) return -1;
	for(i = 0;i < n_data;i ++){
		length[i]    = ((buf2[i*6  ]-0x30) << 12) + ((buf2[i*6+1]-0x30) << 6) + (buf2[i*6+2]-0x30);
		intensity[i] = ((buf2[i*6+3]-0x30) << 12) + ((buf2[i*6+4]-0x30) << 6) + (buf2[i*6+5]-0x30);
	}
	return n_data;
}

/*!
 * @brief デカルト座標系への変換
 *
 * @param[in]  tilt チルトの角度(rad)
 * @param[in]  data 取得した距離データ[mm]
 * @param[out] p    デカルト座標系に変換した障害物の位置データ(mm)
 *
 * @return 取得したデータの数
 */
int CURG::TranslateCartesian(float tilt, int data[n_data], pos p[n_data])
{
	float x, y, ang;
	for(int i = 0; i < n_data; i ++){
		if (data[i] < 20){
			p[i].x = p[i].y = p[i].z = 0;
			continue;
		}
		ang = 0.25f*(i-640/2)*M_PI/180.0f;		// URGスキャン面の角度
		x = data[i]*cos(ang);
		y = data[i]*sin(ang);
		ang = tilt*M_PI/180.0f;					// URGチルト角度
		p[i].x = (int)(x*cos(ang));
		p[i].y = (int)(y         );
		p[i].z = (int)(x*sin(ang));
	}
	return n_data;
}
