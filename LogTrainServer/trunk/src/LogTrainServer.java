import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Properties;
import java.util.logging.*;

public class LogTrainServer {

	static String hadoop_host;
	static String hadoop_port;
	static String hadoop_outdir;
	static Short server_port;
	static Logger logger = Logger.getLogger(LogTrainServer.class.getName());

	/**
	 * LogTrainServerの設定取得
	 */
	private static void GetPropertie(String filename) {
		Properties p = new Properties();
		try {
			p.load(new FileInputStream(filename)); 
		    hadoop_host = p.getProperty("hadoop_host");
		    hadoop_port = p.getProperty("hadoop_port");
		    hadoop_outdir = p.getProperty("hadoop_outdir");
		    server_port = Short.parseShort(p.getProperty("server_port"));
		} catch (IOException e) {
			e.printStackTrace();
		    System.exit(-1);
		}
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		//Log
		logger.log(Level.INFO,"start");

		//プロパティ
		String filename = args[0]; 
		GetPropertie(filename);
		logger.log(Level.INFO,"hadoop_host："+hadoop_host);
		logger.log(Level.INFO,"hadoop_port："+hadoop_port);
		logger.log(Level.INFO,"hadoop_outdir："+hadoop_outdir);
		logger.log(Level.INFO,"server_port："+server_port);
		
		try	{
			//ソケット生成
			ServerSocket serverSocket = new ServerSocket(server_port);

			//クライアントから接続があった場合にスレッド生成
			while(true){
				//クライアント受付
				Socket socket = serverSocket.accept();
				String cip=socket.getInetAddress().getHostAddress();
				logger.log(Level.INFO,"connect from "+cip);
				
				//スレッド生成
				LogTrainServerThread tr1 = new LogTrainServerThread();
				tr1.setIPaddress(cip);
				tr1.setSocket(socket);
				tr1.start();
			}

		}catch(Exception e)	{
			e.printStackTrace();
			logger.log(Level.SEVERE,"ubnormal end");
			System.exit(1);
		}	

		logger.log(Level.INFO,"normal end");
		System.exit(0);
	}

}
