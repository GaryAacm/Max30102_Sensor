#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QPixmap>
#include "MaxPlot.h"
#include "QRCodeGenerator.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void Setup_Window_Button();
    void Setup_Layout();
    void Setup_Background();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onGenerateQRClicked();
    void onStartClicked();
    void onExitClicked();
    void sendExitMessage();

private:
    QPushButton *generateQRButton;
    QPushButton *startButton;
    QPushButton *exitButton;
    QPixmap backgroundPixmap;
    MaxPlot *maxPlotWindow;
    QRCodeGenerator *qr;
    std::string User_Message;

};

#endif // MAINWINDOW_H
