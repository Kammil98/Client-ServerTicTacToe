/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package window.listeners;

import clienttictactoe.game.Game;
import clienttictactoe.game.ServerConnetioner;
import java.awt.Point;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import window.GameJPanel;
import window.MainWindowFrame;

/**
 *
 * @author kamil
 */
public class WindowMouseListener implements MouseListener{

    /**
     * Constructor of WindowMouseListener class
     */
    public WindowMouseListener(){
        fieldClicked = -1;
        double width = 500;
        // +5 because lines between fields are 5 pixel width
        fieldWidth = (int) (width / 4) + 5;
    }
    
    int fieldClicked;
    int fieldWidth;
    
    /**
     * Fuction to calculate which field of board 
     * was click.
     * @return value in range <0, 8>. 
     * Number of clicked field
     */
    private int findField(MouseEvent e){
        int place;
        Point p = new Point(e.getX(), e.getY());
        // +5 and -25 because JPanel do not start at pixel (0,0) there is menubar 
        // and a small invisible place of JFrame otside the visible part 
        double width = p.getX() + 5 , height = p.getY() - 25;
        if(width > 3 * fieldWidth)
            return -1;
        if(height > 3 * fieldWidth)
            return -1;
        place =   ((int) (width / fieldWidth) + 
                3 * (int) (height / fieldWidth));
        return place;
    }
    
    
    /**
     * Activate when mouse clicked
     * Check if it is game and send
     * demand of book field to server
     * @param e
     */
    @Override
    public void mouseClicked(MouseEvent e) {
        MainWindowFrame frame = (MainWindowFrame) e.getComponent();
        if(frame.getActivePanel().getClass() == GameJPanel.class){
            fieldClicked = findField(e);
        }
        if(Game.getSign() == Game.getTurn() && fieldClicked != -1 && Game.getSign() != 'n'){
            String msg = String.valueOf(fieldClicked) + '\n';
            ServerConnetioner.writeMsg(msg);
        }
    }
    
    
    /**
     * Activate when mouse pressed
     * do nothing
     * @param e
     */
    @Override
    public void mousePressed(MouseEvent e) {
    }
    
    
    /**
     * Activate when mouse released
     * do nothing
     * @param e
     */
    @Override
    public void mouseReleased(MouseEvent e) {
    }
    
    
    /**
     * Activate when mouse entered component
     * do nothing
     * @param e
     */
    @Override
    public void mouseEntered(MouseEvent e) {
    }
    
    
    /**
     * Activate when mouse exited component
     * do nothing
     * @param e
     */
    @Override
    public void mouseExited(MouseEvent e) {
    }
    
}
