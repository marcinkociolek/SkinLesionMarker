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

const int miniatureScale = 12;

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
void FillHolesInMask(Mat Mask)
{
    FillBorderWithValue(Mask, 0xFFFF);
    OneRegionFill5Fast1(Mask, 0xFFFF);
    FillHoles(Mask, 3);
    DeleteRegionFromImage(Mask, 0xFFFF);
}

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
    prevPosX = -1;
    prevPosY = -1;
    ui->comboBoxShowMode->addItem("Bez Maski");
    ui->comboBoxShowMode->addItem("Maska");
    ui->comboBoxShowMode->addItem("Kontur");
    ui->comboBoxShowMode->addItem("Maska przeźroczysta");
    ui->comboBoxShowMode->addItem("Kontur przeźroczysty");
    ui->comboBoxShowMode->setCurrentIndex(3);

    ui->comboBoxDrawingTool->addItem("Brak");
    ui->comboBoxDrawingTool->addItem("Punkt");
    ui->comboBoxDrawingTool->addItem("Linia");
    //ui->comboBoxDrawingTool->addItem("Gumka");
    ui->comboBoxDrawingTool->setCurrentIndex(2);

    int tileStep = ui->spinBoxTileStep->value();

    ui->spinTilePositionX->setSingleStep(tileStep);
    ui->spinTilePositionY->setSingleStep(tileStep);

    prevTilePositionX = -1;
    prevTilePositionY = -1;

    allowMoveTile = 1;

    //ui->widgetImage->setFocus();


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
void MainWindow::OpenImagesFolder()
{
    ui->listWidgetImageFiles->clear();

    path FolderPath(ui->lineEditImageFolder->text().toStdWString());

    for (directory_entry& FileToProcess : directory_iterator(FolderPath))
    {
        wregex FilePattern(ui->lineEditRegexImageFile->text().toStdWString());
        if (!regex_match(FileToProcess.path().filename().wstring(), FilePattern ))
            continue;
        path PathLocal = FileToProcess.path();
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdWString(PathLocal.filename().wstring()) + " File not exists" );
            break;
        }
        ui->listWidgetImageFiles->addItem(QString::fromStdWString(PathLocal.filename().wstring()));
    }
    if(ui->listWidgetImageFiles->count())
        ui->listWidgetImageFiles->setCurrentRow(0);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowImages()
{
    if(ui->checkBoxShowInput->checkState())
        ShowsScaledImage(ImIn, "Input Image");
    if(ui->checkBoxShowMask->checkState())
        ShowsScaledImage(ShowRegion(changeRegionNumber(Mask,3,6)), "Mask");
        //ShowsScaledImage(ShowRegion(Mask), "Mask");
    if(ui->checkBoxShowMaskOnImage->checkState())
        //ShowsScaledImage(ShowSolidRegionOnImage(Mask, ImIn), "Mask on image");
    {
        //int tileStep = ui->spinBoxTileStep->value();
        int tileSize = ui->spinBoxTileSize->value();

        int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
        int tilePositionY = ui->spinTilePositionY->value();// * tileStep;

        Mat ImToShow;
        ShowSolidRegionOnImage(Mask, ImIn).copyTo(ImToShow);
        rectangle(ImToShow, Rect(tilePositionX,tilePositionY, tileSize, tileSize), Scalar(0.0, 255.0, 0.0, 0.0), 4);
        //ShowsScaledImage(ImToShow, "Mask on image");
        ShowsPartialScaledImage(ImToShow, "Mask on image");
    }

    int tileSize = ui->spinBoxTileSize->value();

    int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
    int tilePositionY = ui->spinTilePositionY->value();// * tileStep;


    if(ui->checkBoxShowTileOnImage->checkState())
    {
        //int tileStep = ui->spinBoxTileStep->value();
        //int tileSize = ui->spinBoxTileSize->value();

        //int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
        //int tilePositionY = ui->spinTilePositionY->value();// * tileStep;

        Mat ImToShow;
        ImIn.copyTo(ImToShow);
        rectangle(ImToShow, Rect(tilePositionX,tilePositionY, tileSize, tileSize), Scalar(0.0, 255.0, 0.0, 0.0), 4);
        ShowsScaledImage(ImToShow, "Tile On Image");

    }
    Mat ImToShow;

    ImIn.copyTo(ImToShow);
    rectangle(ImToShow, Rect(tilePositionX,tilePositionY, tileSize, tileSize), Scalar(0.0, 255.0, 0.0, 0.0),
              ui->spinBoxScaleBaseWhole->value());
    ui->widgetImageWhole->paintBitmap(ImToShow);
    ui->widgetImageWhole->repaint();
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

    double displayScale = 1.0 / double(ui->spinBoxScaleBase->value());
    if (displayScale != 1.0)
        cv::resize(ImToShow,ImToShow,Size(), displayScale, displayScale, INTER_AREA);
    imshow(ImWindowName, ImToShow);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowsPartialScaledImage(Mat Im, string ImWindowName)
{
    if(Im.empty())
    {
        ui->textEditOut->append("Empty Image to show");
        return;
    }

    int partCount = ui->spinBoxImPartition->value();
    double displayScale = double(partCount) / double(ui->spinBoxScaleBase->value());

    int tileSize = ui->spinBoxTileSize->value();

    int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
    int tilePositionY = ui->spinTilePositionY->value();// * tileStep;


    int partSizeX = ImIn.cols / partCount;
    int partSizeY = ImIn.rows / partCount;


    int partPositionX = (tilePositionX / partSizeX) * partSizeX;
    int partPositionY = (tilePositionY / partSizeY) * partSizeY;

    if(partCount >1)
    {
        partSizeX += tileSize * 2;

        if(partPositionX)
        {
            if(partPositionX > Im.cols - partSizeX)
                partPositionX = Im.cols - partSizeX;
            else
                partPositionX -= tileSize;
        }


        partSizeY += tileSize * 2;
        if(partPositionY)
        {
            if(partPositionY > Im.rows - partSizeY)
                partPositionY = Im.rows - partSizeY;
            else
                partPositionY -= tileSize;
        }

    }

    Mat ImToShow;
    Im(Rect(partPositionX, partPositionY, partSizeX, partSizeY)).copyTo(ImToShow);

    if (displayScale != 1.0)
        cv::resize(ImToShow,ImToShow,Size(), displayScale, displayScale, INTER_AREA);
    imshow(ImWindowName, ImToShow);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::GetTile()
{
    if(Mask.empty())
        return;
    if(ImIn.empty())
        return;
    //int tileStep = ui->spinBoxTileStep->value();
    int tileSize = ui->spinBoxTileSize->value();

    int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
    int tilePositionY = ui->spinTilePositionY->value();// * tileStep;

    prevTilePositionX = tilePositionX;
    prevTilePositionY = tilePositionY;

    ImIn(Rect(tilePositionX, tilePositionY, tileSize, tileSize)).copyTo(TileIm);
    Mask(Rect(tilePositionX, tilePositionY, tileSize, tileSize)).copyTo(TileMask);

    ShowTile();
    ShowImages();
}
//------------------------------------------------------------------------------------------------------------------------------
int MainWindow::CopyTileToRegion()
{
    if(prevTilePositionX < 0 || prevTilePositionY < 0)
        return 0;

    if(TileMask.empty())
        return -1;
    int tileStep = ui->spinBoxTileStep->value();
    int tileSize = ui->spinBoxTileSize->value();

    //int tilePositionX = ui->spinTilePositionX->value() * tileStep;
    //int tilePositionY = ui->spinTilePositionY->value() * tileStep;

    TileMask.copyTo(Mask(Rect(prevTilePositionX, prevTilePositionY, tileSize, tileSize)));
    return 1;
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowTile()
{
    Mat ImShow;
    if(TileMask.empty() || TileIm.empty())
        return;
    //TileIm.copyTo(ImShow);

    switch(ui->comboBoxShowMode->currentIndex())
    {
    case 1:
        ImShow = ShowSolidRegionOnImage(TileMask, TileIm);
        break;
    case 2:
        ImShow = ShowSolidRegionOnImage(GetContour5(TileMask), TileIm);
        break;
    case 3:
        ImShow = ShowTransparentRegionOnImage(TileMask, TileIm, ui->spinBoxTransparency->value());
        break;
    case 4:
        ImShow = ShowTransparentRegionOnImage(GetContour5(TileMask), TileIm, ui->spinBoxTransparency->value());
        break;
    default:
         TileIm.copyTo(ImShow);
        break;
    }



    ui->widgetImage->paintBitmap(ImShow);
    ui->widgetImage->repaint();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ScaleTile()
{
    int scaledSize = ui->spinBoxTileSize->value() * ui->spinBoxTileScale->value();
    int positionX = 530;
    int positionY = 40;
    ui->widgetImage->setGeometry(positionX,positionY,scaledSize,scaledSize);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ScaleImMiniature()
{
    if(ImIn.empty())
        return;
    int scale = ui->spinBoxScaleBaseWhole->value();
    int scaledSizeX = ImIn.cols/scale;
    int scaledSizeY = ImIn.rows/scale;
    int positionX = 0;
    int positionY = 550;
    ui->widgetImageWhole->setGeometry(positionX,positionY,scaledSizeX,scaledSizeY);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::LoadMask()
{
    QString ImageFolderQStr = ui->lineEditImageFolder->text();
    path maskFilePath = ImageFolderQStr.toStdWString();
    maskFilePath.append(imageFilePath.stem().string() + ".png");

    if(!exists(imageFilePath))
    {
        ui->textEditOut->append("image file " + QString::fromStdWString(imageFilePath.wstring()) + " not exists");
        return;
    }


    if(exists(maskFilePath))
    {
        Mask = imread(maskFilePath.string(), CV_LOAD_IMAGE_ANYDEPTH);
        if(Mask.type() != CV_16U)
        {
            ui->textEditOut->append("mask format improper");
            Mask.convertTo(Mask, CV_16U);
        }

        if(Mask.empty())
        {
            Mask = Mat::zeros(ImIn.rows, ImIn.cols, CV_16U);
            ui->textEditOut->append("mask file " + QString::fromStdWString(maskFilePath.wstring()) + "cannot be read");
            ui->textEditOut->append("empty mask was created");
        }
    }
    else
    {
        Mask = Mat::zeros(ImIn.rows, ImIn.cols, CV_16U);
        ui->textEditOut->append("mask file " + QString::fromStdWString(maskFilePath.wstring()) + " not exists");
        ui->textEditOut->append("empty mask was created");
    }
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::SaveMask()
{
    QString ImageFolderQStr = ui->lineEditImageFolder->text();
    path maskFilePath = ImageFolderQStr.toStdWString();
    maskFilePath.append(imageFilePath.stem().string() + ".png");

    if(!exists(imageFilePath))
    {
        ui->textEditOut->append("image file " + QString::fromStdWString(imageFilePath.wstring()) + " not exists");
        return;
    }

    if(Mask.empty())
    {
        ui->textEditOut->append("mask empty. nothing to save");
        return;
    }

    if(!exists(maskFilePath))
    {
        ui->textEditOut->append("new mask saved");
    }
    else
    {
        ui->textEditOut->append("mask updated");
    }

    imwrite(maskFilePath.string(),Mask);
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::SaveImages()
{
    if(ImIn.empty())
        return;
    if(Mask.empty())
        return;

    if(TileMask.empty() || TileIm.empty())
        return;


    QString ImageFolderQStr = ui->lineEditImageFolder->text();
    path largeImageFilePath = ImageFolderQStr.toStdWString();
    largeImageFilePath.append(imageFilePath.stem().string() + "_" + ItoStrLZ(ui->spinBoxSaveImNr->value(), 2) + "L.jpg");


    path miniImageFilePath = ImageFolderQStr.toStdWString();
    miniImageFilePath.append(imageFilePath.stem().string() + "_" + ItoStrLZ(ui->spinBoxSaveImNr->value(), 2) + "M.jpg");

    int tileSize = ui->spinBoxTileSize->value();

    int tilePositionX = ui->spinTilePositionX->value();// * tileStep;
    int tilePositionY = ui->spinTilePositionY->value();// * tileStep;

    Mat ImToSave;
    ShowSolidRegionOnImage(Mask, ImIn).copyTo(ImToSave);
    rectangle(ImToSave, Rect(tilePositionX,tilePositionY, tileSize, tileSize), Scalar(0.0, 255.0, 0.0, 0.0), 4);

    imwrite(largeImageFilePath.string(),ImToSave);



    Mat ImToSaveMini;

    ImToSaveMini = ShowSolidRegionOnImage(GetContour5(TileMask), TileIm);
    imwrite(miniImageFilePath.string(),ImToSaveMini);
    ui->spinBoxSaveImNr->setValue(ui->spinBoxSaveImNr->value() + 1);
}

//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          Slots
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_pushButtonOpenImageFolder_clicked()
{
    QFileDialog dialog(this, "Open Folder");
    //dialog.setFileMode(QFileDialog::Directory);

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

    OpenImagesFolder();
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_listWidgetImageFiles_currentTextChanged(const QString &currentText)
{
    QString ImageFolderQStr = ui->lineEditImageFolder->text();
    imageFilePath = ImageFolderQStr.toStdWString();
    imageFilePath.append(currentText.toStdWString());

    if(!exists(imageFilePath))
    {
        ui->textEditOut->append("file " + QString::fromStdWString(imageFilePath.wstring()) + "not exists");
        return;
    }

    string fileToOpenStr = imageFilePath.string();

    if(ui->checkBoxAutocleanOut->checkState())
        ui->textEditOut->clear();
    int flags = IMREAD_COLOR;;

    ImIn = imread(fileToOpenStr, flags);

    if(ImIn.empty())
    {
        ui->textEditOut->append("improper file");
        return;
    }

    LoadMask();

    string extension = imageFilePath.extension().string();

    if((extension == ".tif" || extension == ".tiff") && ui->checkBoxShowTiffInfo->checkState())
        ui->textEditOut->append(QString::fromStdString(TiffFilePropetiesAsText(fileToOpenStr)));

    if(ui->checkBoxShowMatInfo->checkState())
        ui->textEditOut->append(QString::fromStdString(MatPropetiesAsText(ImIn)));

    //int tileStep = ui->spinBoxTileStep->value();
    int tileSize = ui->spinBoxTileSize->value();
    int imMaxX = ImIn.cols;
    int imMaxY = ImIn.rows;

    //ui->spinTilePositionX->setMaximum((imMaxX-tileSize - 1)/tileStep);
    //ui->spinTilePositionY->setMaximum((imMaxY-tileSize - 1)/tileStep);
    ui->spinTilePositionX->setMaximum(imMaxX-tileSize - 1);
    ui->spinTilePositionY->setMaximum(imMaxY-tileSize - 1);
    ScaleImMiniature();
    ShowImages();
    GetTile();
    ui->spinBoxSaveImNr->setValue(1);

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_checkBoxShowInput_toggled(bool checked)
{
    if(checked)
        ShowImages();
    else
        destroyWindow("Input Image");
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinBoxScalePower_valueChanged(int arg1)
{
    if(arg1 == 1)
        ui->spinBoxScaleBase->setValue(1);
    else
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
    if(!allowMoveTile)
        return;
    FillHolesInMask(TileMask);
    CopyTileToRegion();
    GetTile();

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_spinTilePositionY_valueChanged(int arg1)
{
    if(!allowMoveTile)
        return;
    FillHolesInMask(TileMask);
    CopyTileToRegion();
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
    int tileSize = arg1;
    int imMaxX = ImIn.cols;
    int imMaxY = ImIn.rows;

    ui->spinTilePositionX->setMaximum(imMaxX-tileSize - 1);
    ui->spinTilePositionY->setMaximum(imMaxY-tileSize - 1);

    FillHolesInMask(TileMask);
    GetTile();
    ScaleTile();
}

void MainWindow::on_widgetImage_on_mouseMove(QPoint point, int butPressed)
{
    int radius = ui->spinBoxPenSize->value();
    int tileScale = ui->spinBoxTileScale->value();
    int x = point.x() / tileScale;
    int y = point.y() / tileScale;
    if(butPressed & 0x1)
    {
        switch(ui->comboBoxDrawingTool->currentIndex())
        {
        case 1:
            circle(TileMask, Point(x,y),radius - 1,3,-1);
            break;
        case 2:
            circle(TileMask, Point(x,y),radius - 1,3,-1);
            if(prevPosX < 0 || prevPosX < 0)
            {
                prevPosX = x;
                prevPosY = y;
            }
            else
            {
                line(TileMask, Point(prevPosX,prevPosY), Point(x,y), 3, radius);
                prevPosX = x;
                prevPosY = y;
            }
            break;
        }
    }
    else
    {
        prevPosX = -1;
        prevPosY = -1;
    }
    if(butPressed & 0x2)
    {
        circle(TileMask, Point(x,y),radius - 1,0,-1);
    }
    ShowTile();
}
void MainWindow::on_widgetImage_on_mousePressed(QPoint point, int butPressed)
{
    int radius = ui->spinBoxPenSize->value();
    int tileScale = ui->spinBoxTileScale->value();
    int x = point.x() / tileScale;
    int y = point.y() / tileScale;
    if(butPressed & 0x1)
    {
        prevPosX = x;
        prevPosY = y;
        circle(TileMask, Point(x,y),radius - 1,3,-1);
    }
    if(butPressed & 0x2)
    {
        circle(TileMask, Point(x,y),radius - 1,0,-1);
    }
    if(butPressed & 0x4)
    {
        ui->spinBoxTransparency->setValue(0);
    }
    if(butPressed & 0x8)
    {
        ui->spinBoxTransparency->setValue(30);
    }
    ShowTile();
}

void MainWindow::on_comboBoxShowMode_currentIndexChanged(int index)
{
    ShowTile();
}

void MainWindow::on_spinBoxTransparency_valueChanged(int arg1)
{
    ShowTile();
}

void MainWindow::on_pushButtonFillHoles_clicked()
{
    FillHolesInMask(TileMask);
    //FillBorderWithValue(TileMask, 0xFFFF);
    //OneRegionFill5Fast1(TileMask, 0xFFFF);
    //FillHoles(TileMask, 3);
    //DeleteRegionFromImage(TileMask, 0xFFFF);
    ShowTile();
}

void MainWindow::on_pushButtonClearRegion_clicked()
{
    DeleteRegionFromImage(TileMask, 3);
    ShowTile();
}

void MainWindow::on_pushButtonReloadTile_clicked()
{
    GetTile();
}

void MainWindow::on_pushButtonCopyToMask_clicked()
{
    FillHolesInMask(TileMask);
    CopyTileToRegion();
    ShowImages();
}

void MainWindow::on_checkBoxShowMask_toggled(bool checked)
{
    if(checked)
        ShowImages();
    else
        destroyWindow("Mask");;
}

void MainWindow::on_pushButtonSaveMask_clicked()
{
    FillHolesInMask(TileMask);
    CopyTileToRegion();
    SaveMask();
}

void MainWindow::on_pushButtonReloadMask_clicked()
{
    LoadMask();
    ShowImages();
    GetTile();
}

void MainWindow::on_lineEditRegexImageFile_returnPressed()
{
    OpenImagesFolder();
}

void MainWindow::on_lineEditImageFolder_returnPressed()
{
    OpenImagesFolder();
}

void MainWindow::on_checkBoxShowMaskOnImage_toggled(bool checked)
{
    if(checked)
        ShowImages();
    else
        destroyWindow("Mask on image");
}

void MainWindow::on_spinBoxTileStep_valueChanged(int arg1)
{
    int tileStep = ui->spinBoxTileStep->value();

    ui->spinTilePositionX->setSingleStep(tileStep);
    ui->spinTilePositionY->setSingleStep(tileStep);
}

void MainWindow::on_checkBoxShowTileOnImage_toggled(bool checked)
{
    if(checked)
        ShowImages();
    else
        destroyWindow("Tile On Image");
}

void MainWindow::on_widgetImageWhole_on_mouseMove(QPoint point, int butPressed )
{

}

void MainWindow::on_widgetImageWhole_on_mousePressed(QPoint point, int butPressed )
{
    int tileSize = ui->spinBoxTileSize->value();
    int tileHalfSize = tileSize /2;
    int tileStep = ui->spinBoxTileStep->value();
    int maxXadjusted = ImIn.cols - tileSize - 1;
    int maxYadjusted = ImIn.cols - tileSize - 1;
    int miniatureScale = ui->spinBoxScaleBaseWhole->value();
    int x = point.x() * miniatureScale - tileHalfSize;//((point.x() * miniatureScale - tileHalfSize) / tileStep) * tileStep;
    int y = point.y() * miniatureScale - tileHalfSize;//((point.y() * miniatureScale - tileHalfSize) / tileStep) * tileStep;
    if(x < 0)
        x = 0;
    if(x > maxXadjusted)
        x = maxXadjusted;
    if(y < 0)
        y = 0;
    if(y > maxYadjusted)
        y = maxYadjusted;
    allowMoveTile = 0;
    ui->spinTilePositionX->setValue(x);
    ui->spinTilePositionY->setValue(y);
    allowMoveTile = 1;
    FillHolesInMask(TileMask);
    CopyTileToRegion();
    GetTile();

}



void MainWindow::on_spinBoxScaleBaseWhole_valueChanged(int arg1)
{
    ScaleImMiniature();
    ShowImages();
}

void MainWindow::on_widgetImage_on_KeyPressed(int button)
{
    switch (button)
    {
    case Qt::Key_Z:
        ui->spinBoxTransparency->setValue(0);
        break;
    case Qt::Key_X:
        ui->spinBoxTransparency->setValue(16);
        break;
    case Qt::Key_Up:
        ui->spinTilePositionY->setValue(ui->spinTilePositionY->value() - ui->spinTilePositionY->singleStep());
        break;
    case Qt::Key_Down:
        ui->spinTilePositionY->setValue(ui->spinTilePositionY->value() + ui->spinTilePositionY->singleStep());
        break;
    case Qt::Key_Left:
        ui->spinTilePositionX->setValue(ui->spinTilePositionX->value() - ui->spinTilePositionX->singleStep());
        break;
    case Qt::Key_Right:
        ui->spinTilePositionX->setValue(ui->spinTilePositionX->value() + ui->spinTilePositionX->singleStep());
        break;
    case Qt::Key_Space:
        FillHolesInMask(TileMask);
        break;
    case Qt::Key_Period:
        ui->spinBoxTileSize->setValue(ui->spinBoxTileSize->value() + ui->spinBoxTileSize->singleStep());
        break;
    case Qt::Key_Comma:
        ui->spinBoxTileSize->setValue(ui->spinBoxTileSize->value() - ui->spinBoxTileSize->singleStep());
        break;

    default:
        break;
    }
}

void MainWindow::on_checkBoxGrabKeyboard_toggled(bool checked)
{
    if(checked)
        ui->widgetImage->grabKeyboard();
    else
        ui->widgetImage->releaseKeyboard();
}

void MainWindow::on_pushButtonSaveOut_clicked()
{
    SaveImages();
}

void MainWindow::on_spinBoxImPartition_valueChanged(int arg1)
{
     ShowImages();
}
