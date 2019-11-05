/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

import static java.lang.System.exit;
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
        int port = -1;
        String serverAddr = null;
        if(args.length % 2 != 0){
            System.out.println("Niepoprawna ilość argumentów");
            exit(-1);
        }
        for(int i = 0; i < args.length; i += 2){
            switch(args[i]){
                case "-p": 
                    port = Integer.parseInt(args[i + 1]);
                    break;
                case "-a":
                    serverAddr = args[i + 1];
                    break;
                default:
                    System.out.println("Niepoprawny argument nr " + (i/2 + 1));
                    exit(-1);
            }
        }
        ServerConnetioner conn = new ServerConnetioner(port, serverAddr);
        MainWindowFrame.main(args);
    }
    
}
