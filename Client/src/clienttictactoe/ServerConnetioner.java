/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Create connection between 
 * Server and Client
 * @author kamil
 */
public class ServerConnetioner {
    
    
    private static int port;
    private static Socket socket;
    private static BufferedWriter bw;
    private static BufferedReader br;
    /**
     * Initializer ServerConnetioner class
     * @param port number of port to connect with server
     */
    public ServerConnetioner(int port){
        this(port, "localhost");
    }
    
    
    /**
     * Initializer ServerConnetioner class
     * @param IPaddr server IP
     */
    public ServerConnetioner(String IPaddr){
        this(1235, IPaddr);
    }
    
    
    /**
     * Initializer ServerConnetioner class
     */
    public ServerConnetioner(){
        this(1235, "localhost");
    }
    
    
    /**
     * Initializer ServerConnetioner class
     * @param port number of port to connect with server
     * @param IPaddr server IP
     */
    public ServerConnetioner(int port, String IPaddr){
            this.port = port;
            OutputStream os = null;
            InputStream is = null;
            InetAddress addr = null;
            socket = null;
            try {
                socket = new Socket("localhost", port);
                addr = socket.getInetAddress();
                os = socket.getOutputStream();
                is = socket.getInputStream();
            } catch (IOException ex) {
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
            }
            bw = new BufferedWriter(new OutputStreamWriter(os));
            br = new BufferedReader(new InputStreamReader(is));
            System.out.println("Connected to " + addr);
            
    }
    
    /**
     * @return next msg or nul if eof is reached
     */
    public static String readMsg(){
        String msg = null;
        try {
            msg = br.readLine();
        } catch (IOException ex) {
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
        }
        return msg;
    }
    public static void writeMsg(String msg){
        try {
            bw.write(msg);
            bw.flush();
        } catch (IOException ex) {
            System.out.println("Failed to send msg \"" + msg + "\"");
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    /**
     * Destructor of ServerConnetioner class
     */
    public static void closeConnection(){
        try {
            br.close();
            bw.close();
            socket.close();
        } catch (IOException ex) {
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
