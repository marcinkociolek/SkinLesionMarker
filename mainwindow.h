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



    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void ShowImages();
    void ShowsScaledImage(cv::Mat Im, std::string ImWindowName);
    void GetTile();
    void ShowTile();
    void ScaleTile();


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

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
