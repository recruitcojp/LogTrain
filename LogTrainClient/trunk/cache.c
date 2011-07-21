#include "LogTrainClient.h"

/* キャッシュファイル出力 */
int cache_write(char *cache_file, char *stdbuf, int stdbuf_len){
	int ret;
	int filefd;

	/* キャッシュファイル出力 */
	if ( (filefd=open(cache_file,O_CREAT|O_WRONLY|O_APPEND,S_IREAD | S_IWRITE)) < 0 ){
		com_logPrint("open error(%s)",cache_file);
		return -1;
	}

	/* ファイルロック */
	if (flock(filefd, LOCK_EX|LOCK_NB) != 0){
		com_logPrint("lock error(%s)",cache_file);
		return -2;
	}

	/* 書き込み */
	ret=write(filefd, stdbuf, stdbuf_len);

	flock(filefd, LOCK_UN);

	close(filefd);

	/* 異常チェック */
	if ( ret != stdbuf_len ){
		com_logPrint("write()=%d",ret); 
		return -3;
	}

	return 0;
}
