#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <string>
#include <QDebug>
#include <QMessageBox>
#include "joystick.h"

//--cste joystick--//
//-----------------//
#define path "/dev/input/js0"
#define sigma 65534
#define value 32767

//--constructeur de classe--//
//--------------------------//
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //--parse data--//
    //--------------//
    serialBuffer = "";

    parseData1 = "";

    //--declare arduino--//
    //-------------------//
    arduino = new QSerialPort(this);

    qDebug() << "Nombre de ports: " << QSerialPortInfo::availablePorts().length() << "\n";

    //--detect ID arduino--//
    //---------------------//
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
        qDebug() << "Description: " << serialPortInfo.description() << "\n";
        qDebug() << "Has vendor id?: " << serialPortInfo.hasVendorIdentifier() << "\n";
        qDebug() << "Vendor ID: " << serialPortInfo.vendorIdentifier() << "\n";
        qDebug() << "Has product id?: " << serialPortInfo.hasProductIdentifier() << "\n";
        qDebug() << "Product ID: " << serialPortInfo.productIdentifier() << "\n";
    }

    bool arduino_is_available = false;
    QString arduino_uno_port_name;

    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
        if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier())
        {
            if((serialPortInfo.productIdentifier() == arduino_uno_product_id)
                    && (serialPortInfo.vendorIdentifier() == arduino_uno_vendor_id))
            {
                arduino_is_available = true;
                arduino_uno_port_name = serialPortInfo.portName();
            }
        }
    }

    //--config port serie--//
    //---------------------//
    if(arduino_is_available)
    {
        qDebug() << "Arduino OK !\n";
        arduino->setPortName(arduino_uno_port_name);
        arduino->open(QSerialPort::ReadOnly);
        arduino->setBaudRate(QSerialPort::Baud115200);
        arduino->setDataBits(QSerialPort::Data8);
        arduino->setFlowControl(QSerialPort::NoFlowControl);
        arduino->setParity(QSerialPort::NoParity);
        arduino->setStopBits(QSerialPort::OneStop);
        QObject::connect(arduino, SIGNAL(readyRead()), this, SLOT(readSerial()));

        ui->label_daq->setText("CONNECTED");
    }
    else
    {
        qDebug() << "Pas de ports corrects pour l'Arduino\n";
        QMessageBox::information(this, "Petit bug...", "Connecte l'Arduino bordel !!!");

        ui->label_daq->setText("DISCONNECTED");
    }
}

//--destructor--//
//--------------//
MainWindow::~MainWindow()
{
    if(arduino->isOpen())
        arduino->close();

    delete ui;
}

//--bouton start--//
//----------------//
void MainWindow::on_pushButton_clicked()
{
    //--open webcam--//
    //---------------//
    cap.open(0);

    //--access joystick--//
    //-------------------//
    Joystick joy(path);

    //--define tracker algo--//
    //-----------------------//
    string tracker_algorithm = "KCF";
    Ptr<Tracker> tracker = Tracker::create(tracker_algorithm);

    //--read 1st frame--//
    //------------------//
    cap >> image;
    image.copyTo(frame);

    while(true)
    {
        //--display video--//
        //-----------------//
        cap >> image;
        image.copyTo(frame);

        //--define zone Lock and Track--//
        //------------------------------//
        Rect2d zoneLOCK = Rect2d(frame.cols/2-30, frame.rows/2-30, 60,60);
        Rect2d zoneTRACK = Rect2d(frame.cols/2-120, frame.rows/2-120, 240, 240);

        //--entering Lock Mode--//
        //----------------------//
        if(centroid.inside(zoneLOCK))
        {
            Square(frame, Point(frame.cols/2, frame.rows/2), rouge, 60, 3, 8);
            circle(frame, center, 1, rouge, 2);
            line(frame, center, centroid, bleu, 2);
        }

        //--entering Track Mode--//
        //-----------------------//
        if(centroid.inside(zoneTRACK))
        {
            Square(frame, Point(frame.cols/2, frame.rows/2), rouge, 240, 1, 8);
            circle(frame, center, 1, rouge, 2);
            line(frame, center, centroid, blanc, 2);
        }

        //--get joystick axis--//
        //---------------------//
        x = joy.getAxis(0);
        y = joy.getAxis(1);

        //--get joystick button--//
        //-----------------------//
        select = joy.getButton(0);
        release = joy.getButton(1);

        //--scaling joystick to screen--//
        //------------------------------//
        scaleX=((x+value)*frame.cols)/sigma;
        scaleY=((y+value)*frame.rows)/sigma;

        //--plot joystick coordinates--//
        //-----------------------------//
        drawMarker(frame, Point(scaleX, scaleY), vert,MARKER_CROSS, 20, 2);

        //--calculate joystick center point--//
        //-----------------------------------//
        centroid = Point(boundingBox.x + (boundingBox.width/2), boundingBox.y + (boundingBox.height/2));

        //--start acq. target--//
        //---------------------//
        if(select)
        {
            boundingBox = Rect2d(scaleX-30, scaleY-30,60,60);
            tracker->init(frame, boundingBox);
        }

        //--track target--//
        //----------------//
        tracker->update(frame, boundingBox);
        rectangle(frame, boundingBox, rouge,2);
        circle(frame, centroid, 2, rouge,2);

        //--release tracker--//
        //-------------------//
        if(release)
        {
            //todo: creer cancel tracker joystick
        }

        //--reticule--//
        //------------//
        if(reticule)
        {
            Square(frame, Point(frame.cols/2, frame.rows/2), couleur, taille, 1, 8);

            center = Point(frame.cols/2, frame.rows/2);
            circle(frame, center, 1, couleur, 2);

            pt1 = Point(0, frame.rows/2);
            pt2 = Point(frame.cols/2 - offset, frame.rows/2);
            line(frame, pt1, pt2, couleur, 1);

            pt3 = Point(frame.cols/2 + offset, frame.rows/2);
            pt4 = Point(frame.cols, frame.rows/2);
            line(frame, pt3, pt4, couleur, 1);

            pt5 = Point(frame.cols/2, 0);
            pt6 = Point(frame.cols/2, frame.rows/2 - offset);
            line(frame, pt5, pt6, couleur, 1);

            pt7 = Point(frame.cols/2, frame.rows/2 + offset);
            pt8 = Point(frame.cols/2, frame.rows);
            line(frame, pt7, pt8, couleur, 1);
        }

        //--draw black mask around window--//
        //---------------------------------//
        rectangle(frame, Point(0,0), Point(frame.cols, epais), noir, -1);

        //--print infos LOCK on screen--//
        //------------------------------//
        putText(frame, "M.LOCK: ", Point(500,25),1,1,blanc,1);

        if(centroid.inside(zoneLOCK))
            putText(frame, "ON",Point(570,25),1,1,blanc,2);
        else
            putText(frame, "OFF",Point(570,25),1,1,blanc,1);

        //--print infos TRACK on screen--//
        //-------------------------------//
        putText(frame, "M.TRACK: ", Point(350,25),1,1,blanc,1);

        if(centroid.inside(zoneTRACK))
            putText(frame, "ON",Point(430,25),1,1, blanc,2);
        else
            putText(frame, "OFF",Point(430,25),1,1, blanc,1);

        //--masque bas--//
        //--------------//
        rectangle(frame, Point(0, frame.rows), Point(frame.cols, frame.rows-epais-5), noir, -1);

        //--echelle vitesse--//
        //-------------------//
        for(int i=0; i<frame.cols; i+=delta)
        {
            line(frame, Point(i, frame.rows), Point(i, frame.rows-scale), blanc,1);

            //--int2str--//
            //-----------//
            stringstream v;
            v << i/5;

            putText(frame, v.str(), Point(i-10, frame.rows-scale-5), 1, 1, blanc, 1);
        }

        //--parse data gps--//
        //------------------//
        speed = parseData1.toInt();

        //--speed scale--//
        //---------------//
        speedScale = (speed *5);

        //--cuseur vitesse masque bas--//
        //-----------------------------//
        TriangleDown(frame, Point(speedScale, frame.rows - offset-20), blanc, 20, 2, 8);

        //--connexion gps--//
        //-----------------//
        if(parseData1==0)
            ui->label_gps->setText("DISCONNECTED");

        //--lecture frame--//
        //-----------------//
        Frame();

        //--pause ms frame--//
        //------------------//
        waitKey(32);
    }
}

//--bouton stop--//
//---------------//
void MainWindow::on_pushButton_2_clicked()
{
    destroyAllWindows();
    close();
    exit(0);
}

//--lecture video--//
//-----------------//
void MainWindow::Frame()
{
    cv::resize(frame, frame, Size(ui->label->width(), ui->label->height()));
    cvtColor(frame, frame,CV_BGR2RGB);

    QImage imgFrame= QImage((uchar*) frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    QPixmap pixFrame = QPixmap::fromImage(imgFrame);

    ui->label->setPixmap(pixFrame);
}

//--lecture port serie--//
//----------------------//
void MainWindow::readSerial()
{
    QStringList buffer_split = serialBuffer.split(",");

    if(buffer_split.length() < 2) //1 data
    {
        serialData = arduino->readAll();
        serialBuffer += QString::fromStdString(serialData.toStdString());
        serialData.clear();
    }
    else
    {
        //--parse data--//
        //--------------//
        serialBuffer = "";
        qDebug() << buffer_split << endl;

        //--connexion gps--//
        //-----------------//
        if(serialBuffer.isEmpty())
            ui->label_gps->setText("CONNECTED");

        if(parseData1 != '$')
        {
            parseData1 = buffer_split[0];
        }

        //--affiche debugg output--//
        //-------------------------//
        qDebug() << parseData1 << endl;

        float a = parseData1.toFloat();

        qDebug() << a << endl;
    }
}

//--affichage reticule--//
//----------------------//
void MainWindow::on_checkBox_reticule_clicked(bool checked)
{
    if(checked==true)
        reticule=true;
    else
        reticule=false;
}

//--gestion couleurs--//
//--------------------//
void MainWindow::on_radioButton_blanc_clicked()
{
    couleur = blanc;
}

void MainWindow::on_radioButton_noir_clicked()
{
    couleur = noir;
}

void MainWindow::on_radioButton_rouge_clicked()
{
    couleur = rouge;
}

void MainWindow::on_radioButton_vert_clicked()
{
    couleur = vert;
}

void MainWindow::on_radioButton_bleu_clicked()
{
    couleur = bleu;
}

//--centre du reticule--//
//----------------------//
void MainWindow::Square(Mat& img, Point position, const Scalar& color, int markerSize, int thickness, int line_type)
{
    line(img, Point(position.x-(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y-(markerSize/2)), color, thickness, line_type);
    line(img, Point(position.x+(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y+(markerSize/2)), color, thickness, line_type);
    line(img, Point(position.x+(markerSize/2), position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y+(markerSize/2)), color, thickness, line_type);
    line(img, Point(position.x-(markerSize/2), position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y-(markerSize/2)), color, thickness, line_type);
}

//--curseur vitesse--//
//-------------------//
void MainWindow::TriangleDown(Mat& img, Point position, const Scalar& color, int markerSize, int thickness, int line_type)
{
    line(img, Point(position.x+(markerSize/2), position.y), Point(position.x, position.y+(markerSize/2)), color, thickness, line_type);
    line(img, Point(position.x, position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y), color, thickness, line_type);
    line(img, Point(position.x-(markerSize/2), position.y), Point(position.x+(markerSize/2), position.y), color, thickness, line_type);
}
