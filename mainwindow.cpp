#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

#include <string>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>


#include <math.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "NormalizationLib.h"
#include "DispLib.h"
#include "histograms.h"
#include "gradient.h"
#include "RegionU16Lib.h"
#include "StringFcLib.h"

#include "mazdaroi.h"
#include "mazdaroiio.h"

#include <tiffio.h>

typedef MazdaRoi<unsigned int, 2> MR2DType;

using namespace std;
using namespace boost::filesystem;
using namespace boost;
using namespace cv;
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          Local Functions
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void PlaceShapeOnMask(Mat Mask, int x, int y, int size, unsigned short val)
{
    int maxX = Mask.cols;
    int maxY = Mask.rows;
    int maxXm1 = maxX-1;
    int maxYm1 = maxY-1;
    if(y >= 0 && y < maxY && x >= 0 && x < maxY)
        Mask.at<unsigned short>(y,x) = val;
    if(size > 1)
    {
        if(y>0)
            Mask.at<unsigned short>(y-1,x) = val;
        if(y < maxYm1)
            Mask.at<unsigned short>(y+1,x) = val;
        if(x>0)
            Mask.at<unsigned short>(y,x-1) = val;
        if(x < maxXm1)
            Mask.at<unsigned short>(y,x+1) = val;
    }
    if(size > 2)
    {
        if(y > 0 && x > 0)
            Mask.at<unsigned short>(y-1,x-1) = val;
        if(y > 0 && x < maxXm1)
            Mask.at<unsigned short>(y-1,x+1) = val;
        if(y < maxYm1 && x > 0)
            Mask.at<unsigned short>(y+1,x-1) = val;
        if(y < maxYm1 && x < maxXm1)
            Mask.at<unsigned short>(y+1,x+1) = val;
    }
}

//----------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          constructor Destructor
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ScaleTile();
}

MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          CLASS FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowImages()
{
    if(ui->checkBoxShowInput->checkState())
        ShowsScaledImage(ImIn, "Input Image");
    if(ui->checkBoxShowMask->checkState())
        ShowsScaledImage(ShowRegion(Mask), "Mask");
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowsScaledImage(Mat Im, string ImWindowName)
{
    if(Im.empty())
    {
        ui->textEditOut->append("Empty Image to show");
        return;
    }

    Mat ImToShow;

    ImToShow = Im.clone();

    double displayScale = pow(double(ui->spinBoxScaleBase->value()), double(ui->spinBoxScalePower->value()));
    if (displayScale != 1.0)
        cv::resize(ImToShow,ImToShow,Size(), displayScale, displayScale, INTER_AREA);
    if(ui->checkBoxImRotate->checkState())
        rotate(ImToShow,ImToShow, ROTATE_90_CLOCKWISE);
    imshow(ImWindowName, ImToShow);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::GetTile()
{
    int tileStep = ui->spinBoxTileStep->value();
    int tileSize = ui->spinBoxTileSize->value();

    int tilePositionX = ui->spinTilePositionX->value() * tileStep;
    int tilePositionY = ui->spinTilePositionY->value() * tileStep;


    if(ui->checkBoxShowTileOnImage->checkState())
    {

        Mat ImToShow;
        ImIn.copyTo(ImToShow);
        rectangle(ImToShow, Rect(tilePositionX,tilePositionY, tileSize, tileSize), Scalar(0.0, 255.0, 0.0, 0.0), 4);
        ShowsScaledImage(ImToShow, "Tile On Image");
    }
    ImIn(Rect(tilePositionX, tilePositionY, tileSize, tileSize)).copyTo(TileIm);
    Mask(Rect(tilePositionX, tilePositionY, tileSize, tileSize)).copyTo(TileMask);

    ShowTile();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowTile()
{
    Mat ImShow;
    //TileIm.copyTo(ImShow);
    ImShow = ShowSolidRegionOnImage(TileMask,TileIm);
    ui->widgetImage->paintBitmap(ImShow);
    ui->widgetImage->repaint();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ScaleTile()
{
    int scaledSize = ui->spinBoxTileSize->value() * ui->spinBoxTileScale->value();
    int positionX = 520;
    int positionY = 90;
    ui->widgetImage->setGeometry(positionX,positionY,scaledSize,scaledSize);
}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          Slots
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_pushButtonOpenImageFolder_clicked()
{
    QFileDialog dialog(this, "Open Folder");
    dialog.setFileMode(QFileDialog::Directory);

    QString FolderQString = ui->lineEditImageFolder->text();
    path FolderPath;

    dialog.setDirectory(FolderQString);


    if(dialog.exec())
    {
        FolderQString = dialog.directory().path();
        FolderPath = FolderQString.toStdWString();
        if(!exists(FolderPath))
        {
            ui->textEditOut->append("folder: "+ FolderQString + "does not exists");
            return;
        }
        if (!is_directory(FolderPath))
        {
            ui->textEditOut->append("folder: "+ FolderQString + "is not a directory");
            return;
        }
    }
    else
        return;

    ui->lineEditImageFolder->setText(FolderQString);

    ui->listWidgetImageFiles->clear();

    for (directory_entry& FileToProcess : directory_iterator(FolderPath))
    {
        regex FilePattern(ui->lineEditRegexImageFile->text().toStdString());
        if (!regex_match(FileToProcess.path().filename().string().c_str(), FilePattern ))
            continue;
        path PathLocal = FileToProcess.path();
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdString(PathLocal.filename().string() + " File not exists" ));
            break;
        }
        ui->listWidgetImageFiles->addItem(PathLocal.filename().string().c_str());
    }
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_listWidgetImageFiles_currentTextChanged(const QString &currentText)
{
    QString ImageFolderQStr = ui->lineEditImageFolder->text();
    path fileToOpenPath = ImageFolderQStr.toStdWString();
    fileToOpenPath.append(currentText.toStdWString());
    string fileToOpenStr = fileToOpenPath.string();

    if(ui->checkBoxAutocleanOut->checkState())
        ui->textEditOut->clear();
    int flags;
    if(ui->checkBoxLoadAnydepth->checkState())
        flags = CV_LOAD_IMAGE_ANYDEPTH;
    else
        flags = IMREAD_COLOR;

    ImIn = imread(fileToOpenStr, flags);

    if(ImIn.empty())
    {
        ui->textEditOut->append("improper file");
        return;
    }

    //-------------------------------------
    //          Mask
    //-------------------------------------
    Mask = Mat::zeros(ImIn.rows, ImIn.cols, CV_16U);

    //-------------------------------------
    //-------------------------------------

    string extension = fileToOpenPath.extension().string();

    if((extension == ".tif" || extension == ".tiff") && ui->checkBoxShowTiffInfo->checkState())
        ui->textEditOut->append(QString::fromStdString(TiffFilePropetiesAsText(fileToOpenStr)));

    if(ui->checkBoxShowMatInfo->checkState())
        ui->textEditOut->append(QString::fromStdString(MatPropetiesAsText(ImIn)));

    int tileStep = ui->spinBoxTileStep->value();
    int tileSize = ui->spinBoxTileSize->value();
    int imMaxX = ImIn.cols;
    int imMaxY = ImIn.rows;

    ui->spinTilePositionX->setMaximum((imMaxX-tileSize - 1)/tileStep);
    ui->spinTilePositionY->setMaximum((imMaxY-tileSize - 1)/tileStep);
    ShowImages();
    GetTile();

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_checkBoxShowInput_toggled(bool checked)
{
    ShowImages();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinBoxScalePower_valueChanged(int arg1)
{
    ShowImages();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinBoxScaleBase_valueChanged(int arg1)
{
    ShowImages();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinTilePositionX_valueChanged(int arg1)
{
    GetTile();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinTilePositionY_valueChanged(int arg1)
{
    GetTile();
}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------


void MainWindow::on_spinBoxTileScale_valueChanged(int arg1)
{
    ScaleTile();
}

void MainWindow::on_spinBoxTileSize_valueChanged(int arg1)
{
    GetTile();
    ScaleTile();
}

void MainWindow::on_widgetImage_on_mouseMove(QPoint point, int butPressed)
{
    int radius = ui->spinBoxPenSize->value();
    int tileScale = ui->spinBoxTileScale->value();
    int x = point.x() / tileScale;
    int y = point.y() / tileScale;
    if(butPressed == 1)
    {
        circle(TileMask, Point(x,y),radius,3,-1);

        //PlaceShapeOnMask(TileMask, x, y, 0, 2);
    }
    if(butPressed & 0x2)
    {
        circle(TileMask, Point(x,y),radius,0,-1);
        //PlaceShapeOnMask(TileMask, x, y, 0, 0);
    }
    ShowTile();
}
