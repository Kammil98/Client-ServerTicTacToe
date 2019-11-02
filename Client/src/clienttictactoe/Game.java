/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.Timer;
import window.GameJPanel;

/**
 *
 * @author kamil
 */
public class Game  implements ActionListener{
    
    private final Timer timer;
    private final GameJPanel panel;
    private final char turn;
    private final char sign;
    /**
     * Connections per second with server
     */
    private int CPS;
    
    private void clearBoard(){
        char[] board;
        board = new char[9];
        for(int i = 0; i < 9; i++)
            board[i] = '-';
        panel.setBoard(board);
    }
    /**
     * Initializer of Game class
     * @param panel JPanel to draw on
     * @param CPS Connections per second with server
     */
    public Game(GameJPanel panel, int CPS){
        this.CPS = CPS;
        this.panel = panel;
        String msg = null;
        while(msg == null)
            msg = ServerConnetioner.readMsg();
        this.turn = msg.charAt(0);
        this.sign = msg.charAt(1);
        clearBoard();
        System.out.println("Initialized game.\n" + this.turn + " starts.\nYour sign is " + this.sign);
        timer = new Timer(1000 / CPS, this);
        timer.setInitialDelay(0);
        timer.start();
    }

    @Override
    public void actionPerformed(ActionEvent e) {
        
    }

    /**
     * @return the CPS
     */
    public int getCPS() {
        return CPS;
    }

    /**
     * @param CPS CPS to set
     */
    public void setCPS(int CPS) {
        this.CPS = CPS;
        timer.setDelay(1000 / CPS);
    }
}
