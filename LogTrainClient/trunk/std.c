#include "LogTrainClient.h"

/* ディスクプリタチェック */
int std_select(int type, int tm) {
    int ret;
    fd_set cset;
    struct timeval timer;

    /* 検査するディスクリプターの設定 */
    FD_ZERO(&cset);
    FD_SET(stdfd, &cset);

    /* タイムアウト値設定 */
    timer.tv_sec = tm;
    timer.tv_usec = 0;
    switch(type){
    case 0: /* 読み取り可能チェック */
        ret=select(stdfd+1, &cset, 0, 0, &timer); break;
    case 1: /* 書き込み可能チェック */
        ret=select(stdfd+1, 0, &cset, 0, &timer); break;
    default:
        return -1;
    }

    /* 異常 */
    if ( ret < 0 ){ return -1; }

    /* タイムアウト */
    if ( ret == 0 ){ return 0; }
    if ( FD_ISSET(stdfd, &cset) == 0){ return 0; }

    return 1;
}

/* 送信 */
int std_write(char *buf, int buf_len){
    int ret;
    ret=write(stdfd, buf, buf_len);
    return ret;
}

/* 受信 */
int std_read(char *buf, int buf_len){
    int ret;
    ret=read(stdfd, buf, buf_len);
    return ret;
}

