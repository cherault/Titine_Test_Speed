#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QByteArray>
#include <QtCore>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/tracking.hpp>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace cv;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //--OpenCV--//
    //----------//
    Mat frame, image;
    VideoCapture cap;

    //--reticule select--//
    //-------------------//
    bool reticule = false;

private slots:
    //--lecture frame--//
    //-----------------//
    void Frame();

    //--read serial--//
    //---------------//
    void readSerial();

    //--video start/stop--//
    //--------------------//
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    //--check reticule--//
    //------------------//
    void on_checkBox_reticule_clicked(bool checked);

    //--check couleur--//
    //-----------------//
    void on_radioButton_blanc_clicked();
    void on_radioButton_noir_clicked();
    void on_radioButton_rouge_clicked();
    void on_radioButton_vert_clicked();
    void on_radioButton_bleu_clicked();

    //--fonction dessin--//
    //-------------------//
    void Square(Mat&, Point, const Scalar&, int, int, int);
    void TriangleDown(Mat&, Point, const Scalar&, int, int, int);

private:
    Ui::MainWindow *ui;

    //--ID arduino--//
    //--------------//
    QSerialPort *arduino;

    static const quint16 arduino_uno_vendor_id = 1027; //nano
    static const quint16 arduino_uno_product_id = 24577; //nano

    //--serial + parsing--//
    //--------------------//
    QByteArray serialData;
    QString serialBuffer;

    QString parseData1; //vitesse

    //--def. couleurs--//
    //-----------------//
    Scalar blanc = Scalar(255,255,255);
    Scalar rouge = Scalar(0,0,255);
    Scalar vert = Scalar(0,255,0);
    Scalar bleu = Scalar(255,0,0);
    Scalar noir = Scalar(0,0,0);

    Scalar couleur;

    //--reticule--//
    //------------//
    Point center;
    Point pt1, pt2;
    Point pt3, pt4;
    Point pt5, pt6;
    Point pt7, pt8;

    //--cst. echelle--//
    //----------------//
    int offset = 20;
    int taille = 60;
    int epais = 40;
    int delta = 50;
    int scale = 10;
    int vMax = 120;

    //--data gps--//
    //------------//
    int speed;
    int speedScale;

    //--joystick--//
    //------------//
    int x=0;
    int y=0;

    int scaleX=0;
    int scaleY=0;

    bool select = false;
    bool release = false;

    //--tracker--//
    //-----------//
    Rect2d boundingBox;
    Point centroid;
};

#endif // MAINWINDOW_H
