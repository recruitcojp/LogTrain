#include "LogTrainClient.h"

/* 利用方法 */
void usage(char *prog){
	printf("%s [option] ログ種別\n",prog);
	printf("\n");
	printf("    [オプション]\n");
	printf("    -d        ：デバックモード\n");
	printf("    -f file   ：コンフィグファイル名\n");
	printf("    -h server ：接続先サーバのIPアドレス\n");
	printf("    -p port   ：ポート番号\n");
	printf("    -c dir    ：キャッシュディレクトリ\n");
	printf("    -ct sec   ：接続タイムアウト\n");
	printf("    -rt sec   ：readタイムアウト\n");
	printf("    -wt sec   ：writeタイムアウト\n");
}

/* 切断後にwrite()するとSIGPIPEシグナルによりプログラムが終了するのでインターラプト処理を定義 */
void SigHandler(int signo){
	LogTrainTcpClose();
}

/* キャッシュファイルのリモート転送 */
int LogTrainTransFile(char *dirname,char *filename){
	int ret;
	int i;
	int fd;
	char filename1[2048];
	char filename2[2048];
	char log_kind[2048];
	char buf[1024];
	int  buf_len;

	sprintf(filename1,"%s%s",dirname,filename);
	sprintf(filename2,"%s%s",dirname,filename);

	/* キャッシュファイルか？ */
	if ( strncmp(filename,"cache",5) != 0 ){
		com_logPrint("%s -> no cache file",filename1);
		return 0;
	}
	if ( strcmp(filename+strlen(filename)-4,".out",4) == 0 ){
		sprintf(filename2,"%s%s.tmp",dirname,filename);
		rename(filename1,filename2);
	}

	/* ログ種別取得 */
	strcpy(log_kind, dirname + strlen(cache_dir));
	if ( log_kind[strlen(log_kind)-1] == '/' ){
		log_kind[strlen(log_kind)-1]=NULL;
	}

	/* 初期データ送信 */
	LogTrainTcpWrite(LT_DATA_INI, log_kind, strlen(log_kind) );

	/* 応答待ち */
	ret=LogTrainTcpRead(read_timeout);
	if ( ret != 1 ){
		com_logPrint("LogTrainTcpRead()=%d",ret);
		exit(1);
	}

	/* ファイルオープン */
	if ( (fd=open(filename2,O_RDONLY)) < 0 ){
		com_logPrint("%s -> open error",filename2);
		exit(1);
	}

	/* ファイルロック */
	if (flock(fd, LOCK_EX) != 0){
		com_logPrint("%s -> lock error",filename2);
		exit(1);
	}

	/* サーバへ転送 */
	while( (buf_len=read(fd,buf,sizeof(buf))) > 0 ){
		LogTrainTcpWrite(LT_DATA_LOG_P, buf, buf_len );
		ret=LogTrainTcpRead(read_timeout);
		if ( ret != 1 ){
			com_logPrint("LogTrainTcpRead()=%d",ret);
			exit(1);
		}
	}

	flock(fd, LOCK_UN);
	close(fd);

	/* 処理したファイルを削除 */
	ret=unlink(filename2);
	if ( ret != 0 ){
		com_logPrint("%s -> unlink error",filename2);
		exit(1);
	}

	com_logPrint("%s",filename1);
	return 0;
}

/*キャッシュディレクトリ配下を再帰的に処理 */
int LogTrainTrans(char *dirname){
	int ret;
	DIR *dir;
	struct dirent *dp;
	char filename[2048];

	if ( dirname[strlen(dirname)-1] != '/' ){
		strcat(dirname,"/");
	}

	if((dir=opendir(dirname))==NULL){
		com_logPrint("opendir error");
		exit(1);
	}
	while( (dp=readdir(dir)) != NULL ){
		if ( strcmp(dp->d_name,".") == 0 ){ continue; }
		if ( strcmp(dp->d_name,"..") == 0 ){ continue; }
		sprintf(filename,"%s%s",dirname,dp->d_name);
		ret=com_fileCheck(filename);
		if ( ret == 0 ){ LogTrainTransFile(dirname,dp->d_name); }
		if ( ret == 1 ){ LogTrainTrans(filename); }
	}
	closedir(dir);

	return 0;
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

	/* パラメータ解析 */
	for ( i=1; i < argc; i++){
		if ( strcmp(argv[i],"-d") == 0 ){
			debug_flg=1;
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
		}
		if ( argv[i][0] == '-' ){
			usage(argv[0]);
			exit(1);
		}
	}
	if ( com_fileCheck(cache_dir) != 1 ){
		printf("no directory (%s)\n",cache_dir);	
		exit(1);
	}

	com_logPrint("-------parameter----------");
	com_logPrint("config           = %s",config_name);
	com_logPrint("server           = %s",server_name);
	com_logPrint("port             = %d",server_port);
	com_logPrint("cache_dir        = %s",cache_dir);
	com_logPrint("connect timeout  = %d",connect_timeout);
	com_logPrint("connect err wait = %d",connect_err_wait);
	com_logPrint("read timeout     = %d",read_timeout);
	com_logPrint("write timeout    = %d",write_timeout);
	com_logPrint("--------------------------");

	/* サーバ接続 */
	LogTrainTcpConnect();
	ret=tcp_select(1,connect_timeout);
	if ( ret != 1 ){
		com_logPrint("connect error");
		exit(1);
	}

	/* キャッシュファイル検索とリモート転送処理 */
	LogTrainTrans(cache_dir);

	LogTrainTcpClose();
	exit(0);
}

