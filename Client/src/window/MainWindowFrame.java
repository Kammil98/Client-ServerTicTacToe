/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package window;

import clienttictactoe.Game;
import java.awt.BorderLayout;
import javax.swing.JPanel;

/**
 *
 * @author kamil
 */
public class MainWindowFrame extends javax.swing.JFrame {
    
    private JPanel activePanel;
    private final AnimationDrawer drawer;
    private Game game;
    
    /**
     * Creates new form MainWindowFrame
     */
    public MainWindowFrame() {
        super("Tic Tac Toe");
        initComponents();
        addMouseListener(new WindowMouseListener());
        setLayout(new BorderLayout(0, 0));
        activePanel = new GameJPanel();
        add(activePanel, BorderLayout.CENTER);
        activePanel.setVisible(true);
        pack();
        setSize(500, 400);
        setResizable(false);
        drawer = new AnimationDrawer(getActivePanel(), 40);
        game = new Game((GameJPanel)activePanel, 40);
    }

    /**
     * Swap two JPanels
     * @param newPanel - new Panel to show in JFrame
     */
    public void setActivePanel(GameJPanel newPanel) {
        if(getActivePanel() != null)
            getActivePanel().setVisible(false);
        activePanel = newPanel;
        drawer.setPanel(getActivePanel());
        getActivePanel().setVisible(true);
    }
    
    
    /**
     * This method is called from within the constructor to initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is always
     * regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 400, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 300, Short.MAX_VALUE)
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        /* Set the Nimbus look and feel */
        //<editor-fold defaultstate="collapsed" desc=" Look and feel setting code (optional) ">
        /* If Nimbus (introduced in Java SE 6) is not available, stay with the default look and feel.
         * For details see http://download.oracle.com/javase/tutorial/uiswing/lookandfeel/plaf.html 
         */
        try {
            for (javax.swing.UIManager.LookAndFeelInfo info : javax.swing.UIManager.getInstalledLookAndFeels()) {
                if ("Nimbus".equals(info.getName())) {
                    javax.swing.UIManager.setLookAndFeel(info.getClassName());
                    break;
                }
            }
        } catch (ClassNotFoundException | InstantiationException | IllegalAccessException | javax.swing.UnsupportedLookAndFeelException ex) {
            java.util.logging.Logger.getLogger(MainWindowFrame.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        }
        //</editor-fold>
        
        //</editor-fold>

        /* Create and display the form */
        java.awt.EventQueue.invokeLater(new Runnable() {
            @Override
            public void run() {
                new MainWindowFrame().setVisible(true);
            }
        });
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables

    /**
     * @return the activePanel
     */
    public final JPanel getActivePanel() {
        return activePanel;
    }
}
