#include "StdAfx.h"
#include "logger.h"
#include <mmsystem.h>
#include <time.h>

FILE* logger::fp = NULL;

/*!
 * @brief 初期化
 *
 * @param[in] filename	ファイル名
 */
void logger::Init(CString filename)
{
	if (NULL == (fp = fopen(filename,"wt"))){
		AfxMessageBox("Cannot Open Log file");
		exit(1);
	}
}

/*!
 * @brief ログファイルへの追加書き込み
 * タイムスタンプの後，指定された文字データを書き込む．
 * 
 * @param[in] str	書き込む文字列（フォーマット指定子を使用可能）
 */
void logger::Write(const char* str, ...)
{
	va_list args;

	if (fp != NULL){
		long time = timeGetTime();
		fprintf(fp, "time(ms):%ld,", time);
		va_start(args, str);
		vfprintf(fp, str, args);
		va_end(args);
	}
}

/*!
 * @brief ログファイルへの追加書き込み
 * タイムスタンプ無しで，指定された文字データを書き込む．
 *
 * @param[in] str	書き込む文字列（フォーマット指定子を使用可能）
 */
void logger::WriteWithoutTime(const char* str, ...)
{
	va_list args;

	if (fp != NULL){
		va_start(args, str);
		vfprintf(fp, str, args);
		va_end(args);
	}
}

/*!
 * @brief 終了処理
 */
void logger::Close()
{
	fclose(fp);
}