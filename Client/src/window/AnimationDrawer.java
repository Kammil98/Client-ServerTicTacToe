/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package window;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JPanel;
import javax.swing.Timer;

/**
 * Class to draw animations
 * @author kamil
 */
public class AnimationDrawer implements ActionListener{

    /**
     * Initializer of AnimationDrawer class
     * @param panel JPanel to draw on
     * @param FPS Frames per Second
     */
    public AnimationDrawer(JPanel panel, int FPS){
        this.FPS = FPS;
        this.panel = panel;
        timer = new Timer(1000 / FPS, this);
        timer.setInitialDelay(0);
        timer.start();
    }
    private final Timer timer;
    private JPanel panel;
    private int FPS;
    
    /**
     * Repaint Active Panel in MainWindow 
     * every quant of of time
    * @param e event
    */
    @Override
    public void actionPerformed(ActionEvent e) {
        getPanel().repaint();
    }

    /**
     * @return the panel
     */
    public JPanel getPanel() {
        return panel;
    }

    /**
     * @param panel the panel to set
     */
    public void setPanel(JPanel panel) {
        this.panel = panel;
    }

    /**
     * @return the FPS
     */
    public int getFPS() {
        return FPS;
    }

    /**
     * @param FPS the FPS to set
     */
    public void setFPS(int FPS) {
        this.FPS = FPS;
        timer.setDelay(1000 / FPS);
    }
}
