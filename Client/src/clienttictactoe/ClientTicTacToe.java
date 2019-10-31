/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

import static java.lang.Thread.sleep;
import java.net.InetAddress;
import java.net.Socket;
import java.util.logging.Level;
import java.util.logging.Logger;
import window.MainWindowFrame;

/**
 *
 * @author kamil
 */
public class ClientTicTacToe {

    /**
     * number of port for connection with server
     */
    public static int port;
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        MainWindowFrame.main(args);
        /*System.out.println("hello world :)");
        try {
            port = 1235;
            InetAddress addr;
            Socket sock = new Socket("localhost", port);
            addr = sock.getInetAddress();
            System.out.println("Connected to " + addr);
            try {
                sleep(10000);
            } catch (InterruptedException ex) {
                Logger.getLogger(ClientTicTacToe.class.getName()).log(Level.SEVERE, null, ex);
            }
            sock.close();
        } 
        catch (java.io.IOException e) {
            System.out.println("Can't connect to " + args[0]);
            System.out.println(e);
        }*/
    }
    
}
