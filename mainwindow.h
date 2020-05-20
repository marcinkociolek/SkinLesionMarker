#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <boost/filesystem.hpp>
#include <opencv2/core/core.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    cv::Mat ImIn;
    cv::Mat Mask;

    cv::Mat TileIm;
    cv::Mat TileMask;

    int prevPosX;
    int prevPosY;

    int prevTilePositionX;
    int prevTilePositionY;

    boost::filesystem::path imageFilePath;

    bool allowMoveTile;


    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void MainWindow::OpenImagesFolder();
    void ShowImages();
    void ShowsScaledImage(cv::Mat Im, std::string ImWindowName);
    void GetTile();
    int CopyTileToRegion();
    void ShowTile();
    void ScaleTile();
    void ScaleImMiniature();
    void LoadMask();
    void SaveMask();


private slots:
    void on_pushButtonOpenImageFolder_clicked();

    void on_listWidgetImageFiles_currentTextChanged(const QString &currentText);

    void on_checkBoxShowInput_toggled(bool checked);

    void on_spinBoxScalePower_valueChanged(int arg1);

    void on_spinBoxScaleBase_valueChanged(int arg1);

    void on_spinTilePositionX_valueChanged(int arg1);

    void on_spinTilePositionY_valueChanged(int arg1);

    void on_spinBoxTileScale_valueChanged(int arg1);

    void on_spinBoxTileSize_valueChanged(int arg1);



    void on_widgetImage_on_mouseMove(QPoint point, int butPressed );
    void on_widgetImage_on_mousePressed(QPoint point, int butPressed);

    void on_comboBoxShowMode_currentIndexChanged(int index);

    void on_spinBoxTransparency_valueChanged(int arg1);

    void on_pushButtonFillHoles_clicked();

    void on_pushButtonClearRegion_clicked();

    void on_pushButtonReloadTile_clicked();

    void on_pushButtonCopyToMask_clicked();

    void on_checkBoxShowMask_toggled(bool checked);

    void on_pushButtonSaveMask_clicked();

    void on_pushButtonReloadMask_clicked();

    void on_lineEditRegexImageFile_returnPressed();

    void on_lineEditImageFolder_returnPressed();

    void on_checkBoxShowMaskOnImage_toggled(bool checked);

    void on_spinBoxTileStep_valueChanged(int arg1);

    void on_checkBoxShowTileOnImage_toggled(bool checked);

    void on_widgetImageWhole_on_mouseMove(QPoint point, int butPressed );

    void on_widgetImageWhole_on_mousePressed(QPoint point, int butPressed  );



    void on_spinBoxScaleBaseWhole_valueChanged(int arg1);

    void on_widgetImage_on_KeyPressed(int );

    void on_checkBoxGrabKeyboard_toggled(bool checked);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
