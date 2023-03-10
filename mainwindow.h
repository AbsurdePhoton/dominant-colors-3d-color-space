/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.3 - 2023/03/10
#
#-------------------------------------------------*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/ximgproc.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/core/utility.hpp"
#include <QMainWindow>
#include <QFileDialog>
#include <QElapsedTimer>

#include "openglwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QElapsedTimer timer;

    int saveXOpenGL, saveYOpenGL, saveWidthOpenGL, saveHeightOpenGL; // for fullscreen and widget resizing
    int saveXButtonSave, saveYButtonSave; // for fullscreen, save button position


public slots:
    void ShowTimer(const bool start); // time elapsed


private slots:

    //// quit
    void on_button_quit_clicked(); // quit GUI
    void on_button_whats_this_clicked(); // What's this function

    //// load & save
    void ChangeBaseDir(QString filename); // set base dir and file
    void SaveDirBaseFile(); // just to keep the last open dir

    //// GUI
    void on_button_load_image_clicked(); // load image to analyze
    void on_button_load_lut_clicked(); // load .cube LUT to analyze
    void on_button_compute_clicked(); // compute dominant colors
    void on_button_save_clicked(); // save results
    void on_button_3d_reset_clicked(); // // recenter position & zoom for 3D scene for each color space
    void on_comboBox_color_space_currentIndexChanged(int index); // change color space
    void on_comboBox_sort_currentIndexChanged(int index); // sort palette
    void on_checkBox_3d_light_clicked(); // light on/off in 3D scene
    void on_checkBox_3d_fullscreen_clicked(); // view 3D scene fullscreen, <ESC> to return from it
    void on_button_3d_exit_fullscreen_clicked(); // exit fullscreen view of 3d scene
    void on_button_save_3d_clicked(); // save current view of 3D color space
    void on_button_3d_reset_flags_clicked(); // reset visibility and selection flags in 3D view
    void on_button_values_clicked(); // show converted color in several color spaces


private:
    Ui::MainWindow *ui;

    //// UI
    void InitializeValues(); // initialize GUI and variables

    //// Display
    void ShowImages(); // display thumbnail, quantized image, palette, and update 3D view

    //// Mouse & Keyboard
    void mousePressEvent(QMouseEvent *eventPress); // mouse clic events
    void keyPressEvent(QKeyEvent *keyEvent); // keyboard events

    //// General
    void Compute(); // compute dominant colors
    void SortPalettes(); // sort palettes
    QString ConvertColor(const double &R, const double &G, const double &B); // convert a RGB color to other color spaces

    //// Variables

    std::string basefile, basedir, basedirinifile; // main image filename: directory and filename without extension
    bool loaded, computed; // indicators: image loaded or computed
    cv::Mat image, // main image
            thumbnail, // thumbnail of main image
            quantized, // quantized image
            //classification, // classification image
            palette; // palette image
    Qt::MouseButton mouseButton; // mouse button value
    QPoint mouse_pos; // mouse position
    const int palette_width = 1025; // palette image dimensions
    const int palette_height = palette_width / 5;
    QString converted; // for color values displayed in a QDialog

    struct struct_color_names { // for finding a color name
        int R;
        int G;
        int B;
        QString name;
    };
    struct_color_names color_names[10000]; // csv file contains more than 9000 values
    int nb_color_names; // color names number
};

#endif // MAINWINDOW_H
