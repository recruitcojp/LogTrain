#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

/*--------------------------------------------------------------------------*/
/* デフォルト値 															*/
/*--------------------------------------------------------------------------*/
#define LOGTRAIN_SERVER		"127.0.0.1"
#define LOGTRAIN_PORT		5550
#define CONFIG_FILE			"/usr/local/logtrain/bin/logtrain.properties"
#define CONNECT_TIMEOUT 	30
#define CONNECT_ERR_WAIT	60
#define READ_TIMEOUT		5
#define WRITE_TIMEOUT		5
#define CACHE_DIR			"/usr/local/logtrain/cache/"
#define CACHE_LIMIT			1024*1024*500
#define MAX_STDIN_BUF		1024000

/* 通信関連 */
#define LT_DATA_INI		1		/* 初期データ */
#define LT_DATA_LOG_R	2		/* リアルタイムログ */
#define LT_DATA_LOG_P	3		/* 過去ログ */
#define LT_DATA_RES		9		/* 応答 */

/* プロセス処理ステイタス */
#define PROC_STS_INI	0		/* 初期/サーバ未接続 */
#define PROC_STS_CON	1		/* コネクト待ち */
#define PROC_STS_DATA	2		/* 初期データ送信中 */
#define PROC_STS_LOG	3		/* ログデータ送信中 */
#define PROC_STS_WAIT	4		/* イベント待ち中 */


/*--------------------------------------------------------------------------*/
/* 構造体定義 																*/
/*--------------------------------------------------------------------------*/
typedef struct{
	unsigned int		kind;	/* データ種別 */
	unsigned int		size;	/* データサイズ */
}LOGTRAIN_FMT_HEADER;

typedef struct{
	LOGTRAIN_FMT_HEADER hd;
	char	buf[MAX_STDIN_BUF];	/* データ部 */
}LOGTRAIN_FMT;

/*--------------------------------------------------------------------------*/
/* グローバル変数 															*/
/*--------------------------------------------------------------------------*/
extern char  config_name[];		/* コンフィグファイル名 */
extern char  log_kind[];		/* HDFS上ファイルの識別子 */
extern int   stdfd;				/* STDINのファイルディスクプリタ */
extern int   sockfd;			/* ソケット通信用のファイルディスクプリタ */
extern int   debug_flg;			/* デバックフラグ */
extern int   proc_status;		/* プロセスステイタス */
extern int   ini_wait;			/* 1の場合は起動後にサーバ接続待ちを行う */
extern int   sv_connect_time;	/* 前回接続した時間 */

extern char  server_name[];		/* LogTrainServerのサーバ名 */
extern short server_port;		/* LogTrainServerのポート番号 */
extern char  cache_dir[];		/* LocalDiskキャッシュの出力先ディレクトリ */
extern int   cache_limit;		/* LocalDiskキャッシュの最大サイズ */
extern int   connect_timeout;	/* TCP/IP 接続タイムアウト */
extern int   connect_err_wait;	/* TCP/IP コネクト異常時に、この時間が経過するまでは再接続しない */
extern int   read_timeout;		/* TCP/IP 受信タイムアウト */
extern int   write_timeout;		/* TCP/IP 送信タイムアウト */

/*--------------------------------------------------------------------------*/
/* プロトタイプ宣言 														*/
/*--------------------------------------------------------------------------*/
/* 共通関数 */
int com_logPrint(char *msgfmt, ...);
char *com_SetParameter(char *s);
int com_ReadConfig(char *file_name);
int com_fileCheck(char *filename);
int com_nborder(unsigned int orgValue);
int com_allmkdir(char *bdir);
int com_select();

/* ソケット通信系 */
int tcp_connect();
int tcp_select(int type, int tm);
int tcp_write(char *buf, int buf_len);
int tcp_read(char *buf, int buf_len);
int tcp_disconnect();

/* 標準入出力系 */
int std_select(int type, int tm);
int std_write(char *buf, int buf_len);
int std_read(char *buf, int buf_len);

/* キャッシュ系 */
int cache_write(char *cache_file, char *stdbuf, int stdbuf_len);

/* 上位レイア関数 */
int LogTrainStdRead(char *stdbuf);
int LogTrainTcpConnect();
int LogTrainTcpWrite(int kind, char *buf, int buf_len);
int LogTrainTcpRead(int tm);
int LogTrainTcpClose();
int LogTrainCacheWrite(char *stdbuf,int stdbuf_len);

