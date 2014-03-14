// futabaのサンプルプログラムをほぼそのまま使用させて頂いています．

#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include <mmsystem.h>
#include "rs405cb.h"

/*!
 * @brief 通信ポートを開く
 * 通信速度は、460800bps固定
 *
 * @param[in] pport 通信ポート名(サーボと通信可能なポート名)
 *
 * @return 0:通信ハンドルエラー,0でない値:成功(通信用ハンドル)
 */
HANDLE CommOpen( char *pport )
{
	HANDLE			hComm;		// 通信用ハンドル
	DCB				cDcb;		// 通信設定用
	COMMTIMEOUTS	cTimeouts;	// 通信ポートタイムアウト用


	// 通信ポートを開く
	hComm = CreateFileA( pport,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL );
	// ハンドルの確認
	if( hComm == INVALID_HANDLE_VALUE ){
		hComm = NULL;
		goto FuncEnd;
	}


	// 通信設定
	if( !GetCommState( hComm, &cDcb )){
		// 通信設定エラーの場合
		printf( "ERROR:GetCommState error\n" );
		CommClose( hComm );
		hComm = NULL;
		goto FuncEnd;
	}
	cDcb.BaudRate = 460800;				// 通信速度
	cDcb.ByteSize = 8;					// データビット長
	cDcb.fParity  = TRUE;				// パリティチェック
	cDcb.Parity   = NOPARITY;			// ノーパリティ
	cDcb.StopBits = ONESTOPBIT;			// 1ストップビット

	if( !SetCommState( hComm, &cDcb )){
		// 通信設定エラーの場合
		printf( "ERROR:GetCommState error\n" );
		CommClose( hComm );
		hComm = NULL;
		goto FuncEnd;
	}


	// 通信設定(通信タイムアウト設定)
	// 文字の読み込み待ち時間(ms)
	cTimeouts.ReadIntervalTimeout			= 50;
	// 読み込みの１文字あたりの時間(ms)
	cTimeouts.ReadTotalTimeoutMultiplier	= 50;
	// 読み込みの定数時間(ms)
	cTimeouts.ReadTotalTimeoutConstant		= 50;
	// 書き込みの１文字あたりの時間(ms)
	cTimeouts.WriteTotalTimeoutMultiplier	= 0;

	if( !SetCommTimeouts( hComm, &cTimeouts )){
		// 通信設定エラーの場合
		printf( "ERROR:SetCommTimeouts error\n" );
		CommClose( hComm );
		hComm = NULL;
		goto FuncEnd;
	}


	// 通信バッファクリア
	PurgeComm( hComm, PURGE_RXCLEAR );


FuncEnd:
	return hComm;
}

/*!
 * @brief 通信ポートを閉じる
 *
 * @param[in] hComm 通信ポートのハンドル
 *
 * @return 1:成功
 */
int CommClose( HANDLE hComm )
{
	if( hComm ){
		CloseHandle( hComm );
	}

	return 1;
}

/*!
 * @brief サーボの出力角を指定
 *
 * @param[in] hComm 通信ポートのハンドル
 * @param[in] sPos  移動位置(sPos x 0.1deg)
 * @param[in] sTime 移動時間(sTime x 10ms)
 *
 * @return 0以上:成功,0未満:エラー
 */
int RSMove( HANDLE hComm, short sPos, unsigned short sTime )
{
	unsigned char	sendbuf[28];
	unsigned char	sum;
	int				i;
	int				ret;
	unsigned long	len;


	// ハンドルチェック
	if( !hComm ){
		return -1;
	}

	// バッファクリア
	memset( sendbuf, 0x00, sizeof( sendbuf ));

	// パケット作成
	sendbuf[0]  = (unsigned char)0xFA;				    // ヘッダー1
	sendbuf[1]  = (unsigned char)0xAF;				    // ヘッダー2
	sendbuf[2]  = (unsigned char)SERVO_ID;			    // サーボID
	sendbuf[3]  = (unsigned char)0x00;				    // フラグ
	sendbuf[4]  = (unsigned char)0x1E;				    // アドレス(0x1E=30)
	sendbuf[5]  = (unsigned char)0x04;				    // 長さ(4byte)
	sendbuf[6]  = (unsigned char)0x01;				    // 個数
	sendbuf[7]  = (unsigned char)(sPos&0x00FF);		    // 位置
	sendbuf[8]  = (unsigned char)((sPos&0xFF00)>>8);	// 位置
	sendbuf[9]  = (unsigned char)(sTime&0x00FF);	    // 時間
	sendbuf[10] = (unsigned char)((sTime&0xFF00)>>8);	// 時間
	// チェックサムの計算
	sum = sendbuf[2];
	for( i = 3; i < 11; i++ ){
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[11] = sum;								// チェックサム

	// 通信バッファクリア
	PurgeComm( hComm, PURGE_RXCLEAR );

	// 送信
	ret = WriteFile( hComm, &sendbuf, 12, &len, NULL );

	return ret;
}

/*!
 * @brief サーボの現在角度の取得を開始する
 * 取得は別の関数 RSGetAngle(hComm)
 * 待ち時間を有効に活用するために，別の関数で実装した
 *
 * @param[in] hComm 通信ポートのハンドル
 *
 * @return 0
 */
short RSStartGetAngle( HANDLE hComm )
{
	unsigned char	sendbuf[32];
	unsigned char	sum;
	int				i;
	int				ret;
	unsigned long	len;

	// ハンドルチェック
	if( !hComm ){
		return -1;
	}

	// バッファクリア
	memset( sendbuf, 0x00, sizeof( sendbuf ));

	// パケット作成
	sendbuf[0]  = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1]  = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2]  = (unsigned char)SERVO_ID;			// サーボID
	sendbuf[3]  = (unsigned char)0x09;				// フラグ(0x01 | 0x04<<1)
	sendbuf[4]  = (unsigned char)0x00;				// アドレス(0x00)
	sendbuf[5]  = (unsigned char)0x00;				// 長さ(0byte)
	sendbuf[6]  = (unsigned char)0x01;				// 個数
	// チェックサムの計算
	sum = sendbuf[2];
	for( i = 3; i < 7; i++ ){
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム

	// 通信バッファクリア
	PurgeComm( hComm, PURGE_RXCLEAR );

	// 送信
	ret = WriteFile( hComm, &sendbuf, 8, &len, NULL );
	if( len < 8 ){
		printf("writeError\n");
		// 送信エラー
		return -1;
	}
	return 0;
}

/*!
 * @brief サーボの現在角度を取得する
 * 取得を開始する関数RSStartGetAngle(hComm)を予め実行して，間をおいてから呼び出す．
 *
 * @param[in] hComm 通信ポートのハンドル
 *
 * @return 0以上:サーボの現在角度(0.1度=1),0未満:エラー
 */
short RSGetAngle( HANDLE hComm )
{
	const int MAX_LEN = 128;
	unsigned char	sum;
	int				i;
	int				ret;
	unsigned long	len, readlen;
	unsigned char	readbuf[MAX_LEN];
	short			angle;

	// 読み込み
	memset( readbuf, 0x00, sizeof( readbuf ));
	len = 0;

	COMSTAT Comstat;
	DWORD Error;
	ClearCommError(hComm,&Error,&Comstat);
	if(!Comstat.cbInQue) return 0;					// 受信していない場合
	if (Comstat.cbInQue > MAX_LEN) readlen = MAX_LEN;
	else readlen = Comstat.cbInQue;						// 受信できる最大文字数を超えている場合は，max_lenだけ受信

	ret = ReadFile( hComm, readbuf, readlen, &len, NULL );
	if( len < 26 ){
		printf("readError\n");
		// 受信エラー
		return -2;
	}

	// 受信データの確認
	sum = readbuf[2];
	for( i = 3; i < 26; i++ ){
		sum = sum ^ readbuf[i];
	}
	if( sum ){
		printf("sumError\n");
		// チェックサムエラー
		return -3;
	}

	angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
	return angle;
}

/*!
 * @brief サーボのトルクをON/OFFする
 *
 * @param[in] hComm 通信ポートのハンドル
 * @param[in] sMode 1:トルクON,0:トルクOFF
 *
 * @return 0以上:成功,0未満:エラー
 */
int RSTorqueOnOff( HANDLE hComm, short sMode )
{
	unsigned char	sendbuf[28];
	unsigned char	sum;
	int				i;
	int				ret;
	unsigned long	len;


	// ハンドルチェック
	if( !hComm ){
		return -1;
	}

	// バッファクリア
	memset( sendbuf, 0x00, sizeof( sendbuf ));

	// パケット作成
	sendbuf[0]  = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1]  = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2]  = (unsigned char)SERVO_ID;			// サーボID
	sendbuf[3]  = (unsigned char)0x00;				// フラグ
	sendbuf[4]  = (unsigned char)0x24;				// アドレス(0x24=36)
	sendbuf[5]  = (unsigned char)0x01;				// 長さ(4byte)
	sendbuf[6]  = (unsigned char)0x01;				// 個数
	sendbuf[7]  = (unsigned char)(sMode&0x00FF);	// ON/OFFフラグ
	// チェックサムの計算
	sum = sendbuf[2];
	for( i = 3; i < 8; i++ ){
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[8] = sum;								// チェックサム

	// 通信バッファクリア
	PurgeComm( hComm, PURGE_RXCLEAR );

	// 送信
	ret = WriteFile( hComm, &sendbuf, 9, &len, NULL );

	return ret;
}

/*!
 * @brief サーボのトルクを設定する
 *
 * @param[in] hComm 通信ポートのハンドル
 * @param[in] maxTorque 最大トルクの設定
 *
 * @return 0以上:成功,0未満:エラー
 */
int RSMaxTorque( HANDLE hComm , int maxTorque)
{
	unsigned char	sendbuf[28];
	unsigned char	sum;
	int				i;
	int				ret;
	unsigned long	len;


	// ハンドルチェック
	if( !hComm ){
		return -1;
	}

	// バッファクリア
	memset( sendbuf, 0x00, sizeof( sendbuf ));

	// パケット作成
	sendbuf[0]  = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1]  = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2]  = (unsigned char)SERVO_ID;			// サーボID
	sendbuf[3]  = (unsigned char)0x00;				// フラグ
	sendbuf[4]  = (unsigned char)0x23;				// アドレス(0x24=36)
	sendbuf[5]  = (unsigned char)0x01;				// 長さ(1byte)
	sendbuf[6]  = (unsigned char)0x01;				// 個数
	sendbuf[7]  = (unsigned char)MAX_TORQUE;		// トルク最大値表記(MAX=0x64)
	// チェックサムの計算
	sum = sendbuf[2];
	for( i = 3; i < 8; i++ ){
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[8] = sum;								// チェックサム

	// 通信バッファクリア
	PurgeComm( hComm, PURGE_RXCLEAR );

	// 送信
	ret = WriteFile( hComm, &sendbuf, 9, &len, NULL );

	return ret;
}
