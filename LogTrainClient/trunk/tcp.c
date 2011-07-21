#include "LogTrainClient.h"

/* サーバ接続 */
int tcp_connect(){
	int ret;
	int flags;
	struct sockaddr_in client_addr;

	/*
	 * client_addr構造体に、接続するサーバのアドレス・ポート番号を設定
	 */
	bzero((char *)&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(server_name);
	client_addr.sin_port = htons(server_port);

	/* ソケットを生成 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		sockfd=(-1);
		return -1;
	}

	/* 非同期に設定 */
	flags = fcntl(sockfd, F_GETFL, 0);
	if ( flags < 0 ){
		sockfd=(-1);
		close(sockfd);
		return -2;
	}
	ret = fcntl(sockfd, F_SETFL, flags|O_NONBLOCK);
	if ( ret < 0 ){
		sockfd=(-1);
		close(sockfd);
		return -3;
	}

	/* サーバに接続 */
	ret=connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));
	return 0;
}

/* ディスクプリタチェック */
int tcp_select(int type, int tm) {
	int ret;
	fd_set cset;
	struct timeval timer;

	if ( sockfd == (-1) ){ return -1; }

	/* 検査するディスクリプターの設定 */
	FD_ZERO(&cset);
	FD_SET(sockfd, &cset);

	/* タイムアウト値設定 */
	timer.tv_sec = tm;
	timer.tv_usec = 0;
	switch(type){
	case 0: /* 読み取り可能チェック */
		ret=select(sockfd+1, &cset, 0, 0, &timer); break;
	case 1: /* 書き込み可能チェック */
		ret=select(sockfd+1, 0, &cset, 0, &timer); break;
	default:
		return -1;
	}

	/* 異常 */
	if ( ret < 0 ){ return -1; }

	/* タイムアウト */
	if ( ret == 0 ){ return 0; }
	if ( FD_ISSET(sockfd, &cset) == 0){ return 0; }

	return 1;
}

/* 送信 */
int tcp_write(char *buf, int buf_len){
	int ret;
	if ( sockfd == (-1) ){ return -1; }
	ret=write(sockfd, buf, buf_len);
	return ret;
}

/* 受信 */
int tcp_read(char *buf, int buf_len){
	int ret;
	if ( sockfd == (-1) ){ return -1; }
	ret=read(sockfd, buf, buf_len);
	return ret;
}

/* ソケットをクローズ */
int tcp_disconnect(){
	if ( sockfd == (-1) ){ return -1; }
	close(sockfd);
	sockfd=(-1);
	return 0;
}

