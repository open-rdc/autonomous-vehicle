// Comm.cpp : 実装ファイル
//

#include "stdafx.h"
#include "Comm.h"

OVERLAPPED sendop, recop;

/*!
 * @class CComm
 * @brief シリアル通信を行うためのクラス
 */

/*!
 * @brief コンストラクタ
 */
CComm::CComm() : com_port(0)
{
}


/*!
 * @brief デストラクタ
 */
CComm::~CComm()
{
}


/*!
 * @brief COMポートのオープン
 * 通信開始時には，必ず実行する
 *
 * @param[in] port ポートの番号 [1-]
 * @param[in] baudrate ボーレート(bps)
 *
 * @return true:成功, false:失敗
 */
bool CComm::Open(int port, int baudrate)
{
	// 通信用初期設定
	CString port_name;
	
	port_name.Format("COM%d",port);

	hComm = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
	if (hComm == INVALID_HANDLE_VALUE){
		return false;		// 異常終了
	} else {
		DCB dcb;
		GetCommState(hComm,&dcb);
		dcb.BaudRate = baudrate;
		dcb.ByteSize = 8;
		SetCommState(hComm,&dcb);
	}
	com_port = port;
	return true;			// 正常終了
}


/*!
 * @brief COMポートのクローズ
 * 通信終了時には，必ず実行する
 *
 * @return true
 */
bool CComm::Close(void)
{
	CloseHandle(hComm);
	return true;
}


/*!
 * @brief データを送信する
 *
 * @param[in] data データの先頭アドレス
 * @param[in] len 文字数
 *
 * @return 送信した文字数（lenと同じ数の場合は成功）, -1:エラー
 */
int CComm::Send(char *data, int len)
{
	DWORD retlen;

	if(hComm == INVALID_HANDLE_VALUE) return -1;
	if (len == 0){			// もし文字数を省略したらNULLまでの文字数
		len = (int)strlen(data);
	}
	WriteFile(hComm, data, len, &retlen, &sendop);
	return retlen;
}


/*!
 * @brief データを受信する
 *
 * @param[in] data データの先頭アドレス
 * @param[in] max_len 最大文字数
 *
 * @return 受信した文字数, -1:エラー
 */
int CComm::Recv(char *data, int max_len)
{
	int len;
	COMSTAT Comstat;
	DWORD NoOfByte,Error;

	if(hComm == INVALID_HANDLE_VALUE) return -1;	// ハンドルがない場合
	ClearCommError(hComm,&Error,&Comstat);
	if(!Comstat.cbInQue) return 0;					// 受信していない場合
	if (Comstat.cbInQue > (unsigned int)max_len) len = max_len;
	else len = Comstat.cbInQue;						// 受信できる最大文字数を超えている場合は，max_lenだけ受信
	ReadFile(hComm, data, len, &NoOfByte, &recop);
	return len;
}

/*!
 * @brief データバッファをクリアする
 *
 * @return true
 */
bool CComm::ClearRecvBuf(void)
{
	const int buf_size = 1000;
	char buf[buf_size];
	
	while(Recv(buf,buf_size) > 0);
	return true;
}
