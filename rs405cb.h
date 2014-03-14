#define SERVO_ID	1							// サーボのID
#define	COM_PORT	"COM1"						// サーボのCOM番号
#define	MAX_TORQUE	0x64						// 最大トルク

HANDLE CommOpen( char *pport );					// 通信ポートのオープン
int CommClose( HANDLE hComm );					// 通信ポートを閉じる
int RSMove( HANDLE hComm, short sPos, unsigned short sTime );
												// サーボの出力角を指定
short RSStartGetAngle( HANDLE hComm );			// サーボの現在角度の取得を開始する
short RSGetAngle( HANDLE hComm );				// サーボの現在角度を取得する
int RSTorqueOnOff( HANDLE hComm, short sMode );	// サーボのトルクをON/OFFする
int RSMaxTorque( HANDLE hComm, int maxTorque );	// サーボのトルクを設定する
