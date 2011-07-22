***************************************************************************

  apacheアクセスログのリアルタイムHDFS保存ツール「LogTrain」

                    - Copyright 2011 Recruit -
***************************************************************************

           ダウンロードいただきましてありがとうございます。

---------------------------------------------------------------------------
■はじめに
---------------------------------------------------------------------------

　本ドキュメントは、「LogTrain」に関する推奨環境や導入に関する情報を提供します。

---------------------------------------------------------------------------
■LogTrainとは
---------------------------------------------------------------------------

　LogTrainはapacheアクセスログをTCP/IP経由でリアルタイムにログを転送しhadoop
のHDFSへ保存するツールです。アクセスの多いサイトでは大量に出力されるapacheア
クセスログの影響によりディスクIO負荷、領域不足、ログ解析バッチ処理遅延といっ
た課題に直面しています。LogTrainを利用する事でそれらの問題が解決します。

---------------------------------------------------------------------------
■動作環境
---------------------------------------------------------------------------

本ツールでの動作確認済み環境は以下となります。

　・Linux 2.6
　・Apache 2.2
　・hadoop 0.20.2
　・hive 0.6
　・JDK1.6.0_23

---------------------------------------------------------------------------
■ご利用条件
---------------------------------------------------------------------------

　本ツール一式は使用もしくは再配布について、無料でご利用いただけます。

　LogTrain配布アーカイブに含まれる著作物に対する権利は、Recruitが保有してお
り、GNU一般公衆利用許諾契約に基づいて配布しております。再配布・改変等は契約
の範囲内で自由に行うことが出来ます。詳しくは、添付のGNU一般公衆利用許諾契約
書をお読みください。

　なお、本ツールは一般的な利用において動作を確認しておりますが、ご利用の環
境や状況、設定もしくはプログラム上の不具合等により期待と異なる動作をする場
合が考えられます。本ツールの利用に対する効果は無保証であり、あらゆる不利益
や損害等について、当方は一切の責任をいたしかねますので、ご了承いただきます
ようお願い申し上げます。

---------------------------------------------------------------------------
■LogTrain構成
---------------------------------------------------------------------------

LogTrainはLogTrainServer、LogTrainClient、LogTrainCacheの３つのソフトウェアか
ら構成されます。

　LogTrainClient：ログをLogTrainServerにリアルタイムで転送
　LogTrainServer：LogTrainClientから送られたログをHDFS上に蓄積
　LogTrainCache：サーバ障害時にLocal Cacheに貯められたログを転送


  「Web Server」 　　　　　     「hadoop master Node」

　　　Apache
　　　　｜
　LogTrainClient　－－－－－－＞　LogTrainServer -> (HDFS) -> ログ解析処理
　　　　｜　　　　　　　｜　
　 (LocalCache) 　　　　｜　
　　　　｜　　　　　　　｜　
　LogTrainCache 　－－－┘


---------------------------------------------------------------------------
■インストール方法
---------------------------------------------------------------------------

○共通手順
(1)githubよりLogTrainをダウンロード
(2)ダウンロードしたファイルを展開
   #tar xvfz recruitcojp-LogTrain-xxxxx.tar.gz
(3)ディレクトリ作成
   #mkdir -p /usr/local/logtrain/bin
   #mkdir -p /usr/local/logtrain/cache
   #mkdir -p /usr/local/logtrain/log
   #mkdir -p /usr/local/logtrain/temp

○LogTriinServerの導入方法(hadoopマスターノードで実行)
(1)コンパイル
   #cd recruitcojp-LogTrain-xxxxx/LogTrainServer/trunk
   #ant
(2)ファイルの配置   
   #cp ext/LogTrainServer.jar /usr/local/logtrain/bin
   #cp resources/* /usr/local/logtrain/bin
   #cp shell/LogTrainServer /etc/init.d
(3)自動起動設定
   #chkconfig LogTrainServer on
   #chkconfig --list LogTrainServer
(4)LogTrainServer起動
   #service LogTrainServer start
(5)LogTrainServer停止
   #service LogTrainServer stop

○LogTrannClientの導入方法(Webサーバで実行)
(1)ソースのコンパイルとインストール
   #cd recruitcojp-LogTrain-xxxxx/LogTrainClient/trunk
   #make
   #make install(*)
   (*)/usr/local/logtrain/binにコピーされます（install先を変更したい場合はMakefileを変更してください） 
(2)apache設定を変更
   #vi httpd.conf

   例) CustomLog "|/usr/local/logtrain/bin/LogTrainClient xxxx" combined
   xxxx部分はHDFSパスの一部となります

   #service httpd restart


　　　　　　　　　　　　　　　　- 以上 -

