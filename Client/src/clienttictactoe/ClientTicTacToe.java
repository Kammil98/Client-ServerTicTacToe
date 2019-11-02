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
        ServerConnetioner conn = new ServerConnetioner(1235, "localhost");
        MainWindowFrame.main(args);
        ServerConnetioner.closeConnection();
    }
    
}
