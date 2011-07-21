#include "LogTrainClient.h"

/* 利用方法 */
void usage(char *prog){
	printf("%s [option] ログ種別\n",prog);
	printf("\n");
	printf("    [オプション]\n");
	printf("    -d        ：デバックモード\n");
	printf("    -i        ：初期化モード変更\n");
	printf("    -f file   ：コンフィグファイル名\n");
	printf("    -h server ：接続先サーバのIPアドレス\n");
	printf("    -p port   ：ポート番号\n");
	printf("    -c dir    ：キャッシュディレクトリ\n");
	printf("    -l size   ：キャッシュファイルの上限サイズ\n");
	printf("    -ct sec   ：接続タイムアウト\n");
	printf("    -rt sec   ：readタイムアウト\n");
	printf("    -wt sec   ：writeタイムアウト\n");
}

/* 切断後にwrite()するとSIGPIPEシグナルによりプログラムが終了するのでインターラプト処理を定義 */
void SigHandler(int signo){
	LogTrainTcpClose();
}

/* メイン処理 */
main(int argc, char **argv) {
	int i;
	int ret;
	int stdbuf_len;
	time_t tm;
	char stdbuf[MAX_STDIN_BUF];

	signal(SIGPIPE, SigHandler);

	/* デフォルト値設定 */
	com_ReadConfig(CONFIG_FILE);
	strcpy(config_name,CONFIG_FILE);
	strcpy(server_name,LOGTRAIN_SERVER);
	server_port=LOGTRAIN_PORT;
	strcpy(cache_dir,CACHE_DIR);
	connect_timeout=CONNECT_TIMEOUT;
	connect_err_wait=CONNECT_ERR_WAIT;
	read_timeout=READ_TIMEOUT;
	write_timeout=WRITE_TIMEOUT;
	cache_limit=CACHE_LIMIT;
	log_kind[0]=NULL;

	/* パラメータ解析 */
	for ( i=1; i < argc; i++){
		if ( strcmp(argv[i],"-d") == 0 ){
			debug_flg=1;
			continue;
		}
		if ( strcmp(argv[i],"-i") == 0 ){
			ini_wait=0;
			continue;
		}

		if ( i < argc -1 ){
			if ( strcmp(argv[i],"-f") == 0 ){
				strcpy(config_name,argv[++i]);
				com_ReadConfig(config_name);
				continue;
			}
			if ( strcmp(argv[i],"-h") == 0 ){
				strcpy(server_name,argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-p") == 0 ){
				server_port=atoi(argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-c") == 0 ){
				strcpy(cache_dir,argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-ct") == 0 ){
				connect_timeout=atoi(argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-rt") == 0 ){
				read_timeout=atoi(argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-wt") == 0 ){
				write_timeout=atoi(argv[++i]);
				continue;
			}
			if ( strcmp(argv[i],"-l") == 0 ){
				cache_limit=atoi(argv[++i]);
				continue;
			}
		}
		if ( argv[i][0] == '-' ){
			usage(argv[0]);
			exit(1);
		}
		strcpy(log_kind,argv[i]);
	}
	if ( log_kind[0] == NULL ){
		usage(argv[0]);
		exit(1);
	}
	if ( com_fileCheck(cache_dir) != 1 ){
		printf("no directory (%s)\n",cache_dir);	
		exit(1);
	}

	com_logPrint("-------parameter----------");
	com_logPrint("log kind         = %s",log_kind);
	com_logPrint("config           = %s",config_name);
	com_logPrint("server           = %s",server_name);
	com_logPrint("port             = %d",server_port);
	com_logPrint("cache_dir        = %s",cache_dir);
	com_logPrint("connect timeout  = %d",connect_timeout);
	com_logPrint("connect err wait = %d",connect_err_wait);
	com_logPrint("read timeout     = %d",read_timeout);
	com_logPrint("write timeout    = %d",write_timeout);
	com_logPrint("cache limit      = %d",cache_limit);
	com_logPrint("--------------------------");

	/* サーバ接続 */
	LogTrainTcpConnect();

	/* 初期処理でサーバ接続＆接続待ちを行う */
	if ( ini_wait == 1 ){
		
		/* サーバ接続待ち */
		ret=tcp_select(1,connect_timeout);
		if ( ret != 1 ){
			com_logPrint("connect error");
			LogTrainTcpClose();
			exit(1);
		}

		/* 初期データ送信 */
		LogTrainTcpWrite(LT_DATA_INI, log_kind, strlen(log_kind) );
		proc_status=PROC_STS_DATA;

		/* 応答待ち */
		ret=LogTrainTcpRead(read_timeout);
		if ( ret != 1 ){
			com_logPrint("LogTrainTcpRead()=%d",ret);
			LogTrainTcpClose();
			exit(1);
		}
		proc_status=PROC_STS_WAIT; 
	}

	/* 標準入力データをサーバに転送 */
	while(1){

		/* STDIN/TCPの入力チェック(read_timeout待ち有り) */
		ret=com_select();

		/* 標準入力 */
		stdbuf_len=LogTrainStdRead(stdbuf);
		if ( stdbuf_len > 0 ){
			if ( proc_status == PROC_STS_DATA || proc_status == PROC_STS_LOG || proc_status == PROC_STS_WAIT ){
				LogTrainTcpWrite(LT_DATA_LOG_R, stdbuf, stdbuf_len);
				proc_status=PROC_STS_LOG;
			}else{
				LogTrainCacheWrite(stdbuf,stdbuf_len);
			}
		}

		/* サーバ接続 */
		if ( proc_status == PROC_STS_INI ){
			LogTrainTcpConnect();
		}

		/* 接続待ち中の処理 */
		if ( proc_status == PROC_STS_CON ){
			ret=tcp_select(1,0);

			/* コネクトエラー */
			if ( ret < 0 ){
				com_logPrint("connect error");
				LogTrainTcpClose();

			/* 未接続 */
			}else if ( ret == 0 ){
				if ( (sv_connect_time + connect_timeout) < time(&tm) ){
					com_logPrint("connect timeout");
					LogTrainTcpClose();
				}

			/* コネクト正常時は初期データを送信 */
			}else if ( ret == 1 ){
				com_logPrint("connect ok");
				LogTrainTcpWrite(LT_DATA_INI, log_kind, strlen(log_kind) );
				proc_status=PROC_STS_DATA;
			}
		}

		/* TCP入力 */
		if ( proc_status == PROC_STS_DATA || proc_status == PROC_STS_LOG || proc_status == PROC_STS_WAIT ){
			ret=LogTrainTcpRead(0);
			if ( ret == 1 ){ proc_status=PROC_STS_WAIT; }
			if ( ret < 0 ){ LogTrainTcpClose(); }
		}

	}

	exit(0);
}

