#pragma once

class logger
{
public:
	logger(void);											// コンストラクタ
	~logger(void);											// デストラクタ
	static void Init(CString filename);						// 初期化
	static void Write(const char* str, ...);				// ログファイルへの追加書き込み（タイムスタンプ有）
	static void WriteWithoutTime(const char* str, ...);		// ログファイルへの追加書き込み（タイムスタンプ無）
	static void Close();									// 終了処理
protected:
	static FILE* fp;										//! ファイルポインタ
};

#define LOG( ... ) { logger::Write( __VA_ARGS__ ); }							// ログ書き込みマクロ（タイムスタンプ有）
#define LOG_WITHOUT_TIME( ... ) { logger::WriteWithoutTime( __VA_ARGS__ ); }	// ログ書き込みマクロ（タイムスタンプ無）
