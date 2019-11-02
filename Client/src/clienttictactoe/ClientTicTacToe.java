/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

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
    }
    
}
