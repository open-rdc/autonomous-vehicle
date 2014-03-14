#pragma once

#include <opencv2\\opencv.hpp>

using namespace cv;

class imageProcessing
{
private:
	CvCapture *capture;				//! ビデオキャプチャ構造体
	Mat img;						//! 画像
	
	int terminate;					//! スレッドの破棄（1:破棄, 0:継続）
	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);	// スレッドのエントリーポイント
	DWORD WINAPI ExecThread();		// 別スレッドで動作する関数
	HANDLE mutex, comMutex;			// COMポートの排他制御
public:
	imageProcessing(void);			// コンストラクタ
	~imageProcessing(void);			// デストラクタ
	void init(void);				// 初期化
	void close(void);				// 終了処理
	void update(void);				// 周期的に行う処理
	bool checkTarget(float *cf);	// 検出した結果を渡す変数
};
