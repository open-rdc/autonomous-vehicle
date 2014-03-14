// navigationView.cpp : 実装ファイル
//

#include "stdafx.h"
#include "navigation.h"
#include "navigationView.h"
#include <math.h>
#include <mmsystem.h>
#include <time.h>

#define	M_PI	3.14159f

// CnavigationView

IMPLEMENT_DYNAMIC(CnavigationView, CStatic)

/*!
 * @class CnavigationView
 * @brief 自己位置や障害物データに関して表示を行うクラス
 * @author Y.Hayashibara
 */

/*!
 * @brief コンストラクタ
 */
CnavigationView::CnavigationView():
data_num(0), ref_data_num(0), odo_num(0), ratio(0.02f), center_x(0), center_y(0),
is_target_view(0), particle_num(0), time0(0), step(0), coincidence(0),
is_record(), is_play(0)
{
}

/*!
 * @brief デストラクタ
 */
CnavigationView::~CnavigationView()
{
}

// CnavigationView メッセージ ハンドラ
BEGIN_MESSAGE_MAP(CnavigationView, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

/*!
 * @brief 再描画
 */
void CnavigationView::OnPaint()
{
	CPaintDC mdc(this); // device context for painting

	// Viewのサイズを取得
	CRect rc;
	GetClientRect(&rc);
	mdc.Rectangle(&rc);
	disp_x = rc.Width(), disp_y = rc.Height();
	center_x = disp_x / 2, center_y = disp_y / 2;

	int p;

	// 描画領域の設定
	CDC dc;
	dc.CreateCompatibleDC(&mdc);
	CBitmap memBmp;
	memBmp.CreateCompatibleBitmap(&mdc,disp_x, disp_y);
	CBitmap *pOldBmp = dc.SelectObject(&memBmp);
	dc.Rectangle(0, 0, disp_x, disp_y);

	// フォントの設定
	HFONT hFont = CreateFont(60, //フォント高さ
		0, //文字幅
		0, //テキストの角度
		0, //ベースラインとｘ軸との角度
		FW_DONTCARE, //フォントの重さ（太さ）
		FALSE, //イタリック体
		FALSE, //アンダーライン
		FALSE, //打ち消し線
		DEFAULT_CHARSET, //文字セット
		CLIP_DEFAULT_PRECIS, //出力精度
		CLIP_DEFAULT_PRECIS,//クリッピング精度
		CLEARTYPE_QUALITY, //出力品質
		VARIABLE_PITCH,//ピッチとファミリー
		_T("Impact"));
	HFONT prevFont = (HFONT)SelectObject(dc, hFont);

	HPEN hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(0,0,0));
	HPEN prevPen = (HPEN)SelectObject(dc, hPen);

	// 文字の表示
	rc.left = 10, rc.top = 10;
	dc.DrawText(_T("MEGA ROVER"), -1, &rc, NULL);

	// 文字の表示
	{
		char buf[20];
		sprintf(buf, "STEP: %04d\n",step);
		rc.left = 400, rc.top = 10;
		dc.DrawText(_T(buf), -1, &rc, NULL);
	}

	// 文字の表示
	{
		if (is_record){
			rc.left = 10, rc.top = 400;
			dc.DrawText(_T(">> RECORD"), -1, &rc, NULL);
		} else if (is_play){
			rc.left = 10, rc.top = 400;
			dc.DrawText(_T("PLAY >>"), -1, &rc, NULL);
		}
	}

	// ワールド座標系での中心を求める
	int x0 = 0, y0 = 0;
	if (odo_num > 0){
		x0 = (int)(odo[0].x * 1000);
		y0 = (int)(odo[0].y * 1000);
	}

	// マップの表示
	{
		CString num;
		memBmp.GetBitmapBits(disp_x*disp_y*4,rgb_thre);
		dc.SetROP2(R2_COPYPEN);
		struct bitmap_rgb_T rgb;

		rgb.red = rgb.green = rgb.blue = 0;
		
		int x, y;
		for(int i = 0; i < data_num; i ++){
			translatePos(data_pos[i].x, data_pos[i].y, x0, y0, &x, &y);
			if ((x >= 0)&&(x < disp_x)&&(y >= 0)&&(y < disp_y)){
				p = x + y * disp_x;
				rgb_thre[p] = rgb;
			}
		}
		// 参照データがある場合は，青で表示
		rgb.red = rgb.green = 0, rgb.blue = 255;
		for(int i = 0; i < ref_data_num; i ++){
			translatePos(ref_pos[i].x, ref_pos[i].y, x0, y0, &x, &y);
			if ((x >= 0)&&(x < disp_x)&&(y >= 0)&&(y < disp_y)){
				p = x + y * disp_x;
				rgb_thre[p] = rgb;
			}
		}
		// 反射強度データがある場合は，赤で表示　（強度に応じて白から赤に変化）
		for(int i = 0; i < inten_data_num; i ++){
			int red = (int)min(max((float)inten_pos[i].intensity / 4000 * 255, 0), 255);
			rgb.red = 255, rgb.green = rgb.blue = 255 - red;
			translatePos(inten_pos[i].pos.x, inten_pos[i].pos.y, x0, y0, &x, &y);
			if ((x >= 0)&&(x < disp_x)&&(y >= 0)&&(y < disp_y)){
				p = x + y * disp_x;
				rgb_thre[p] = rgb;
			}
		}
		// 一致度のグラフを描く
		for(int i = 0; i < (coincidence * 200); i ++){
			y = 400 - i;
			if (y < 0) break;
			rgb.red = 255 - i, rgb.green = i, rgb.blue = 0;
			for(x = 500; x < 550; x ++){
				p = x + y * disp_x;
				rgb_thre[p] = rgb;
			}
		}

		memBmp.SetBitmapBits(disp_x*disp_y*4,rgb_thre);
		dc.SetBkColor(RGB(0xff,0xff,0xff));
	}
	
	// 探索対象候補点を赤で表示　（確率に応じて半径が変化）
	{
		static const int MAX_RADIUS = 10;
		int x, y, radius;
		DeleteObject(hPen);
		hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(255,0,0));		// 赤色のペンを選択
		SelectObject(dc, hPen );
		for(int i = 0; i < slate_data_num; i ++){
			radius = (int)(slate_pos[i].probability * MAX_RADIUS);
			translatePos(slate_pos[i].pos.x, slate_pos[i].pos.y, x0, y0, &x, &y);
			dc.Arc(x - radius, y - radius, x + radius, y + radius, x - radius, y, x - radius, y);
		}
		SelectObject(dc, prevPen );
	}

	// パーティクルの表示
	{
		static const int RADIUS = 3, ANG_LEN = 5;
		float angle;
		int x, y, px, py, x1, y1;
		DeleteObject(hPen);
		hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(128,128,128));		// 灰色のペンを選択
		SelectObject(dc, hPen );
		for(int i = 0;i < particle_num; i ++){
			px = (int)(particle[i].x * 1000);
			py = (int)(particle[i].y * 1000);
			translatePos(px, py, x0, y0, &x, &y);
			dc.Ellipse(x - RADIUS, y - RADIUS, x + RADIUS, y + RADIUS);
			angle = particle[i].the;
			x1 = x + (int)(ANG_LEN * cos(angle));
			y1 = y - (int)(ANG_LEN * sin(angle));
			dc.MoveTo(x, y); dc.LineTo(x1, y1);
		}
		SelectObject(dc, prevPen );
	}

	// オドメトリの表示
	if (odo_num > 0) {
		static const int RADIUS = 10, ANG_LEN = 20;
		int ox = (int)(odo[0].x * 1000), oy = (int)(odo[0].y * 1000);
		int x, y, x1, y1;
		float angle = odo[0].the;

		// 現在の位置の描画
		translatePos(ox, oy, x0, y0, &x, &y);
		dc.Ellipse(x - RADIUS, y - RADIUS, x + RADIUS, y + RADIUS);
		x1 = x + (int)(ANG_LEN * cos(angle));
		y1 = y - (int)(ANG_LEN * sin(angle));
		dc.MoveTo(x, y); dc.LineTo(x1, y1);

		// 履歴の描画
		dc.MoveTo(x, y);
		for(int i = 1; i < odo_num;i ++){
			ox = (int)(odo[i].x * 1000);
			oy = (int)(odo[i].y * 1000);
			translatePos(ox, oy, x0, y0, &x, &y);
			DeleteObject(hPen);
			hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(0, 255 * (odo_num - i)/odo_num, 0));
			SelectObject(dc, hPen);
			dc.LineTo(x, y);
		}
		SelectObject(dc, prevPen);
	}

	// ゴールの描画
	if (is_target_view){
		static const int RADIUS = 5, ANG_LEN = 40;
		int tx = (int)(tarX * 1000), ty = (int)(tarY * 1000);
		int x, y, dx, dy;

		translatePos(tx, ty, x0, y0, &x, &y);
		DeleteObject(hPen);
		hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(255,0,0));		// 赤ペンを選択
		SelectObject(dc, hPen );
		dc.Ellipse(x - RADIUS, y - RADIUS, x + RADIUS, y + RADIUS);
		dx =   (int)(ANG_LEN * cos(tarThe + M_PI / 2));
		dy = - (int)(ANG_LEN * sin(tarThe + M_PI / 2));
		dc.MoveTo(x - dx, y - dy); dc.LineTo(x + dx, y + dy);
		SelectObject(dc, prevPen );
	}
	
	SelectObject(dc, prevFont);
	DeleteObject(hFont);
	DeleteObject(hPen);

	// 画面の切り替え
	mdc.BitBlt(0,0,disp_x,disp_y,&dc,0,0,SRCCOPY);
	dc.SelectObject(pOldBmp);
	memBmp.DeleteObject();
	dc.DeleteDC();
}

/*!
 * @brief 障害物の位置データを設定
 *
 * @param[in] p 障害物の位置データ
 * @param[in] num 障害物の位置データの数
 *
 * @return 表示する障害物の位置データの数
 */
int CnavigationView::setData(pos *p, int num)
{
	data_num = min(num, MAX_DATA_NUM);
	pos *r = data_pos;
	for(int i = 0; i < data_num; i ++){
		*r ++ = *p ++;
	}
	InvalidateRect(NULL);

	return data_num;
}

/*!
 * @brief 参照する障害物の位置データの設定
 *
 * @param[in] p 参照する障害物の位置データ
 * @param[in] num 参照する障害物の位置データの数
 *
 * @return 参照する障害物の位置データの数
 */
int CnavigationView::setRefData(pos *p, int num)
{
	ref_data_num = min(num, MAX_REF_DATA_NUM);
	pos *r = ref_pos;
	for(int i = 0; i < ref_data_num; i ++){
		*r ++ = *p ++;
	}
	InvalidateRect(NULL);

	return ref_data_num;
}

/*!
 * @brief 反射強度のデータの設定
 *
 * @param[in] p 反射強度付き位置データ
 * @param[in] num 参照する障害物の位置データの数
 *
 * @return 参照する障害物の位置データの数
 */
int CnavigationView::setIntensityData(pos_inten *p, int num)
{
	inten_data_num = min(num, MAX_INTEN_DATA_NUM);
	pos_inten *r = inten_pos;
	for(int i = 0; i < inten_data_num; i ++){
		*r ++ = *p ++;
	}
	InvalidateRect(NULL);

	return inten_data_num;
}

/*!
 * @brief 探索対象の候補の設定
 *
 * @param[in] p 探索対象の候補の位置データ
 * @param[in] num 探索対象の候補の位置データの数
 *
 * @return 参照する障害物の位置データの数
 */
int CnavigationView::setSlatePoint(pos_slate *p, int num)
{
	slate_data_num = min(num, MAX_SLATE_DATA_NUM);
	pos_slate *r = slate_pos;
	for(int i = 0; i < slate_data_num; i ++){
		*r ++ = *p ++;
	}
	InvalidateRect(NULL);

	return slate_data_num;
}


/*!
 * @brief オドメトリの設定
 *
 * @param[in] x   オドメトリのx座標(m)
 * @param[in] y   オドメトリのx座標(m)
 * @param[in] the オドメトリの角度(rad)
 *
 * @return 表示するオドメトリの数
 */
int CnavigationView::setOdometory(float x, float y, float the)
{
	const float step_period = 1.0f;						// 経路を記録する周期(s)
	long time = timeGetTime();							// step_period以上の間隔を開けて保存（ウェイポイントとなる）

	odo[0].x = x, odo[0].y = y, odo[0].the = the;
	if ((time - time0) > (int)(step_period * 1000)){
		if (odo_num < MAX_ODO_NUM) odo_num ++;
		for(int i = (odo_num - 1); i > 0; i --){
			odo[i] = odo[i - 1];
		}
		time0 = time;
	}
	InvalidateRect(NULL);

	return odo_num;
}

/*!
 * @brief waypointの設定
 *
 * @param[in] x   waypointのx座標(m)
 * @param[in] y   waypointのy座標(m)
 * @param[in] the waypointの角度(rad)
 *
 * @return 0
 */
int CnavigationView::setTargetPos(float x, float y, float the)
{
	is_target_view = 1;
	tarX = x;
	tarY = y;
	tarThe = the;
	InvalidateRect(NULL);

	return 0;
}

/*!
 * @brief パーティクルの設定
 *
 * @param[in] p   パーティクルのデータ
 * @param[in] num パーティクルの数
 *
 * @return 表示するパーティクルの数
 */
int CnavigationView::setParticle(struct particle_T *p, int num)
{
	particle_num = min(num, MAX_PARTICLE_NUM);
	struct particle_T *q = particle;
	for(int i = 0;i < particle_num; i ++){
		*q ++ = *p ++;
	}
	InvalidateRect(NULL);

	return data_num;
}

// ワールド座標からディズプレイの座標に変換 (単位はmm)
/*!
 * @brief ワールド座標系からディズプレイの座標への変換
 *
 * @param[in]  worldX  ワールド座標系のx座標(m)
 * @param[in]  worldY  ワールド座標系のy座標(m)
 * @param[in]  worldX0 ディスプレイの中心となるワールド座標系のx座標(m)
 * @param[in]  worldY0 ディスプレイの中心となるワールド座標系のy座標(m)
 * @param[out] dispX   ディスプレイのx座標(dot)
 * @param[out] dispY   ディスプレイのy座標(dot)
 *
 * @return 0
 */
int CnavigationView::translatePos(int worldX, int worldY, int worldX0, int worldY0, int *dispX, int *dispY)
{
	*dispX = (int)(ratio * (worldX - worldX0) + center_x);
	*dispY = (int)(ratio * (disp_y - (worldY - worldY0)) + center_y);
	
	return 0;
}

/*!
 * @brief waypointの数の設定
 *
 * @param[in] step waypointの数
 *
 * @return 0
 */
int CnavigationView::setStep(int step)
{
	this->step = step;

	return 0;
}

/*!
 * @brief 一致度の設定
 *
 * @param[in] coincidence 一致度(0-1)
 *
 * @return 0
 */
int CnavigationView::setCoincidence(float coincidence)
{
	this->coincidence = coincidence;

	return 0;
}

/*!
 * @brief 状態(保存モード，再生モード)の設定
 *
 * @param[in] is_record 保存モードかどうかのフラグ(1:保存モード，0:保存モードではない)
 * @param[in] is_play   再生モードかどうかのフラグ(1:再生モード，0:再生モードではない)
 *
 * @return 0
 */
int CnavigationView::setStatus(int is_record, int is_play)
{
	this->is_record = is_record;
	this->is_play   = is_play  ;

	return 0;
}
