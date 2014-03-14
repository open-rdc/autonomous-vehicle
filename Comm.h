#pragma once

#define DEFAULT_BAUDRATE 19200								//! 標準のビットレート

class CComm : public CWnd
{
public:
	CComm();												// コンストラクタ
	virtual ~CComm();										// デストラクタ

public:
	int com_port;											//! ポート番号
	HANDLE hComm;											//! ハンドラ
	
	bool Open(int port, int baudrate = DEFAULT_BAUDRATE);	// COMポートのオープン
	bool Close(void);										// COMポートのクローズ
	int Send(char *data, int len = 0);						// データを送信する
	int Recv(char *data, int max_len);						// データを受信する
	bool ClearRecvBuf(void);								// データバッファをクリアする
};
