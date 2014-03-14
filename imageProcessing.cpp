#include "StdAfx.h"
#include "imageProcessing.h"

/*!
 * @class imageProcessing
 * @brief 画像処理により探索対象者を検出して，その有無と方向を知らせるクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
imageProcessing::imageProcessing(void):
	terminate(0), capture(NULL)
{
}

/*!
 * @brief デストラクタ
 */
imageProcessing::~imageProcessing(void)
{
}

/*!
 * @brief 初期化
 * カメラや画像処理の初期化を行い，画像処理のスレッドを立ち上げる．
 */
void imageProcessing::init(void)
{
	mutex = CreateMutex(NULL, FALSE, _T("IMAGE_PROCESSING_RESULT"));

	capture = cvCreateCameraCapture(0);
	if (capture == NULL) AfxMessageBox("Not Find Camera Device");

	// 速度制御のスレッドを開始
	DWORD threadId;	
	HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, &threadId); 
	// スレッドの優先順位を上げる
	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
}

/*!
 * @brief 終了処理
 */
void imageProcessing::close(void)
{
	terminate = 1;				// 速度制御スレッドの停止
	CloseHandle(mutex);
	cvDestroyWindow ("Camera");
}

/*!
 * @brief 周期的に行う処理
 * 画像処理を行い，結果をクラス変数に代入する．
 * 画像処理した結果をウィンドウに表示する．
 */
void imageProcessing::update(void)
{
	std::vector<cv::Rect> res;
	img = cvQueryFrame(capture);

#ifdef _DEBUG
	{
		static int prev_sec = 0;
		char s[100];
		time_t timer = time(NULL);
		struct tm *date = localtime(&timer);
		if ((date->tm_sec != prev_sec)&&(!img.empty())){	// 保存は１秒間に１枚以内
			prev_sec = date->tm_sec;
			sprintf(s, "image%04d%02d%02d%02d%02d%02d.bmp", date->tm_year+1900, date->tm_mon+1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
			cv::imwrite(s, img);
		}
	}
#endif

	// 結果の描画
	namedWindow("Camera", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	imshow( "Camera", img ); 
	waitKey(1000);														// 1000ms(1秒)ウエイト
}

/*!
 * @brief 検出した結果を渡す変数
 *
 * @param[out] cf    確信度(0.0:ターゲットではない～1.0:ターゲットである)
 *
 * @return true:発見, false:未発見
 */
bool imageProcessing::checkTarget(float *cf)
{
	// 呼び出されて画像処理，もしくは既に処理した結果を戻す．
	*cf = 0.0;			// ターゲットではない．

	return false;
}

/*!
 * @brief スレッドのエントリーポイント
 *
 * @param[in] lpParameter インスタンスのポインタ
 * 
 * @return S_OK
 */
DWORD WINAPI imageProcessing::ThreadFunc(LPVOID lpParameter) 
{
	return ((imageProcessing*)lpParameter)->ExecThread();
}

/*!
 * @brief 別スレッドで動作する関数
 * ロボットの制御を別スレッドで行う．
 *
 * @return S_OK
 */
DWORD WINAPI imageProcessing::ExecThread()
{
	while(!terminate){
		update();
	}
	return S_OK; 
}
