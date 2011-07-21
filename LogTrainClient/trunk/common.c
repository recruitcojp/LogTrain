#include "LogTrainClient.h"

/* グローバル変数 */
char  config_name[1024];
char  log_kind[1024];
int   sockfd=-1;
int   stdfd=0;
int   debug_flg=0;
int   proc_status=PROC_STS_INI;
int   ini_wait=1;
int   sv_connect_time=0;

char  server_name[1024];
short server_port;
char  cache_dir[1024];
int   cache_limit;
int   connect_timeout;
int   connect_err_wait;
int   read_timeout;
int   write_timeout;

/* ログ出力 */
int com_logPrint(char *msgfmt, ...){
	char mbuf[10240];
	struct tm *dt;
	time_t nowtime;
	va_list argptr;

	if ( debug_flg == 0 ){ return 0; }

	/* 可変引数文字列の編集  */
	va_start(argptr,msgfmt);
	vsprintf(mbuf,msgfmt,argptr);
	va_end(argptr);

	time(&nowtime);
	dt = localtime( &nowtime );

	printf("%04d/%02d/%02d-%02d:%02d:%02d %s\n",
		dt->tm_year + 1900,dt->tm_mon + 1,dt->tm_mday,
		dt->tm_hour,dt->tm_min,dt->tm_sec, mbuf );
	fflush(stdout);

	return 0;
}

/* 文字列セパレート */
char *com_SetParameter(char *s){
	int i,j;
	int flg=0;
	static char ret[1024];

	ret[0]=NULL;
	for(i=0,j=0; s[i]!=NULL; i++){
		if ( s[i] == '=' ){ flg=1; continue; }
		if ( s[i] == '"' ){ continue; }
		if ( s[i] == ' ' ){ continue; }
		if ( s[i] == '\n' ){ continue; }
		if ( s[i] == '\r' ){ continue; }
		if ( flg == 0 ){ continue; }
		ret[j++]=s[i];
		ret[j]=NULL;
	}

	return ret;
}

/* コンフィグファイルの読み込み */
int com_ReadConfig(char *file_name){
	FILE *fp;
	char buf[1024];

	if ( (fp=fopen(file_name,"r")) == NULL ){
		com_logPrint("file open error(%s)",file_name);
		return 1;
	}
	while( (fgets(buf,sizeof(buf),fp)) != NULL ){
		if ( strncmp(buf,"server_name",11) == 0 ){ strcpy(server_name,com_SetParameter(buf)); }
		if ( strncmp(buf,"server_port",11) == 0 ){ server_port=atoi(com_SetParameter(buf)); }
		if ( strncmp(buf,"cache_dir",9) == 0 ){ strcpy(cache_dir,com_SetParameter(buf)); }
		if ( strncmp(buf,"connect_timeout",15) == 0 ){ connect_timeout=atoi(com_SetParameter(buf)); }
		if ( strncmp(buf,"connect_err_wait",16) == 0 ){ connect_err_wait=atoi(com_SetParameter(buf)); }
		if ( strncmp(buf,"read_timeout",12) == 0 ){ read_timeout=atoi(com_SetParameter(buf)); }
		if ( strncmp(buf,"write_timeout",13) == 0 ){ write_timeout=atoi(com_SetParameter(buf)); }
		if ( strncmp(buf,"cache_limit",11) == 0 ){ cache_limit=atoi(com_SetParameter(buf)); }
	}
	fclose(fp);
	return 0;
}

/* ファイルチェック */
int com_fileCheck(char *filename) {
	struct stat stbuf;

	if (lstat(filename, &stbuf) == -1 ) { return -1; }

	/* DIR */
	if ((stbuf.st_mode & S_IFMT) == S_IFDIR ) { return 1; }

	/* FILE */
	if ((stbuf.st_mode & S_IFMT) == S_IFREG ) { return 0; }

	/* LINK */
	if (S_ISLNK(stbuf.st_mode)) { return 2; }

	/* OTHER */
	return 0;
}

/* ネットワークバイトオーダの変換 */
int com_nborder(unsigned int orgValue) {
	int nbValue = 0;
	char *p;
	int i = 0;
	char tmp[sizeof(int)];
	int *ip;

	p = (char *)&orgValue;
	for(i=0; i < (int)sizeof(int); i++){
		tmp[i] = *(p + sizeof(orgValue) -1 - i);
	}
	ip = (int *)tmp;
	nbValue = *ip;

	return nbValue;
}

/* ディレクトリの作成 */
int com_allmkdir(char *bdir){
	int ret;
	int i;
	char wk[2048];

	ret=com_fileCheck(bdir);
	if ( ret == 1 ){ return 0; }
	if ( ret != -1 ){ return -1; }

	wk[0]=NULL;
	for (i=0; ; i++){
		if ( wk[0]!=NULL && (bdir[i]=='/' || bdir[i]==NULL) ){
			if ( com_fileCheck(wk) == (-1) ){
				if ( mkdir(wk,0755) != 0 ){
					com_logPrint("mkdir error(%s)",wk);
					return -1;
				}
			}
		}
		wk[i]=bdir[i];
		wk[i+1]=NULL;

		if ( bdir[i]==NULL ){ break; }
	}

	return 0;
}

/* 入力チェック */
int com_select(){
	int ret;
	int i;
	fd_set rset;
	fd_set wset;
	struct timeval timer;
	int cks=stdfd;

	/* 検査するディスクリプターの設定 */
	FD_ZERO(&rset);
	FD_SET(stdfd, &rset);
	if ( sockfd != (-1) ){
		FD_SET(sockfd, &rset);
		cks=sockfd; 
	}

	/* タイムアウト値設定 */
	timer.tv_sec = read_timeout;
	timer.tv_usec = 0;

	/* 読み取り可能チェック */
	ret=select(cks+1, &rset, 0, 0, &timer); 
	if ( ret < 0 ){ return -1; }

	/* タイムアウトチェック */
	if ( ret == 0 ){ return 0; }
	if ( FD_ISSET(stdfd, &rset) == 1 ){ return 1; }
	if ( sockfd != (-1) ){
   		if ( FD_ISSET(sockfd, &rset) == 1 ){ return 2; }
	}

	return 0;
}

