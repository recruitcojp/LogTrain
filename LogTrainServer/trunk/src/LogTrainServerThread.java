import java.io.*;
import java.nio.*;
import java.net.*;
import java.text.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.apache.hadoop.fs.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.conf.Configuration;

public class LogTrainServerThread extends Thread {
	Socket socket = null;
	String IPaddress=null;
	String LogKind=null;
	
	static Logger logger = Logger.getLogger(LogTrainServerThread.class.getName());
	
	/*
	 * ソケットオブジェクト設定
	 */
  	public void setSocket(Socket socket){
  		this.socket	=	socket;
  	}

  	/*
  	 * クライアントのIPアドレス設定
  	 */
  	public void setIPaddress(String inetAddress) {
  		this.IPaddress=inetAddress;
	}
  	
  	/*
  	 * byte配列からint型への変換
  	 */
  	public int byte2int(byte[] buf,int offset,int length){
  		ByteArrayInputStream bais = new ByteArrayInputStream(buf,offset,length);
  		DataInputStream in2 = new DataInputStream(bais);
  		try{
  			int val = in2.readInt();
  			return val;
		}catch(Exception e)	{
			e.printStackTrace();
		}
  		return 0;
  	}
  	
  	/*
  	 * クライアントからのソケット受信処理
  	 */
  	private ReturnValue readWrap(InputStream is1){
  		ReturnValue value = new ReturnValue();
  		int i;
  		int n;
  		byte buf[] = new byte[1048576];

  		//リターン値初期化
  		value.ret=0;
  		value.kind=0;
  		value.size=0;

  		//ソケット受信処理
  		try{
			socket.setSoTimeout(10000);
			n = is1.read(buf, 0, buf.length);
			socket.setSoTimeout(0);
			if ( n < 0 ){
				value.ret=-1;
			}else if ( n <= 8 ){
				value.ret=0;
			}else{
				value.kind=byte2int(buf,0,4);
				value.size=byte2int(buf,4,4);
				value.ret=1;
				for (i = 0; i < value.size; i++) { 
					value.buf[i]=buf[i+8];
					//System.out.printf("%02X ",value.buf[i]); 
				}
				//System.out.println("k=" + Integer.toString(value.kind) + " s=" + Integer.toString(value.size));
			}
  		} catch (SocketTimeoutException e) {
			value.ret=0;
		}catch(Exception e)	{
			e.printStackTrace();
			value.ret=-1;
		}

		return value;
  	}
		
	/*
	 * クライアントへ応答送信
	 */
	private int writeWrap(OutputStream os1){
  		byte res[] = new byte[12];

  		//応答
		res[0]=9;	//応答
		res[4]=4;	//データ部のサイズ
		res[8]=0;	//結果（0=正常 / 0以外=異常)
		try{
			os1.write(res);
			os1.flush();
		}catch(Exception e)	{
			e.printStackTrace();
			return -2;
		}

		return 0;
  	}
  	
	/*
	 * スレッドメイン処理
	 */
	public void run() {
		
		OutputStream output = null;
		String SV_hdfsPathString=null;
		String SV_yyyymmddhh=null;
		String SV_LogKind=null;
		String hdfsBase=null;
		ReturnValue tcpbuf = null;
		int ret;

		if ( socket	==	null ) { return; }
		if ( IPaddress	==	null ) { return; }
		
		try	{

			//ソケット初期設定
			InputStream	is1	= socket.getInputStream();
			OutputStream os1 = socket.getOutputStream();
			
			//HDFS初期化
			Configuration hadoopconf = new Configuration();
			
			//ソケト受信とHDFS出力
			while ( true ) {

				//ソケット受信
				tcpbuf=readWrap(is1);
				//logger.log(Level.CONFIG,"read() k=" + Integer.toString(tcpbuf.kind) + " s=" + Integer.toString(tcpbuf.size) + " ret=" +  Integer.toString(tcpbuf.ret));
				if ( tcpbuf.ret < 0 ){
					logger.log(Level.CONFIG,"disconnect from " + IPaddress);
					break;
				}
				
				//応答送信
				if ( tcpbuf.ret > 0 ){
					ret=writeWrap(os1);
					//logger.log(Level.CONFIG,"write() ret=" + Integer.toString(ret));
				}
				
				//初期データリクエスト
				if (tcpbuf.kind == 1){
					LogKind = new String(tcpbuf.buf,0,tcpbuf.size);
					hdfsBase="hdfs://" + LogTrainServer.hadoop_host + ":" + LogTrainServer.hadoop_port + LogTrainServer.hadoop_outdir;
					hdfsBase=hdfsBase.replaceAll("%KIND%",LogKind);
					hdfsBase=hdfsBase.replaceAll("%IP%",IPaddress);
					//logger.log(Level.CONFIG,hdfsBase);
					continue;
				}

				//初期データ受信前は何もしない
				if ( LogKind == null){
					continue;
				}

				//時間取得
				Date date1 = new Date();
				SimpleDateFormat sdf1 = new SimpleDateFormat("yyyyMMddhhmm");
				String yyyymmddhh=sdf1.format(date1);
				
				//ファイル切り替え処理
				if ( yyyymmddhh.equals(SV_yyyymmddhh)==false || LogKind.equals(SV_LogKind)==false){

					//HDFSファイル名生成
					SimpleDateFormat sdf2 = new SimpleDateFormat("yyyyMMddhhmmssSSS");
					String yyyymmddhhmmss=sdf2.format(date1);
					String hdfsPathString = hdfsBase + yyyymmddhhmmss + ".log";

					//HDFSファイルオープン中の場合はクローズ
					if ( output!=null){
						try{
							logger.log(Level.CONFIG,SV_hdfsPathString + " CLOSE");
							output.close();
						} catch (IOException e2) {
							e2.printStackTrace();
						}
					}

					//受信TIMEOUTの場合はHDFSファイルのクローズのみ実行
					if (tcpbuf.ret == 0 ){
						output=null;
						SV_hdfsPathString=null;
						continue;
					}

					//HDFSファイルのオープン
					Path hdfsPath = new Path(hdfsPathString);
					FileSystem fs = hdfsPath.getFileSystem( hadoopconf );
					output = fs.create(hdfsPath);

					//セーブ
					SV_LogKind=LogKind;
					SV_yyyymmddhh=yyyymmddhh;
					SV_hdfsPathString=hdfsPathString;
					logger.log(Level.CONFIG,hdfsPathString + " OPEN");
				}
				
				if (tcpbuf.ret == 0 ){
					continue;
				}
				
				//HDFSへの出力
				InputStream input = new ByteArrayInputStream(tcpbuf.buf,0,tcpbuf.size);
				IOUtils.copyBytes(input, output, hadoopconf, false);
			
			}
				
			//ソケットのクローズ
			is1.close();
			os1.close();
			socket.close();
			
		}catch(Exception e)	{
			e.printStackTrace();
		} finally {
			if (output != null) {
				try{
					logger.log(Level.CONFIG,SV_hdfsPathString + " CLOSE");
					output.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}


}
