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
        if(port == -1)
            port = 1235;
        if(IPaddr == null)
            IPaddr = "localhost";
        ServerConnetioner.port = port;
        InetAddress addr = null;
        OutputStream os = null;
        InputStream is = null;
        socket = null;
        try {
            socket = new Socket(IPaddr, port);
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
     * Read message from server
     * @return next msg or nul if eof is reached
     */
    public static String readMsg(){
        String msg = null;
        try {
            if(br.ready())
                msg = br.readLine();
        } catch (IOException ex) {
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
        }
        return msg;
    }

    
    /**
     * Write message to server
     * @param msg message to send
     */
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
            System.out.println("Zamykam połączenie");
            bw.close();
            br.close();
            socket.close();
        } catch (IOException ex) {
            Logger.getLogger(ServerConnetioner.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
