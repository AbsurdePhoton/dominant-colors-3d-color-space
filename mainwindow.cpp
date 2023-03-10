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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSizeGrip>
#include <QGridLayout>
//#include <QDesktopWidget>
#include <QCursor>
#include <QMouseEvent>
#include <QWhatsThis>

#include <fstream>
#include <thread>
#include <omp.h>

#include "widgets/file-dialog.h"
#include "lib/dominant-colors.h"
#include "lib/image-transform.h"
#include "lib/image-color.h"
#include "lib/color-spaces.h"
#include "lib/image-utils.h"
#include "lib/image-lut.h"

using namespace cv;
using namespace cv::ximgproc;
using namespace std;

/////////////////// Window init //////////////////////

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // numeric dot vs comma
    std::setlocale(LC_NUMERIC,"C"); // dot as numeric separator for decimal values - some countries use commas

    // number of processors
    int processor_threads = std::thread::hardware_concurrency(); // find how many processor threads in the system
    omp_set_num_threads(processor_threads/* / 2*/ - 1); // set usable threads for OMP

    // window
    setWindowFlags((((windowFlags() | Qt::CustomizeWindowHint)
                            & ~Qt::WindowCloseButtonHint) | Qt::WindowMinMaxButtonsHint)); // don't show buttons in title bar
    this->setWindowState(Qt::WindowMaximized); // maximize window
    setFocusPolicy(Qt::StrongFocus); // catch keyboard and mouse in priority
    statusBar()->setVisible(false); // no status bar

    // add size grip to openGL widget
    ui->openGLWidget_3d->setWindowFlags(Qt::SubWindow);
    QSizeGrip * sizeGrip = new QSizeGrip(ui->openGLWidget_3d);
    QGridLayout * layout = new QGridLayout(ui->openGLWidget_3d);
    layout->addWidget(sizeGrip, 0,0,1,1,Qt::AlignBottom | Qt::AlignRight);

    // populate color space combobox
    ui->comboBox_color_space->blockSignals(true); // don't launch automatic update of 3D view
    ui->comboBox_color_space->addItem(tr("RGB"));
    ui->comboBox_color_space->addItem(tr("Wheel"));
    ui->comboBox_color_space->addItem(tr("HSV"));
    ui->comboBox_color_space->addItem(tr("HWB"));
    ui->comboBox_color_space->addItem(tr("CIE L*a*b*"));
    ui->comboBox_color_space->addItem(tr("CIE L*u*v*"));
    ui->comboBox_color_space->addItem(tr("OKLAB"));
    ui->comboBox_color_space->addItem(tr("----------"));
    ui->comboBox_color_space->addItem(tr("RGB Triangle"));
    ui->comboBox_color_space->addItem(tr("HCV"));
    ui->comboBox_color_space->addItem(tr("HSL"));
    ui->comboBox_color_space->addItem(tr("HCL"));
    ui->comboBox_color_space->addItem(tr("CIE XYZ"));
    ui->comboBox_color_space->addItem(tr("CIE xyY"));
    ui->comboBox_color_space->addItem(tr("LMS"));
    ui->comboBox_color_space->addItem(tr("Hunter Lab"));
    //ui->comboBox_color_space->addItem(tr("CIE LCHab")); // same as CIE L*a*b*
    //ui->comboBox_color_space->addItem(tr("CIE LCHuv")); // same as CIE L*u*v*
    ui->comboBox_color_space->blockSignals(false); // return to normal behavior

    // populate sort combobox
    ui->comboBox_sort->blockSignals(true); // don't launch automatic update of palette
    ui->comboBox_sort->addItem("Percentage");
    ui->comboBox_sort->addItem("Hue");
    ui->comboBox_sort->addItem("Chroma");
    ui->comboBox_sort->addItem("Saturation");
    ui->comboBox_sort->addItem("Value");
    ui->comboBox_sort->addItem("Lightness");
    ui->comboBox_sort->addItem("Luminance");
    ui->comboBox_sort->addItem("Distance");
    ui->comboBox_sort->addItem("Whiteness");
    ui->comboBox_sort->addItem("Blackness");
    ui->comboBox_sort->addItem("RGB");
    ui->comboBox_sort->addItem("Luma");
    ui->comboBox_sort->addItem("Rainbow6");
    ui->comboBox_sort->blockSignals(false); // return to normal behavior

    // fullscreen button
    ui->button_3d_exit_fullscreen->setVisible(false); // hide 3D fullscreen exit button

    // Timer
    ui->timer->setPalette(Qt::red); // red !
    ui->timer->display("-------");

    // initial variable values
    InitializeValues();
}

MainWindow::~MainWindow()
{
    delete ui;
}

///////////////////      GUI       //////////////////////

void MainWindow::InitializeValues() // Global variables init
{

    loaded = false; // main image NOT loaded
    computed = false; // dominant colors NOT computed

    converted = "No color to convert"; // convert string for displaying color values in a QMessageBox

    // save and load initialization
    basedirinifile = QDir::currentPath().toUtf8().constData(); // where to store the folder ini file
    basedirinifile += "/dir.ini";
    cv::FileStorage fs(basedirinifile, FileStorage::READ); // open dir ini file
    if (fs.isOpened()) {
        fs["BaseDir"] >> basedir; // load default dir
    }
        else basedir = "/home/"; // default base path and file
    basefile = "example";

    quantized = cv::Mat();
    palette = cv::Mat();

    // read color names from .csv file
    std::string line; // line to read in text file
    ifstream names; // file to read
    names.open("color-names.csv"); // read color names file

    if (names) { // if successfully read
        nb_color_names = -1; // index of color names array
        size_t pos; // index for find function
        std::string s; // used for item extraction
        getline(names, line); // read first line (header)
        while (getline(names, line)) { // read each line of text file: R G B name
            pos = 0; // find index at the beginning of the line
            nb_color_names++; // current index in color names array
            int pos2 = line.find(";", pos); // find first semicolon char
            s = line.substr(pos, pos2 - pos); // extract R value
            color_names[nb_color_names].R = std::stoi(s); // R in array
            pos = pos2 + 1; // next char
            pos2 = line.find(";", pos); // find second semicolon char
            s = line.substr(pos, pos2 - pos); // extract G value
            color_names[nb_color_names].G = std::stoi(s); // G in array
            pos = pos2 + 1; // next char
            pos2 = line.find(";", pos); // find third semicolon char
            s = line.substr(pos, pos2 - pos); // extract B value
            color_names[nb_color_names].B = std::stoi(s); // B in array
            s = line.substr(pos2 + 1, line.length() - pos2); // color name is at the end of the line
            color_names[nb_color_names].name = QString::fromStdString(s); // color name in array
        }

        names.close(); // close text file
    }
    else {
        QMessageBox::critical(this, "Colors CSV file not found!", "You forgot to put 'color-names.csv' in the same folder as the executable! This tool will crash as soon as you quantize an image...");
    }
}

void MainWindow::on_button_whats_this_clicked() // What's this function
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::on_button_quit_clicked() // quit GUI
{
    int quit = QMessageBox::question(this, "Quit this wonderful program", "Are you sure you want to quit?", QMessageBox::Yes|QMessageBox::No); // quit, are you sure ?
    if (quit == QMessageBox::No) // don't quit !
        return;

    QCoreApplication::quit(); // quit
}

void MainWindow::on_button_compute_clicked() // compute dominant colors and result images
{
    Compute(); // yes do it !
}

void MainWindow::on_button_3d_reset_clicked() // recenter position & zoom for 3D scene for each color space
{
    ui->openGLWidget_3d->zoom3D = 4; // zoom coefficient

    if (ui->openGLWidget_3d->color_space == "Wheel") {
        ui->openGLWidget_3d->SetXRotation(180);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(-90);
    }
    if (ui->openGLWidget_3d->color_space == "RGB") {
        ui->openGLWidget_3d->SetXRotation(287);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(300);
    }
    if (ui->openGLWidget_3d->color_space == "CIE XYZ") {
        ui->openGLWidget_3d->SetXRotation(287);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(280);
    }
    if (ui->openGLWidget_3d->color_space == "LMS") {
        ui->openGLWidget_3d->SetXRotation(287);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(300);
    }
    if (ui->openGLWidget_3d->color_space == "RGB Triangle") {
        ui->openGLWidget_3d->SetXRotation(26);
        ui->openGLWidget_3d->SetYRotation(30);
        ui->openGLWidget_3d->SetZRotation(180);
    }
    if ((ui->openGLWidget_3d->color_space == "HSV") or (ui->openGLWidget_3d->color_space == "HCV")
            or (ui->openGLWidget_3d->color_space == "HSL") or (ui->openGLWidget_3d->color_space == "HCL")) {
        ui->openGLWidget_3d->SetXRotation(280);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(90);
    }
    if (ui->openGLWidget_3d->color_space == "HWB") {
        ui->openGLWidget_3d->SetXRotation(100);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(-90);
    }
    if ((ui->openGLWidget_3d->color_space == "CIE L*a*b*") or (ui->openGLWidget_3d->color_space == "Hunter Lab") or (ui->openGLWidget_3d->color_space == "OKLAB")) {
        ui->openGLWidget_3d->SetXRotation(290);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(120);
    }
    if (ui->openGLWidget_3d->color_space == "CIE xyY") {
        ui->openGLWidget_3d->SetXRotation(210);
        ui->openGLWidget_3d->SetYRotation(240);
        ui->openGLWidget_3d->SetZRotation(270);
    }
    if (ui->openGLWidget_3d->color_space == "CIE L*u*v*") {
        ui->openGLWidget_3d->SetXRotation(280);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(120);
    }

    ui->openGLWidget_3d->SetXShift(0); // initial (x,y) position
    ui->openGLWidget_3d->SetYShift(0);
}

void MainWindow::on_checkBox_3d_light_clicked() // light on/off in 3D scene
{
    ui->openGLWidget_3d->lightEnabled = ui->checkBox_3d_light->isChecked(); // set value
    ui->openGLWidget_3d->update(); // view 3D scene
}

void MainWindow::on_checkBox_3d_fullscreen_clicked() // view 3D scene fullscreen, <ESC> to return from it
{
    saveXOpenGL = ui->openGLWidget_3d->x(); // save openGL widget position and size
    saveYOpenGL = ui->openGLWidget_3d->y();
    saveWidthOpenGL = ui->openGLWidget_3d->width();
    saveHeightOpenGL = ui->openGLWidget_3d->height();
    saveXButtonSave = ui->button_save_3d->x(); // save button position
    saveYButtonSave = ui->button_save_3d->y();
    ui->openGLWidget_3d->raise(); // bring the 3d widget to front, above all other objects
    ui->button_save_3d->setGeometry(4, 40, ui->button_save_3d->width(), ui->button_save_3d->height()); // set it to upper-left corner
    ui->button_save_3d->raise(); // bring this button to front, above all other objects
    ui->button_3d_exit_fullscreen->setVisible(true); // show exit button
    ui->button_3d_exit_fullscreen->raise(); // bring this button to front, above all other objects

    QRect screenSize = QGuiApplication::primaryScreen()->geometry(); // get screen size in which app is run
    int newW = screenSize.width(); // set 3D widget size to screen
    int newH = screenSize.height();


    setWindowFlags(Qt::Window | Qt::FramelessWindowHint); // window without frame
    show();
    ui->openGLWidget_3d->move(QPoint(0, 0)); // move widget to upper-left position in window
    ui->openGLWidget_3d->resize(QSize(newW, newH)); // resize openGL widget
}

void MainWindow::on_button_3d_exit_fullscreen_clicked() // exit fullscreen view of 3d scene
{
    ui->button_save_3d->setGeometry(saveXButtonSave,saveYButtonSave, ui->button_save_3d->width(), ui->button_save_3d->height()); // move back save image button
    this->setWindowFlags((((windowFlags() | Qt::CustomizeWindowHint)
                            & ~Qt::WindowCloseButtonHint) | Qt::WindowMinMaxButtonsHint)); // window buttons
    show(); // show window
    ui->openGLWidget_3d->move(QPoint(saveXOpenGL, saveYOpenGL)); // restore openGL widget to its previous position
    ui->openGLWidget_3d->resize(QSize(saveWidthOpenGL, saveHeightOpenGL)); // ... and size
    ui->checkBox_3d_fullscreen->setChecked(false); // uncheck fullscreen button
    ui->button_3d_exit_fullscreen->setVisible(false); // hide exit button
    ui->spinBox_3D_rotate_x->raise(); // bring back 3D controls over 3D view
    ui->spinBox_3D_rotate_y->raise();
    ui->spinBox_3D_rotate_z->raise();
    ui->horizontalSlider_3D_rotate_y->raise();
    ui->verticalSlider_3D_rotate_x->raise();
    ui->button_3d_reset->raise();
    ui->checkBox_3d_light->raise();
    ui->checkBox_3d_fullscreen->raise();
}

void MainWindow::on_button_3d_reset_flags_clicked() // reset visibility and selection flags in 3D view
{
    for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) {
        ui->openGLWidget_3d->palettes[n].selected = false;
        ui->openGLWidget_3d->palettes[n].visible = true;
    }

    ui->openGLWidget_3d->update();
    ShowImages();
}

void MainWindow::on_comboBox_color_space_currentIndexChanged(int index) // change color space
{
    ui->openGLWidget_3d->color_space = ui->comboBox_color_space->currentText().toStdString(); // set value
    ui->openGLWidget_3d->update(); // view 3D scene
}

void MainWindow::on_comboBox_sort_currentIndexChanged(int index) // sort palette
{
    SortPalettes(); // call sorting method and palette image reconstruction
}

void MainWindow::on_button_save_3d_clicked() // save current view of 3D color space
{
    QString filename = QFileDialog::getSaveFileName(this, "Save image file",
                                                    QString::fromStdString(basedir + basefile + "-color-space-" + ui->openGLWidget_3d->color_space + ".png"),
                                                    tr("PNG (*.png")); // image filename
    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    ui->openGLWidget_3d->Capture(); // capture current 3D scene
    cv::imwrite(filename.toStdString(), QImage2Mat(ui->openGLWidget_3d->capture3D)); // write image file
}

void MainWindow::on_button_values_clicked() // show converted color in several color spaces
{        
    QMessageBox::information(this, "Color conversion", converted); // show message box with info
}

///////////////////  Keyboard events  //////////////////////

void MainWindow::keyPressEvent(QKeyEvent *keyEvent) // keyboard events
{
    if ((keyEvent->key() == Qt::Key_Escape) & (ui->checkBox_3d_fullscreen->isChecked())) { // only way to get out of fullscreen view
        ui->button_save_3d->setGeometry(saveXButtonSave,saveYButtonSave, ui->button_save_3d->width(), ui->button_save_3d->height()); // move back save image button
        this->setWindowFlags((((windowFlags() | Qt::CustomizeWindowHint)
                                & ~Qt::WindowCloseButtonHint) | Qt::WindowMinMaxButtonsHint)); // window buttons
        show(); // show window
        ui->openGLWidget_3d->move(QPoint(saveXOpenGL, saveYOpenGL)); // restore openGL widget to its previous position
        ui->openGLWidget_3d->resize(QSize(saveWidthOpenGL, saveHeightOpenGL)); // ... and size
        ui->checkBox_3d_fullscreen->setChecked(false); // uncheck fullscreen button
        ui->button_3d_exit_fullscreen->setVisible(false); // hide exit button
        ui->spinBox_3D_rotate_x->raise(); // bring back 3D controls over 3D view
        ui->spinBox_3D_rotate_y->raise();
        ui->spinBox_3D_rotate_z->raise();
        ui->horizontalSlider_3D_rotate_y->raise();
        ui->verticalSlider_3D_rotate_x->raise();
        ui->button_3d_reset->raise();
        ui->checkBox_3d_light->raise();
        ui->checkBox_3d_fullscreen->raise();
    }

    // controls for 3D view
    if (keyEvent->key() == Qt::Key_Left) // move left
        ui->openGLWidget_3d->SetShiftLeft();
    if (keyEvent->key() == Qt::Key_Right) // move right
        ui->openGLWidget_3d->SetShiftRight();
    if (keyEvent->key() == Qt::Key_Up) // move up
        ui->openGLWidget_3d->SetShiftUp();
    if (keyEvent->key() == Qt::Key_Down) // move down
        ui->openGLWidget_3d->SetShiftDown();
    if (keyEvent->key() == Qt::Key_PageUp) // vertical rotation up
        ui->openGLWidget_3d->SetAngleXMinus();
    if (keyEvent->key() == Qt::Key_PageDown) // vertical rotation down
        ui->openGLWidget_3d->SetAngleXPlus();
    if (keyEvent->key() == Qt::Key_Home) // horizontal rotation left
        ui->openGLWidget_3d->SetAngleYMinus();
    if (keyEvent->key() == Qt::Key_End) // horizontal rotation right
        ui->openGLWidget_3d->SetAngleYPlus();
    if (keyEvent->key() == Qt::Key_Insert) // z rotation +
        ui->openGLWidget_3d->SetAngleZMinus();
    if (keyEvent->key() == Qt::Key_Delete) // z rotation -
        ui->openGLWidget_3d->SetAngleZPlus();
}

/////////////////// Mouse events //////////////////////

void MainWindow::mousePressEvent(QMouseEvent *eventPress) // event triggered by a mouse click
{
    mouseButton = eventPress->button(); // mouse button value
    bool key_control = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier); // modifier ctrl key pressed ?
    bool key_alt = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier); // modifier alt key pressed ?

    bool color_found = false; // valid color found ?
    Vec3b color; // BGR values of picked color

    if (ui->label_palette->underMouse()) { // mouse over palette ?
        mouse_pos = ui->label_palette->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!palette.empty())) { // mouse left button clicked
            const QPixmap q = ui->label_palette->pixmap(); // stored reduced quantized image in GUI
            int x = round(palette.cols * double(mouse_pos.x() - (ui->label_palette->width() - q.width()) / 2) / double(q.width())); // real x position in palette image
            int y = round(palette.rows * double(mouse_pos.y() - (ui->label_palette->height() - q.height()) / 2) / double(q.height())); // real y position in palette image
            if ((x > 0) and (x < palette.cols) and (y > 0) and (y < palette.rows)) {
                color = palette.at<Vec3b>(0, x); // pick color in palette
                color_found = true; // found !
            }
        else
            color_found = false;
        }
    }

    if (ui->label_quantized->underMouse()) { // mouse over quantized image ?
        mouse_pos = ui->label_quantized->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!quantized.empty())) { // mouse left button clicked
            const QPixmap q = ui->label_quantized->pixmap(); // stored reduced quantized image in GUI

            double percentX = double(mouse_pos.x() - (ui->label_quantized->width() - q.width()) / 2) / double(q.width()); // real x and y position in quantized image
            double percentY = double(mouse_pos.y() - (ui->label_quantized->height() - q.height()) / 2) / double(q.height());

            if ((percentX >= 0) and (percentX < 1) and (percentY >= 0) and (percentY < 1)) {
                color = quantized.at<Vec3b>(round(percentY * quantized.rows), round(percentX * quantized.cols)); // pick color in quantized image at x,y
                color_found = true; // found !
            }
        }
    }

    if (color_found) { // color picked ?
        // RGB values
        int R = color[2];
        int G = color[1];
        int B = color[0];

        // display color, RGB values
        Mat bar = cv::Mat::zeros(cv::Size(1,1), CV_8UC3); // 1 pixel image
        bar = Vec3b(B,G,R); // set it to picked color
        ui->label_color_bar->setPixmap(Mat2QPixmapResized(bar, ui->label_color_bar->width(), ui->label_color_bar->height(), false)); // show picked color
        ui->label_color_r->setText(QString::number(R)); // show RGB values
        ui->label_color_g->setText(QString::number(G));
        ui->label_color_b->setText(QString::number(B));

        // find color in palette
        bool found = false; // picked color found in palette ?
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // search in palette
            if ((round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0) == R)
                    and (round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0) == G)
                    and (round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0) == B)) { // identical RGB values found
                QString value = QString::number(ui->openGLWidget_3d->palettes[n].percentage * 100, 'f', 2) + "%"; // picked color percentage in quantized image
                ui->label_color_percentage->setText(value); // display percentage
                ui->label_color_name->setText(QString::fromStdString(ui->openGLWidget_3d->palettes[n].name));
                ui->label_color_hex->setText(QString::fromStdString(ui->openGLWidget_3d->palettes[n].hexa)); // show hexa

                found = true; // color found in palette

                if (key_control) {
                    ui->openGLWidget_3d->palettes[n].selected = !ui->openGLWidget_3d->palettes[n].selected; // ctrl key = switch selected state for this color in 3D scene
                    ShowImages();
                }
                if (key_alt)
                    ui->openGLWidget_3d->palettes[n].visible = !ui->openGLWidget_3d->palettes[n].visible; // alt key = switch visibility state for this color in 3D scene

                converted = ConvertColor(ui->openGLWidget_3d->palettes[n].RGB.R, ui->openGLWidget_3d->palettes[n].RGB.G, ui->openGLWidget_3d->palettes[n].RGB.B); // compute string with values in most known color spaces

                ui->openGLWidget_3d->update(); // update 3D view

                break; // get out of loop
            }
        }
        if (!found) { // picked color not found in palette
            ui->label_color_percentage->setText(""); // no percentage displayed
            ui->label_color_name->setText(""); // no color name
            ui->label_color_hex->setText(""); // no hexa
        }
    }
}

/////////////////// Save and load //////////////////////

void MainWindow::SaveDirBaseFile() // write current folder name in ini file
{
    cv::FileStorage fs(basedirinifile, cv::FileStorage::WRITE); // open dir ini file for writing
    fs << "BaseDir" << basedir; // write folder name
    fs.release(); // close file
}

void MainWindow::ChangeBaseDir(QString filename) // set base dir and file
{
    basefile = filename.toUtf8().constData(); // base file name and dir are used after to save other files

    // Remove extension if present
    size_t period_idx = basefile.rfind('.'); // find final dot in filename
    if (std::string::npos != period_idx)
        basefile.erase(period_idx);

    basedir = basefile;
    size_t found = basefile.find_last_of("\\/"); // find last directory
    std::string separator = basefile.substr(found,1); // copy path separator (Linux <> Windows)
    basedir = basedir.substr(0,found) + separator; // define base path
    basefile = basefile.substr(found+1); // delete path in base file

    SaveDirBaseFile(); // Save current path to ini file
}

void MainWindow::on_button_load_image_clicked() // load image to analyze
{
    PreviewFileDialog* mpOpenDialog = new PreviewFileDialog(this, "Load image...", QString::fromStdString(basedir), tr("Images (*.jpg *.jpeg *.jp2 *.png *.tif *.tiff *.webp)"));
    QString filename = mpOpenDialog->GetSelectedFile();
    /*QString filename = QFileDialog::getOpenFileName(this, "Load image...", QString::fromStdString(basedir),
                                                    tr("Images (*.jpg *.jpeg *.jp2 *.png *.tif *.tiff)")); // image file name*/

    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    std::string filesession = filename.toUtf8().constData(); // base file name
    image = cv::imread(filesession); // load image
    if (image.empty()) {
        QMessageBox::critical(this, "File error", "There was a problem reading the image file");
        return;
    }

    if (ui->checkBox_gaussian_blur->isChecked()) cv::GaussianBlur(image, image, Size(3,3), 0, 0); // gaussian blur
    if (ui->checkBox_reduce_size->isChecked()) {
        if ((image.rows > 512) or (image.cols > 512)) image = ResizeImageAspectRatio(image, cv::Size(512,512)); // resize image to 512 pixels
    }

    loaded = true; // loaded successfully !

    ui->label_filename->setText(filename); // display filename in ui

    thumbnail = ResizeImageAspectRatio(image, cv::Size(ui->label_thumbnail->width(),ui->label_thumbnail->height())); // create thumbnail
    quantized.release(); // no quantized image yet
    palette.release(); // no palette image yet
    //classification.release();

    ShowImages(); // show images in GUI
    ui->openGLWidget_3d->nb_palettes = -1; // no palette to show yet
    ui->openGLWidget_3d->update(); // show empty 3D view

    ui->timer->display("-------"); // reset timer in GUI
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // number of colors in black = no error

    ui->label_color_bar->setPixmap(QPixmap()); // show picked color
    ui->label_color_r->setText(""); // show RGB values
    ui->label_color_g->setText("");
    ui->label_color_b->setText("");
    ui->label_color_hex->setText("");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");
}

void MainWindow::on_button_load_lut_clicked() // load .cube LUT to analyze
{
    QString filename = QFileDialog::getOpenFileName(this, "Load Cube LUT...", QString::fromStdString(basedir),
                                                    tr("LUT (*.cube *.CUBE)"), NULL, QFileDialog::DontUseNativeDialog); // .cube LUT file name

    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    std::string filesession = filename.toUtf8().constData(); // base file name

    // Cube LUT
    CubeLUT cube; // new cube
    std::ifstream cubeFile; // file for this cube
    cubeFile.open(filesession, std::ifstream::in); // open cube file
    bool ok = cubeFile.is_open(); // indicator

    if (ok) { // file loaded successfully
        CubeLUT::LUTState state = cube.LoadCubeFile(cubeFile);

        if (state == CubeLUT::OK) {
            QApplication::setOverrideCursor(Qt::WaitCursor); // wait cursor
            timer.start(); // start of elapsed time
            ShowTimer(true); // show elapsed time
            qApp->processEvents();

            if (!cube.LUT1D.empty()) {
                image = cv::Mat::zeros(1, cube.LUT1D.size(), CV_8UC3);
                cv::Vec3b* imageP = image.ptr<cv::Vec3b>(0);
                for (int n = 0; n < int(cube.LUT1D.size()); n++) {
                    imageP[n] = cv::Vec3b(cube.LUT1D[n][2] * 255.0, cube.LUT1D[n][1] * 255.0, cube.LUT1D[n][0] * 255.0);
                }
            }
            else if (!cube.LUT3D.empty()) {
                image = cv::Mat::zeros(1, cube.LUT3D[0].size() * cube.LUT3D[0].size() * cube.LUT3D[0].size(), CV_8UC3);
                cv::Vec3b* imageP = image.ptr<cv::Vec3b>(0);
                int n = 0;
                for (int x = 0; x < int(cube.LUT3D[0].size()); x++) {
                    for (int y = 0; y < int(cube.LUT3D[1].size()); y++) {
                        for (int z = 0; z < int(cube.LUT3D[2].size()); z++) {
                            imageP[n] = cv::Vec3b(cube.LUT3D[x][y][z][2] * 255.0, cube.LUT3D[x][y][z][1] * 255.0, cube.LUT3D[x][y][z][0] * 255.0);
                            n++;
                        }
                    }
                }
            }
            else {
                ShowTimer(false); // show elapsed time
                QApplication::restoreOverrideCursor(); // Restore cursor
                loaded = false;
                QMessageBox::information(this, "Error", "Cube LUT bad format");
                return;
            }

            ShowTimer(false); // show elapsed time
            QApplication::restoreOverrideCursor(); // Restore cursor
        }
    }
    else {
        loaded = false;
        QMessageBox::information(this, "Error", "Problem loading Cube LUT");
        return;
    }

    loaded = true; // loaded successfully !

    ui->label_filename->setText(filename); // display filename in ui

    //thumbnail = ResizeImageAspectRatio(image, cv::Size(ui->label_thumbnail->width(),ui->label_thumbnail->height())); // create thumbnail
    thumbnail = cv::Mat::zeros(3, 3, CV_8UC3);
    quantized.release(); // no quantized image yet
    palette.release(); // no palette image yet
    //classification.release();

    ShowImages(); // show images in GUI
    ui->openGLWidget_3d->nb_palettes = -1; // no palette to show yet
    ui->openGLWidget_3d->update(); // show empty 3D view

    ui->timer->display("-------"); // reset timer in GUI
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // number of colors in black = no error

    ui->label_color_bar->setPixmap(QPixmap()); // show picked color
    ui->label_color_r->setText(""); // show RGB values
    ui->label_color_g->setText("");
    ui->label_color_b->setText("");
    ui->label_color_hex->setText("");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");

    // set special options for LUTs
    ui->checkBox_filter_grays->setChecked(false); // don't filter grays
    ui->checkBox_filter_percent->setChecked(false); // don't filter low values
    ui->spinBox_nb_palettes->setValue(512); // 512 levels of color

    Compute();
}

void MainWindow::on_button_save_clicked() // save dominant colors results
{
    QString filename = QFileDialog::getSaveFileName(this, "Save image file", QString::fromStdString(basedir + basefile + ".png"), tr("PNG (*.png *.PNG)")); // image filename
    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    // if image not empty save it with base name + type .PNG
    if (!quantized.empty())
        cv::imwrite(basedir + basefile + "-quantized.png", quantized);
    if (!palette.empty())
        cv::imwrite(basedir + basefile + "-palette.png", palette);
    ui->openGLWidget_3d->Capture(); // capture current 3D scene
    cv::imwrite(basedir + basefile + "-color-space-" + ui->openGLWidget_3d->color_space + ".png", QImage2Mat(ui->openGLWidget_3d->capture3D)); // save 3D scene image file
    /*if (!classification.empty())
        cv::imwrite(basedir + basefile + "-classification.png", classification);*/

    // palette save to CSV file
    ofstream save; // file to save
    save.open(basedir + basefile + "-palette.csv"); // save palette file

    if (save) { // if successfully open
        setlocale(LC_ALL, "C"); // force numeric separator=dot instead of comma (I'm French) when using std functions
        save << "Name;RGB.R;RGB.G;RGB.B;RGB.R normalized;RGB.G normalized;RGB.B normalized;RGB hexadecimal;HSV.H °;HSV.S;HSV.V;HSV.C;HSL.H °;HSL.S;HSL.L;HSL.C;HWB.H °;HWB.W;HWB.B;XYZ.X;XYZ.Y;XYZ.Z;xyY.x;xyY.y;xyY.Y;L*u*v*.L;L*u*v*.u;L*u*v*.v;LCHuv.L;LCHuv.C;LCHuv.H °;L*A*B*.L;L*A*B*.a signed;L*A*B*.b signed;LCHab.L;LCHab.C;LCHab.H °;Hunter LAB.L;Hunter LAB.a signed;Hunter LAB.b signed;LMS.L;LMS.M;LMS.S;CMYK.C;CMYK.M;CMYK.Y;CMYK.K;OKLAB.L;OKLAB.a signed;OKLAB.b signed;OKLCH.L;OKLCH.C;OKLCH.H °;Percentage\n"; // CSV header
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // read palette
            save << ui->openGLWidget_3d->palettes[n].name << ";";
            // RGB [0..255]
            save << ui->openGLWidget_3d->palettes[n].RGB.R * 255.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].RGB.G * 255.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].RGB.B * 255.0 << ";";
            // RGB [0..1]
            save << ui->openGLWidget_3d->palettes[n].RGB.R << ";";
            save << ui->openGLWidget_3d->palettes[n].RGB.G << ";";
            save << ui->openGLWidget_3d->palettes[n].RGB.B << ";";
            // RGB hexa
            save << ui->openGLWidget_3d->palettes[n].hexa << ";";
            // HSV+C
            save << ui->openGLWidget_3d->palettes[n].HSV.H * 360.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.S * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.V * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.C * 100.0 << ";";
            // HSL+C
            save << ui->openGLWidget_3d->palettes[n].HSL.H * 360.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.S * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.C * 100.0 << ";";
            // HWB
            save << ui->openGLWidget_3d->palettes[n].HWB.H * 360.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HWB.W * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HWB.B * 100.0 << ";";
            // CIE XYZ
            save << ui->openGLWidget_3d->palettes[n].XYZ.X * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].XYZ.Y * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].XYZ.Z * 100.0 << ";";
            // CIE xyY
            save << ui->openGLWidget_3d->palettes[n].XYY.x << ";";
            save << ui->openGLWidget_3d->palettes[n].XYY.y << ";";
            save << ui->openGLWidget_3d->palettes[n].XYY.Y * 100.0 << ";";
            // CIE Luv
            save << ui->openGLWidget_3d->palettes[n].LUV.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LUV.u * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LUV.v * 100.0 << ";";
            // CIE LCHuv
            save << ui->openGLWidget_3d->palettes[n].LCHUV.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LCHUV.C * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LCHUV.H * 360.0 << ";";
            // CIE L*a*b*
            save << ui->openGLWidget_3d->palettes[n].CIELAB.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].CIELAB.A * 127.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].CIELAB.B * 127.0 << ";";
            // CIE LCHab
            save << ui->openGLWidget_3d->palettes[n].LCHAB.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LCHAB.C * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LCHAB.H * 360.0 << ";";
            // Hunter LAB
            save << ui->openGLWidget_3d->palettes[n].HLAB.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HLAB.A * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].HLAB.B * 100.0 << ";";
            // CIE LMS
            save << ui->openGLWidget_3d->palettes[n].LMS.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LMS.M * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].LMS.S * 100.0 << ";";
            // CMYK
            save << ui->openGLWidget_3d->palettes[n].CMYK.C * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].CMYK.M * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].CMYK.Y * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].CMYK.K * 100.0 << ";";
            // OKLAB
            save << ui->openGLWidget_3d->palettes[n].OKLAB.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].OKLAB.A * 127.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].OKLAB.B * 127.0 << ";";
            // OKLCH
            save << ui->openGLWidget_3d->palettes[n].OKLCH.L * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].OKLCH.C * 100.0 << ";";
            save << ui->openGLWidget_3d->palettes[n].OKLCH.H * 360.0 << ";";
            // percentage
            save << ui->openGLWidget_3d->palettes[n].percentage << "\n";
        }

        save.close(); // close text file
    }

    // palette .ACT file (Adobe Photoshop and Illustrator)
    char buffer[771] = {0}; // .ACT files are 772 bytes long
    ofstream saveACT (basedir + basefile + "-palette-adobe.act", ios::out | ios::binary); // open stream

    int nbValues = ui->openGLWidget_3d->nb_palettes; // number of values to write
    if (nbValues > 256) // 256 values max !
        nbValues = 256; // so we'll save first 256 values

    for (int n = 0; n < nbValues; n++) { // palette values to buffer
        buffer[n * 3 + 0] = round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0);
        buffer[n * 3 + 1] = round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0);
        buffer[n * 3 + 2] = round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0);
    }
    buffer[768] = (unsigned short) nbValues; // last second 16-bit value : number of colors in palette
    buffer[770] = (unsigned short) 255; // last 16-bit value : which color is transparency
    saveACT.write(buffer, 772); // write 772 bytes from buffer
    saveACT.close(); // close binary file

    // palette .PAL file (text JASC-PAL for PaintShop Pro)
    ofstream saveJASC; // file to save
    saveJASC.open(basedir + basefile + "-palette-paintshoppro.pal"); // save palette file
    if (saveJASC) { // if successfully open
        saveJASC << "JASC-PAL\n0100\n";
        saveJASC << ui->openGLWidget_3d->nb_palettes << "\n";
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // read palette
            saveJASC << round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0) << " ";
            saveJASC << round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0) << " ";
            saveJASC << round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0) << "\n";
        }
        saveJASC.close(); // close text file
    }

    // palette .PAL file (text with CMYK values for CorelDraw)
    ofstream saveCOREL; // file to save
    saveCOREL.open(basedir + basefile + "-palette-coreldraw.pal"); // save palette file
    if (saveCOREL) { // if successfully open
        double C, M, Y, K;
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // read palette
            RGBtoCMYK(ui->openGLWidget_3d->palettes[n].RGB.R, ui->openGLWidget_3d->palettes[n].RGB.G, ui->openGLWidget_3d->palettes[n].RGB.B, C, M, Y, K);
            saveCOREL << '"' << ui->openGLWidget_3d->palettes[n].name << '"' << " " << int(round(C * 100.0)) << " " << int(round(M * 100.0)) << " " << int(round(Y * 100.0)) << " " << int(round(K * 100.0)) << "\n";
        }
        saveCOREL.close(); // close text file
    }

    QMessageBox::information(this, "Results saved", "Your results were saved with base file name:\n" + QString::fromStdString(basedir + basefile));
}

/////////////////// Core functions //////////////////////

void MainWindow::Compute() // analyze image dominant colors
{
    if (!loaded) { // nothing loaded yet = get out
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor); // wait cursor
    timer.start(); // start of elapsed time
    ShowTimer(true); // show elapsed time
    qApp->processEvents();

    Mat imageCopy; // work on a copy of the image, because gray colors can be filtered
    image.copyTo(imageCopy);

    if (ui->checkBox_filter_grays->isChecked()) { // filter whites, blacks and greys
        Vec3b RGB;
        double H, S, L, C;

        for (int x = 0; x < imageCopy.cols; x++) // parse temp image
            for  (int y = 0; y < imageCopy.rows; y++) {
                RGB = imageCopy.at<Vec3b>(y, x); // current pixel color
                RGBtoHSL(double(RGB[2]) / 255.0, double(RGB[1]) / 255.0, double(RGB[0]) / 255.0, H, S, L, C); // convert it to HSL

                if ((S < 0.25) or (L < 0.15) or (L > 0.8)) // white or black or grey pixel ?
                    imageCopy.at<Vec3b>(y, x) = Vec3b(0, 0, 0); // replace it with black
            }
    }

    ui->openGLWidget_3d->nb_palettes= ui->spinBox_nb_palettes->value(); // how many dominant colors
    int nb_palettes_asked = ui->openGLWidget_3d->nb_palettes; // save asked number of colors for later
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // show number of colors in black (in case it was red before)

    if (ui->checkBox_filter_grays->isChecked()) { // if grays and blacks and whites filtered
        Mat1b black_mask;
        inRange(imageCopy, Vec3b(0, 0, 0), Vec3b(0, 0, 0), black_mask); // extract black pixels from image
        if ((cv::sum(black_mask) != Scalar(0,0,0))) // image contains black pixels ?
                ui->openGLWidget_3d->nb_palettes++; // add one color to asked number of colors in palette, to remove it later and get only colors
    }

    // set palette values to 0;
    for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) {
        ui->openGLWidget_3d->palettes[n].RGB.R = 0.0;
        ui->openGLWidget_3d->palettes[n].RGB.G = 0.0;
        ui->openGLWidget_3d->palettes[n].RGB.B = 0.0;
        ui->openGLWidget_3d->palettes[n].count = 0;
        ui->openGLWidget_3d->palettes[n].percentage = 0.0;
        ui->openGLWidget_3d->palettes[n].selected = false; // color not selected
        ui->openGLWidget_3d->palettes[n].visible = true; // color shown
    }

    if (ui->radioButton_eigenvectors->isChecked()) { // eigen method
        cv::Mat cielab = ConvertImageRGBtoCIELab(imageCopy);
        std::vector<cv::Vec3d> palette_vec = DominantColorsEigen(cielab, ui->openGLWidget_3d->nb_palettes, quantized); // get dominant palette, palette image and quantized image
        quantized = ConvertImageCIELabToRGB(quantized);

        /*for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) // store palette values in structured array
        {
            double R, G, B;
            CIELabToRGB(palette_vec[n][0], palette_vec[n][1], palette_vec[n][2], R, G, B);
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = R;
            ui->openGLWidget_3d->palettes[n].RGB.G = G;
            ui->openGLWidget_3d->palettes[n].RGB.B = B;
        }*/

        // palette from quantized image
        cv::Vec3b color[ui->openGLWidget_3d->nb_palettes]; // temp palette
        int nbColor = 0; // current color
        for (int x = 0; x < quantized.cols; x++) // parse entire image
            for (int y = 0; y < quantized.rows; y++) {
                cv::Vec3b col = quantized.at<cv::Vec3b>(y, x); // current pixel color
                bool found = false;
                for (int i = 0; i < nbColor; i++) // look into temp palette
                    if (col == color[i]) { // if color already exists
                        found = true; // found, don't add it
                        break;
                    }
                if (!found) { // color not already in temp palette
                    color[nbColor] = col; // save new color
                    ui->openGLWidget_3d->palettes[nbColor].RGB.R = col[2] / 255.0; // copy RGB values to global palette
                    ui->openGLWidget_3d->palettes[nbColor].RGB.G = col[1] / 255.0;
                    ui->openGLWidget_3d->palettes[nbColor].RGB.B = col[0] / 255.0;
                    nbColor++; // one more color
                }
            }

        ui->openGLWidget_3d->ConvertPaletteFromRGB(); // convert RGB to other values
    }
    else if (ui->radioButton_k_means->isChecked()) { // K-means method
        cv::Mat1f colors; // to store palette from K-means
        quantized = DominantColorsKMeansRGB(imageCopy, ui->spinBox_nb_palettes->value(), colors); // get quantized image and palette

        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) // store palette in structured array
        {
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = double(colors(n, 2)) / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.G = double(colors(n, 1)) / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.B = double(colors(n, 0)) / 255.0;
        }
        ui->openGLWidget_3d->ConvertPaletteFromRGB(); // convert RGB to other values

        //classification.release();
    }

    // compute HSL values from RGB + compute hexa
    for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) {
        // HSL value
        double C;
        RGBtoHSL(ui->openGLWidget_3d->palettes[n].RGB.R, ui->openGLWidget_3d->palettes[n].RGB.G, ui->openGLWidget_3d->palettes[n].RGB.B,
                 ui->openGLWidget_3d->palettes[n].HSL.H, ui->openGLWidget_3d->palettes[n].HSL.S, ui->openGLWidget_3d->palettes[n].HSL.L, C); // convert current color to HSL

        // hexadecimal value
        QString hex = QString(" %1").arg(((int(round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0)) & 0xff) << 16)
                                       + ((int(round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0)) & 0xff) << 8)
                                       +  (int(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0)) & 0xff)
                                         , 6, 16, QChar('0')).trimmed(); // compute hexa RGB value
        hex = "#" + hex;
        ui->openGLWidget_3d->palettes[n].hexa = hex.toUpper().toUtf8().constData();
    }

    // clean palette : number of asked colors may be superior to number of colors found
    int n = CountRGBUniqueValues(quantized); // how many colors in quantized image, really ?
    if (n < ui->openGLWidget_3d->nb_palettes) { // if asked number of colors exceeds total number of colors in image
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // sort palette by hexa value, decreasing values
        if ((ui->openGLWidget_3d->palettes[0].RGB.R == ui->openGLWidget_3d->palettes[1].RGB.R)
                and (ui->openGLWidget_3d->palettes[0].RGB.G == ui->openGLWidget_3d->palettes[1].RGB.G)
                and (ui->openGLWidget_3d->palettes[0].RGB.B == ui->openGLWidget_3d->palettes[1].RGB.B)) // if first color in palette is equal to second we have to reverse sort by percentage
            std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                      [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // sort the palette, this time by increasing hexa values
        ui->openGLWidget_3d->nb_palettes = n; // new number of colors in palette
        ui->spinBox_nb_palettes->setValue(ui->openGLWidget_3d->nb_palettes); // show new number of colors
    }

    int total = quantized.rows * quantized.cols; // size of quantized image in pixels

    // delete blacks in palette if needed
    bool black_found = false;
    if (ui->checkBox_filter_grays->isChecked()) { // delete last "black" values in palette
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
              [](const struct_palette& a, const struct_palette& b) {return a.HSL.L > b.HSL.L;}); // sort palette by lightness value
        while (ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].HSL.L < 0.15) { // at the end of palette, find black colors
            Mat1b black_mask;
            inRange(quantized, Vec3b(int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.R * 255.0))),
                               Vec3b(int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.R * 255.0))),
                               black_mask); // extract black color from image
            int c = countNonZero(black_mask);
            total = total - c; // update total pixel count
            ui->openGLWidget_3d->nb_palettes--; // exclude this black color from palette
            if (c > 0) // really found black color ?
                black_found = true; // black color found !
        }
        if (black_found) { // if black color found
            ui->spinBox_nb_palettes->setValue(ui->openGLWidget_3d->nb_palettes); // show new number of colors without black values
        }
    }

    // compute percentages
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) { // for each color in palette
        Mat1b mask; // current color mask
        inRange(quantized, Vec3b(int(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0))),
                           Vec3b(int(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0))),
                           mask); // create mask for current color
        ui->openGLWidget_3d->palettes[n].count = cv::countNonZero(mask); // count pixels in this mask
        ui->openGLWidget_3d->palettes[n].percentage = double(ui->openGLWidget_3d->palettes[n].count) / double(total); // compute color use percentage
    }

    // delete non significant values in palette by percentage
    if (ui->checkBox_filter_percent->isChecked()) { // filter by x% ?
        bool cleaning_found = false; // indicator
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
              [](const struct_palette& a, const struct_palette& b) {return a.percentage > b.percentage;}); // sort palette by percentage
        while (double(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].count) / double(total) < double(ui->spinBox_nb_percentage->value()) / 100.0) { // at the end of palette, find values < x%
            Mat1b cleaning_mask;
            inRange(quantized, Vec3b(int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.R * 255.0))),
                               Vec3b(int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.B * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.G * 255.0)), int(round(ui->openGLWidget_3d->palettes[ui->openGLWidget_3d->nb_palettes - 1].RGB.R * 255.0))),
                               cleaning_mask); // extract color from image
            int c = countNonZero(cleaning_mask); // count occurences
            total = total - c; // update total pixel count
            ui->openGLWidget_3d->nb_palettes--; // exclude this color from palette
            if (c > 0) // really found this color ?
                cleaning_found = true; // nb_palettes has to change
        }
        if (cleaning_found) { // if cleaning found
            ui->spinBox_nb_palettes->setValue(ui->openGLWidget_3d->nb_palettes); // show new number of colors without cleaned values
            // re-compute percentages
            for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // for each color in palette
                ui->openGLWidget_3d->palettes[n].percentage = double(ui->openGLWidget_3d->palettes[n].count) / double(total); // update percentage
        }
    }

    // find color name by euclidian distance
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) { // for each color in palette
        bool found = false;
        int distance = 1000000; // distance formula can never reach this high
        int index; // to keep nearest color index in color names table

        for (int c = 0; c < nb_color_names; c++) { // search in color names table
            int d = pow(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0 - color_names[c].R, 2) + pow(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0 - color_names[c].G, 2) + pow(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0 - color_names[c].B, 2); // euclidian distance
            if (d == 0) { // exact RGB values found
                ui->openGLWidget_3d->palettes[n].name = color_names[c].name.toUtf8().constData(); // assign color name
                found = true; // color found in palette
                break; // get out of loop
            }
            else {
                if (d < distance) { // if distance is smaller
                    distance = d; // new distance
                    index = c; // keep index
                }
            }
        }
        if (!found) // picked color not found in palette so display nearest color
            ui->openGLWidget_3d->palettes[n].name = color_names[index].name.toUtf8().constData(); // assign color name
    }

    SortPalettes(); // sort and create palette image

    // reset UI elements
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // show number of colors in black (in case it was red before)
    ui->label_color_bar->setPixmap(QPixmap()); // reset picked color
    ui->label_color_bar->setText("Pick\nColor");
    ui->label_color_r->setText("R"); // show RGB values
    ui->label_color_g->setText("G");
    ui->label_color_b->setText("B");
    ui->label_color_hex->setText("Hex");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");
    if (ui->openGLWidget_3d->nb_palettes < nb_palettes_asked) { // more colors asked than were really found ?
        ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:red;}"); // show new number of colors in red
        ui->spinBox_nb_palettes->setValue(ui->openGLWidget_3d->nb_palettes); // show new number of colors
    }

    ui->openGLWidget_3d->update(); // show 3D view
    ShowImages(); // show result images

    ShowTimer(false); // show elapsed time
    QApplication::restoreOverrideCursor(); // Restore cursor

    computed = true; // success !
}

void MainWindow::ShowImages() // display result images in GUI
{
    if (!thumbnail.empty())
        ui->label_thumbnail->setPixmap(Mat2QPixmap(thumbnail)); // thumbnail
        else ui->label_thumbnail->setPixmap(QPixmap());
    if (!quantized.empty()) {
        Mat selected;
        quantized.copyTo(selected);
        bool found = false;
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) {
            if (ui->openGLWidget_3d->palettes[n].selected) {
                Mat mask = quantized == Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0), round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0), round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0));
                selected.setTo(Vec3b(255,255,255), mask);
                found = true;
            }
        }
        if (found)
            ui->label_quantized->setPixmap(Mat2QPixmapResized(selected, ui->label_quantized->width(), ui->label_quantized->height(), true)); // quantized image + selection
        else
            ui->label_quantized->setPixmap(Mat2QPixmapResized(quantized, ui->label_quantized->width(), ui->label_quantized->height(), true)); // quantized image
    }
    else
        ui->label_quantized->setPixmap(QPixmap());
    if (!palette.empty())
        ui->label_palette->setPixmap(Mat2QPixmapResized(palette, ui->label_palette->width(), ui->label_palette->height(), true)); // palette image
        else ui->label_palette->setPixmap(QPixmap());
    /*if (!classification.empty())
        ui->label_classification->setPixmap(Mat2QPixmapResized(classification, ui->label_classification->width(), ui->label_classification->height(), true)); // classification image
        else ui->label_classification->setPixmap(QPixmap());*/
}

void MainWindow::ShowTimer(const bool start) // time elapsed
{
    if (start)
        ui->timer->display(".BUSY..."); // wait message
    else {
        int milliseconds = int(timer.elapsed()%1000); // milliseconds
        int seconds = int(timer.elapsed()/1000%60); // seconds
        ui->timer->display(QString("%1").arg(seconds, 3, 10, QChar('0'))
                          + "."
                          + QString("%1").arg(milliseconds, 3, 10, QChar('0'))); // show value in LCD timer
    }
}

void MainWindow::SortPalettes() // sort palette values and create palette image
{
    if (ui->openGLWidget_3d->nb_palettes < 1) // no palette -> get out
        return;

    // sort by type
    if (ui->comboBox_sort->currentText() == "Percentage")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.percentage > b.percentage;});
    else if (ui->comboBox_sort->currentText() == "Lightness")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HSL.L > b.HSL.L;});
    else if (ui->comboBox_sort->currentText() == "Luminance")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.XYZ.Y > b.XYZ.Y;});
    else if (ui->comboBox_sort->currentText() == "Hue")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HSV.H > b.HSV.H;});
    else if (ui->comboBox_sort->currentText() == "Saturation")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HSV.S > b.HSV.S;});
    else if (ui->comboBox_sort->currentText() == "Chroma")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HSV.C > b.HSV.C;});
    else if (ui->comboBox_sort->currentText() == "Value")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HSV.V > b.HSV.V;});
    else if (ui->comboBox_sort->currentText() == "Distance")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b)
                    {return pow(1-a.RGB.R, 2) + pow(1-a.RGB.G, 2) + pow(1-a.RGB.B, 2) > pow(1-b.RGB.R, 2) + pow(1-b.RGB.G, 2) + pow(1-b.RGB.B, 2);});
    else if (ui->comboBox_sort->currentText() == "Whiteness")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HWB.W > b.HWB.W;});
    else if (ui->comboBox_sort->currentText() == "Blackness")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.HWB.B > b.HWB.B;});
    else if (ui->comboBox_sort->currentText() == "RGB")
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.hexa < b.hexa;});
    else if (ui->comboBox_sort->currentText() == "Luma") // Luma = Sqrt(0.241*R + 0.691*G + 0.068*B)
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return 0.241 * a.RGB.R + 0.691 * a.RGB.G + 0.068 * a.RGB.B < 0.241 * b.RGB.R + 0.691 * b.RGB.G + 0.068 * b.RGB.B;});
    else if (ui->comboBox_sort->currentText() == "Rainbow6") // Hue + Luma
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return int(a.HSL.H * 60.0) + sqrt(0.241 * a.RGB.R + 0.691 * a.RGB.G + 0.068 * a.RGB.B) < int(b.HSL.H * 60.0) + sqrt(0.241 * b.RGB.R + 0.691 * b.RGB.G + 0.068 * b.RGB.B);});

    // create palette image - could be a mess over 250 values
    palette = Mat::zeros(cv::Size(palette_width, palette_height), CV_8UC3); // create blank palette image
    double offset = 0; // current x position in palette
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) { // for each color in palette
        cv::rectangle(palette, Rect(round(offset), 0,
                                    palette_width, palette_height),
                                    Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0),
                                          round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0),
                                          round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0)), -1); // rectangle of current color
        offset += round(ui->openGLWidget_3d->palettes[n].percentage * double(palette_width)); // next x position in palette
    }
    if (offset <= palette_width) {
        cv::Rect crop(0, 0, offset, palette_height);
        palette = palette(crop);
    }

    ShowImages(); // show images
}

QString MainWindow::ConvertColor(const double &R, const double &G, const double &B) // convert a RGB color to other color spaces to a single string for whoing it later
{
    QString result;

    // RGB
    result += "<b>RGB (byte)</b>......... <b><font color='red'>R</font></b>: " + QString::number(round(R * 255.0))
                + " <b><font color='limegreen'>G</font></b>: " + QString::number(round(G * 255.0))
                + " <b><font color='blue'>B</font></b>: " + QString::number(round(B * 255.0)) + "<br>";
    result += "<b>RGB (%)</b>.............. <b><font color='red'>R</font></b>: " + QString::number(R, 'G', 5)
                + " <b><font color='limegreen'>G</font></b>: " + QString::number(G, 'G', 5)
                + " <b><font color='blue'>B</font></b>: " + QString::number(B, 'G', 5) + "<br>";

    // HSL + C
    double H, S, L, V, C; // HSLVC values
    RGBtoHSL(R, G, B,
             H, S, L, C); // convert RGB to HSV values
    result += "<b>HSL + C</b>............... <b><font color='magenta'>H</font></b>: " + QString::number(H * 360.0, 'G', 5)
                + "° <b>S</b>: " + QString::number(S * 100.0, 'G', 5)
                + " <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(round(C * 100.0)) + "<br>";

    // HSV + C
    RGBtoHSV(R, G, B,
             H, S, V, C); // convert RGB to HSL values
    result += "<b>HSV + C</b>.............. <b><font color='magenta'>H</font></b>: " + QString::number(H * 360.0, 'G', 5)
                + "° <b>S</b>: " + QString::number(S * 100.0, 'G', 5)
                + " <b><font color='cadetblue'>V</font></b></b>: " + QString::number(V * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(round(C * 100.0)) + "<br>";

    // HWB
    double h, W, b; // HWB values
    HSVtoHWB(H, S, V, h, W, b);
    result += "<b>HWB</b>.................... <b><font color='magenta'>H</font></b>: " + QString::number(h * 360.0, 'G', 5)
                + "° <b><font color='lightsteelblue'>W</font></b>: " + QString::number(W * 100.0, 'G', 5)
                + " <b><font color='slategrey'>B</font></b>: " + QString::number(b * 100.0, 'G', 5) + "<br>";

    // XYZ
    double X, Y, Z; // XYZ and LAB values
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ values
    result += "<b>CIE XYZ</b>.............. <b>X</b>: " + QString::number(X * 100.0, 'G', 5)
                + " <b><font color='cadetblue'>Y</font></b>: " + QString::number(Y * 100.0, 'G', 5)
                + " <b>Z</b>: " + QString::number(Z * 100.0, 'G', 5) + "<br>";

    // xyY
    double x, y;
    XYZtoxyY(X, Y, Z, x, y);
    result += "<b>CIE xyY</b>............... <b>x</b>: " + QString::number(x * 100.0, 'G', 5)
                + " <b>y</b>: " + QString::number(y * 100.0, 'G', 5)
                + " <b><font color='cadetblue'>Y</font></b>: " + QString::number(Y * 100.0, 'G', 5) + "<br>";

    // L*u*v*
    double u, v;
    XYZtoCIELuv(X, Y, Z, L, u, v);
    result += "<b>CIE L*u*v*</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b>u</b>: " + QString::number(u * 100.0, 'G', 5)
                + " <b>v</b>: " + QString::number(v * 100.0, 'G', 5) + "<br>";

    // LCH*u*v*
    CIELuvToCIELCHuv(u, v, C, H);
    result += "<b>CIE LChuv</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(C * 100.0, 'G', 5)
                + " <b><font color='magenta'>h</font></b>: " + QString::number(v * 360.0, 'G', 5) + "°<br>";

    // L*A*B*
    double a;
    XYZtoCIELab(X, Y, Z, L, a, b); // convert XYZ to LAB values
    result += "<b>CIE L*a*b*</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkkhaki'>a</font></b>: " + QString::number(a * 127.0, 'G', 5)
                + " <b><font color='forestgreen'>b</font></b>: " + QString::number(b * 127.0, 'G', 5) + "<br>";

    // LCH*u*v*
    CIELabToCIELCHab(a, b, C, H);
    result += "<b>CIE LChuv</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(C * 100.0, 'G', 5)
                + " <b><font color='magenta'>h</font></b>: " + QString::number(v * 360.0, 'G', 5) + "°<br>";

    // Hunter LAB
    XYZtoHLAB(X, Y, Z, L, a, b); // convert XYZ to Hunter LAB values
    result += "<b>Hunter Lab</b>........ <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkkhaki'>a</font></b>: " + QString::number(a * 100.0, 'G', 5)
                + " <b><font color='forestgreen'>b</font></b>: " + QString::number(b * 100.0, 'G', 5) + "<br>";

    // CMYK
    double M, K;
    RGBtoCMYK(R, G, B, C, M, Y, K); // convert RGB to CMYK values
    result += "<b>CMYK</b>.................. <b><font color='cyan'>C</font></b>: " + QString::number(C * 100.0, 'G', 5)
                + " <b><font color='magenta'>M</font></b>: " + QString::number(M * 100.0, 'G', 5)
                + " <b><font color='gold'>Y</font></b>: " + QString::number(Y * 100.0, 'G', 5)
                + " <b><font color='black'>K</font></b>: " + QString::number(K * 100.0, 'G', 5) + "<br>";

    return result;
}
