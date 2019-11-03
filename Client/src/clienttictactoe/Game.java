/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Arrays;
import javax.swing.JOptionPane;
import javax.swing.Timer;
import window.GameJPanel;

/**
 *
 * @author kamil
 */
public class Game  implements ActionListener{

    private final Timer timer;
    private final GameJPanel panel;
    private static char turn;
    private static char sign;
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
        Game.turn = 'n';
        Game.sign = 'n';
        clearBoard();
        timer = new Timer(1000 / CPS, this);
        timer.setInitialDelay(0);
        timer.start();
    }
    private void handleReceivedMsg(String msg){
        if(msg == null)
            return;
        System.out.println(msg);
        switch(msg.charAt(0)){
            case 'n'://new Game
                clearBoard();
                Game.turn = msg.charAt(1);
                Game.sign = msg.charAt(2);
                System.out.println("Initialized game.\n" + Game.turn + " starts.\nYour sign is " + Game.sign);
            case 't'://change turn
                turn = msg.charAt(1);
                break;
            case 's'://set sign on postion
                panel.setSign(msg.charAt(1), msg.charAt(2) - '0');
                break;
            case 'w'://someone won
                int n;
                String dialogMsg;
                String[] options = {"Tak", "Nie"};
                if(msg.charAt(1) == Game.sign)
                    dialogMsg = "Gratulacje, wygrałeś!";
                else if(msg.charAt(1) == 'd')
                    dialogMsg = "Remis.";
                else if(msg.charAt(1) == 'X' || msg.charAt(1) == 'O')
                    dialogMsg = "Wygrałeś przez walkower. Drugi gracz się rozłączył.";
                else 
                    dialogMsg = "Przegrałeś :(";
                dialogMsg += "\nCzy chcesz zagrać ponownie?";
                n = JOptionPane.showOptionDialog(null, dialogMsg, null, 
                        JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE, 
                        null, options, null);
                if(n == 0)//Want to play again
                    msg = "ry" + '\n';
                else
                    msg = "rn" + '\n';
                System.out.println("n= " + n);
                System.out.println(msg);
                ServerConnetioner.writeMsg(msg);
                break;
        }
    }
    
    
    /**
     * Read messages from server and handle them
     * @param e event
     */
    @Override
    public void actionPerformed(ActionEvent e) {
        String msg = null;
        msg = ServerConnetioner.readMsg();
        handleReceivedMsg(msg);
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
    
    
    /**
     * @return the turn
     */
    public static char getTurn() {
        return turn;
    }

    
    /**
     * @return the sign
     */
    public static char getSign() {
        return sign;
    }
    
}
