/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package clienttictactoe.game;

import javax.swing.SwingWorker;

/**
 * Swing worker for reading messages from 
 * server and handle them
 * @author kamil
 */
public class MessageHandler extends SwingWorker<Void, Void>{

    private final Game game;
    
    /**
     * Constructor of MesssageHandler class
     * @param game
     */
    public MessageHandler(Game game){
        this.game = game;
    }
    
    private void handleReceivedMsg(String msg){
        if(msg == null)
            return;
        switch(msg.charAt(0)){
            case 'n'://new Game
                game.clearBoard();
                game.getPanel().setGameState("Połączono");
                game.setTurn(msg.charAt(1));
                game.setSign(msg.charAt(2));
            case 't'://change turn
                game.setTurn(msg.charAt(1));
                break;
            case 's'://set sign on postion
                game.getPanel().setSign(msg.charAt(1), msg.charAt(2) - '0');
                break;
            case 'w'://someone won
                game.endGame(msg);
                break;
            case 'S'://search new Game
                game.setSign(msg.charAt(1));
                game.getPanel().setGameState("Oczekiwanie");
        }
    }
    
    @Override
    protected Void doInBackground() throws Exception {
        String msg;
        msg = ServerConnetioner.readMsg();
        handleReceivedMsg(msg);
        return null;
    }
    
}
