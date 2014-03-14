#pragma once

// CnavigationView

#include "URG.h"
#include "dataType.h"

class CnavigationView : public CStatic
{
	DECLARE_DYNAMIC(CnavigationView)

public:
	CnavigationView();
	virtual ~CnavigationView();

protected:
	DECLARE_MESSAGE_MAP()


private:
	// 計測点の表示
	static const int MAX_DATA_NUM = 10000;
	int data_num;
	pos data_pos[MAX_DATA_NUM];

	// 参照データの表示　(playモードで使用)
	static const int MAX_REF_DATA_NUM = 10000;
	int ref_data_num;
	pos ref_pos[MAX_REF_DATA_NUM];

	// 反射強度データの表示
	static const int MAX_INTEN_DATA_NUM = 10000;
	int inten_data_num;
	pos_inten inten_pos[MAX_INTEN_DATA_NUM];

	// 探索対象の候補データの表示
	static const int MAX_SLATE_DATA_NUM = 100;
	int slate_data_num;
	pos_slate slate_pos[MAX_SLATE_DATA_NUM];

	// RGB表示用構造体
	struct bitmap_rgb_T{
		unsigned char blue;
		unsigned char green;
		unsigned char red;
		unsigned char alpha;
	} rgb_thre[640*480];
	
	// オドメトリの表示
	static const int MAX_ODO_NUM = 100;
	struct odometory odo[MAX_ODO_NUM];
	int odo_num;
	long time0;

	// ゴールの表示　(playモードで使用)
	int is_target_view;
	float tarX;				// m
	float tarY;				// m
	float tarThe;			// rad

	// パーティクルの表示　(playモードで使用)
	static const int MAX_PARTICLE_NUM = 1000;	// 最大1000個表示
	int particle_num;
	struct particle_T particle[MAX_PARTICLE_NUM];

	// ワールド座標からディズプレイの座標に変換 (単位はmm)
	int translatePos(int worldX, int worldY, int worldX0, int worldY0, int *dispX, int *dispY);
	int center_x, center_y;								// ビューのの中心(dot)
	int disp_x, disp_y;									// ビューのサイズ(dot)
	int step;											// waypointの番号
	float coincidence;									// 一致度(0-1)
	int is_record, is_play;								// 保存，再生
	float ratio;										// 画面のサイズの比率(m->dot)

public:
	afx_msg void OnPaint();								// 再描画
	int setData(pos *p, int num);						// 障害物の位置データを設定
	int setRefData(pos *p, int num);					// 参照する障害物の位置データの設定
	int setIntensityData(pos_inten *p, int num);		// 反射強度のデータの設定
	int setSlatePoint(pos_slate *p, int num);			// 探索対象の候補の設定
	int setOdometory(float x, float y, float the);		// オドメトリの設定
	int setTargetPos(float x, float y, float the);		// waypointの設定
	int setParticle(struct particle_T *p, int num);		// パーティクルの設定
	int setStep(int step);								// waypointの数の設定
	int setCoincidence(float coincidence);				// 一致度の設定
	int setStatus(int is_record, int is_play);			// 状態(保存モード，再生モード)の設定
};
