#include "LogTrainClient.h"

/* 標準入力からの入力 */
int LogTrainStdRead(char *stdbuf){
	int ret;
	int stdbuf_len=0;

	/* 読み込み可能か(no wait) */
	ret=std_select(0, 0);
	if ( ret == 0 ){ return 0; }

	if ( ret == 1 ){
		stdbuf_len=std_read(stdbuf,MAX_STDIN_BUF);
		com_logPrint("std_read()=%d",stdbuf_len);
		if ( stdbuf_len <= 0 ){ exit(1); }
		return stdbuf_len;
	}

	return -2;
}

/* LogTrainServerへの接続 */
int LogTrainTcpConnect(){
	int ret;
	time_t tm;

	/* 最後にconnectしてからconnect_err_wait秒間は実施しない */
	time(&tm);
	if ( (sv_connect_time + connect_err_wait) > tm ){
		return 1;
	}

	/* サーバへのコネクト */
	ret=tcp_connect();
	com_logPrint("tcp_connect(%s:%d)=%d",server_name,server_port,ret);
	sv_connect_time=tm;
	proc_status=PROC_STS_CON;

	return 0;
}

/* LogTrainServerへのデータ送信 */
int LogTrainTcpWrite(int kind, char *buf, int buf_len){
	int ret;
	LOGTRAIN_FMT outbuf;
	int out_len;

	/* フォーマット変換 */
	outbuf.hd.kind=com_nborder(kind);
	outbuf.hd.size=com_nborder(buf_len);
	memcpy(outbuf.buf, buf, buf_len);
	out_len=buf_len + sizeof(LOGTRAIN_FMT_HEADER);

	/* 書き込み可能か(非同期IOなので待たない) */
	ret=tcp_select(1, write_timeout);

	if ( ret == 1 ){
		ret=tcp_write((char *)&outbuf, out_len);
		com_logPrint("tcp_write(%d)=%d",kind,ret);
		if ( ret != out_len ){ return -1; }
		return 1;
	}
	return -2;
}

/* LogTrainServerからのデータ受信 */
int LogTrainTcpRead(int tm){
	int i;
	int ret;
	char inbuf[1024];

	/* 読み込み可能か(no wait) */
	ret=tcp_select(0, tm);
	if ( ret == 0 ){ return 0; }

	if ( ret == 1 ){
		ret=tcp_read((char *)&inbuf, sizeof(inbuf));
		com_logPrint("tcp_read()=%d",ret);
		if ( ret <= 0 ){ return -1; }
		return 1;
	}
	return -2;
}

/* LogTranServer切断 */
int LogTrainTcpClose(){
	tcp_disconnect();
	com_logPrint("tcp_disconnect()");
	proc_status=PROC_STS_INI;
}

/* キャッシュファイル出力 */
int LogTrainCacheWrite(char *stdbuf,int stdbuf_len){
	char cache_base[2048];
	char cache_file[2048];
	struct stat buf;
	int ret;

	/* ディレクトリ作成 */
	sprintf(cache_base,"%s%s",cache_dir,log_kind);
	if ( com_allmkdir(cache_base) != 0 ){
		com_logPrint("mkdir error(%s)",cache_base);
		return -1;
	}

	/* ファイル名 */
	sprintf(cache_file,"%s/cache%07d.out",cache_base,getpid());
	
	/* サイズ制限 */
	if ( cache_limit > 0 ){
		if (lstat(cache_file, &buf) == 0 ){
			if ( (buf.st_size + stdbuf_len) > cache_limit ){
				return 1;
			}
		}
	}

	/* 出力 */
	ret=cache_write(cache_file,stdbuf,stdbuf_len);
	com_logPrint("cache_write()=%d",ret);
	return 0;
}

