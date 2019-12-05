/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2 - 2019/11/08
#
#-------------------------------------------------*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSizeGrip>
#include <QGridLayout>
#include <QDesktopWidget>
#include <QCursor>
#include <QMouseEvent>

//#include "mat-image-tools.h"
#include "dominant-colors.h"

using namespace cv;
using namespace cv::ximgproc;
using namespace std;

/////////////////// Window init //////////////////////

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    // read color names form .csv file
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
        QMessageBox::critical(this, "Colors CSV file not found!", "You forgot to put 'color-names.csv' in the same folder as the executable! This tool will crash if you click on a color...");
    }
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
    if ((ui->openGLWidget_3d->color_space == "CIE L*a*b*") or (ui->openGLWidget_3d->color_space == "Hunter Lab")) {
        ui->openGLWidget_3d->SetXRotation(290);
        ui->openGLWidget_3d->SetYRotation(0);
        ui->openGLWidget_3d->SetZRotation(310);
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

    QRect screenSize = qApp->desktop()->availableGeometry(qApp->desktop()->primaryScreen()); // get screen size in which app is run
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
                                                    "PNG (*.png *.PNG)"); // image filename
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
            const QPixmap* q = ui->label_palette->pixmap(); // stored reduced quantized image in GUI
            int x = palette.cols * double(mouse_pos.x() - (ui->label_palette->width() - q->width()) / 2) / double(q->width()); // real x position in palette image
            //int x = round(double(palette_width) * mouse_pos.x() / ui->label_palette->width()); // x position in palette
            if ((x >= 0) and (x <= palette.cols)) {
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
            const QPixmap* q = ui->label_quantized->pixmap(); // stored reduced quantized image in GUI

            double percentX = double(mouse_pos.x() - (ui->label_quantized->width() - q->width()) / 2) / double(q->width()); // real x and y position in quantized image
            double percentY = double(mouse_pos.y() - (ui->label_quantized->height() - q->height()) / 2) / double(q->height());

            color = quantized.at<Vec3b>(round(percentY * quantized.rows), round(percentX * quantized.cols)); // pick color in quantized image at x,y
            color_found = true; // found !
        }
    }

    if (color_found) { // color picked ?
        int R = color[2]; // RGB values from BGR
        int G = color[1];
        int B = color[0];

        // display color, RGB values
        Mat bar = cv::Mat::zeros(cv::Size(1,1), CV_8UC3); // 1 pixel image
        bar = Vec3b(B,G,R); // set it to picked color
        ui->label_color_bar->setPixmap(Mat2QPixmapResized(bar, ui->label_color_bar->width(), ui->label_color_bar->height(), false)); // show picked color

        ui->label_color_r->setText(QString::number(R)); // show RGB values
        ui->label_color_g->setText(QString::number(G));
        ui->label_color_b->setText(QString::number(B));

        QString hex = QString(" %1").arg(((R & 0xff) << 16) + ((G & 0xff) << 8) + (B & 0xff), 6, 16, QChar('0')).trimmed(); // compute hexa RGB value
        ui->label_color_hex->setText("#" + hex.toUpper()); // show it
        
        // find color in palette
        bool found = false; // picked color found in palette ?
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // search in palette
            if ((round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0) == R) and (round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0) == G) and (round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0) == B)) { // identical RGB values found
                QString value = QString::number(ui->openGLWidget_3d->palettes[n].percentage * 100, 'f', 2) + " %"; // picked color percentage in quantized image
                ui->label_color_percentage->setText(value); // display percentage

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
            converted = "No color to convert";
        }

        // find nearest color name by euclidian distance
        found = false;
        int distance = 1000000; // distance formula can never reach this high
        int index; // nearest color index in color names table

        for (int n = 0; n < nb_color_names; n++) { // search in color names table
            int d = pow(R - color_names[n].R, 2) + pow(G - color_names[n].G, 2) + pow(B - color_names[n].B, 2); // euclidian distance
            if (d == 0) { // exact RGB values found
                ui->label_color_name->setText(color_names[n].name); // display color name
                found = true; // color found in palette
                break; // get out of loop
            }
            else {
                if (d < distance) { // if distance is smaller
                    distance = d; // new distance
                    index = n; // keep index for this new distance
                }
            }
        }
        if (!found) { // picked color not found in palette so display nearest color
            ui->label_color_name->setText("nearest: "
                                          + color_names[index].name
                                          + " (" + QString::number(color_names[index].R) + ","
                                          + QString::number(color_names[index].G) + ","
                                          + QString::number(color_names[index].B) + ")");
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
    QString filename = QFileDialog::getOpenFileName(this, "Load image...", QString::fromStdString(basedir),
                                                    "Images (*.jpg *.JPG *.jpeg *.JPEG *.jp2 *.JP2 *.png *.PNG *.tif *.TIF *.tiff *.TIFF *.bmp *.BMP)"); // image file name

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

    ui->timer->display("0"); // reset timer in GUI
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // number of colors in black = no error

    ui->label_color_bar->setPixmap(QPixmap()); // show picked color
    ui->label_color_r->setText(""); // show RGB values
    ui->label_color_g->setText("");
    ui->label_color_b->setText("");
    ui->label_color_hex->setText("");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");
}

void MainWindow::on_button_save_clicked() // save dominant colors results
{
    QString filename = QFileDialog::getSaveFileName(this, "Save image file", QString::fromStdString(basedir + basefile + ".png"), "PNG (*.png *.PNG)"); // image filename
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
    std::string line; // line to write in text file
    ofstream save; // file to save
    save.open(basedir + basefile + "-palette.csv"); // save palette file

    if (save) { // if successfully open
        setlocale(LC_ALL, "C"); // force numeric separator=dot instead of comma (I'm French) when using std functions
        save << "RGB.R byte;RGB.G byte;RGB.B byte;RGB.R %;RGB.G %;RGB.B %;RGB hexadecimal;HSV.H °;HSV.S %;HSV.V %;HSV.C %;HSL.H °;HSL.S %;HSL.L %;HSL.C %;HWB.H °;HWB.W %;HWB.B %;XYZ.X %;XYZ.Y %;XYZ.Z %;xyY.x %;xyY.y %;xyY.Y %;L*u*v*.L %;L*u*v*.u %;L*u*v*.v %;LCHuv.L %;LCHuv.C %;LCHuv.H °;L*A*B*.L %;L*A*B*.a signed max=127;L*A*B*.b signed max=127;LCHab.L %;LCHab.C %;LCHab.H °;Hunter LAB.L %;Hunter LAB.a signed %;Hunter LAB.b signed %;LMS.L %;LMS.M %;LMS.S °;Percentage\n"; // CSV header
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // read palette
            // RGB [0..255]
            save << round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0) << ";";
            save << round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0) << ";";
            save << round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0) << ";";
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
            // percentage
            save << ui->openGLWidget_3d->palettes[n].percentage << "\n";
        }

        save.close(); // close text file
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
    ShowTimer(); // show elapsed time
    qApp->processEvents();

    ui->openGLWidget_3d->nb_palettes = ui->spinBox_nb_palettes->value(); // how many dominant colors to compute

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
        std::vector<cv::Vec3b> palette_vec = DominantColorsEigen(image, ui->openGLWidget_3d->nb_palettes, &quantized); // get dominant palette, palette image and quantized image

        for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // store palette values in structured array
        {
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = palette_vec[n][2] / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.G = palette_vec[n][1] / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.B = palette_vec[n][0] / 255.0;
        }
        ui->openGLWidget_3d->ConvertPaletteFromRGB(); // convert RGB to other values
    }
    else if (ui->radioButton_k_means->isChecked()) { // K-means method
        cv::Mat1f colors; // to store palette from K-means
        quantized = DominantColorsKMeans(image, ui->spinBox_nb_palettes->value(), &colors); // get quantized image and palette

        for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // store palette in structured array
        {
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = colors(n, 2) / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.G = colors(n, 1) / 255.0;
            ui->openGLWidget_3d->palettes[n].RGB.B = colors(n, 0) / 255.0;
        }
        ui->openGLWidget_3d->ConvertPaletteFromRGB(); // convert RGB to other values

        //classification.release();
    }

    // count occurences of colors in quantized image
    int total = quantized.rows * quantized.cols; // size of quantized image in pixels
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) { // for each color in palette
        Mat1b mask; // current color mask
        inRange(quantized, Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0)),
                           Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0)),
                           mask); // create mask for current color
        ui->openGLWidget_3d->palettes[n].count = cv::countNonZero(mask); // count pixels in mask
        ui->openGLWidget_3d->palettes[n].percentage = double(ui->openGLWidget_3d->palettes[n].count) / double(total); // color use percentage
    }

    // clean palette : number of asked colors may be superior to number of colors found
    int n = CountRGBUniqueValues(quantized); // how many colors in quantized image, really ?
    if (n < ui->openGLWidget_3d->nb_palettes) { // if asked number of colors exceeds total number of colors in image
        std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // sort palette by percentage, decreasing values
        if ((ui->openGLWidget_3d->palettes[0].RGB.R == ui->openGLWidget_3d->palettes[1].RGB.R)
                and (ui->openGLWidget_3d->palettes[0].RGB.G == ui->openGLWidget_3d->palettes[1].RGB.G)
                and (ui->openGLWidget_3d->palettes[0].RGB.B == ui->openGLWidget_3d->palettes[1].RGB.B)) // if first color in palette is equal to second we have to reverse sort by percentage
            std::sort(ui->openGLWidget_3d->palettes, ui->openGLWidget_3d->palettes + ui->openGLWidget_3d->nb_palettes,
                      [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // this time by increasing percentage values
        ui->openGLWidget_3d->nb_palettes = n; // new number of colors in palette
        ui->spinBox_nb_palettes->setValue(ui->openGLWidget_3d->nb_palettes); // show new number of colors
        ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:red;}"); // show new number of colors in red
    }
    else // asked number of colors does not exceed total number of colors in image
        ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;}"); // show number of colors in black (in case it was red before)

    SortPalettes(); // sort and create palette image

    ui->openGLWidget_3d->update(); // show 3D view
    ShowImages(); // show result images

    ShowTimer(); // show elapsed time
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

void MainWindow::ShowTimer() // show elapsed time
{
    int milliseconds = int(timer.elapsed() % 1000); // milliseconds
    int seconds = int(round(timer.elapsed() / 1000.0)) % 60; // seconds
    ui->timer->display(QString(" %1").arg(seconds, 3, 10, QChar('0')).trimmed()
                      + "."
                      + QString(" %1").arg(milliseconds, 3, 10, QChar('0')).trimmed()); // show elapsed time
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
    XYZtoLuv(X, Y, Z, L, u, v);
    result += "<b>CIE L*u*v*</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b>u</b>: " + QString::number(u * 100.0, 'G', 5)
                + " <b>v</b>: " + QString::number(v * 100.0, 'G', 5) + "<br>";

    // LCH*u*v*
    LUVtoLCHuv(u, v, C, H);
    result += "<b>CIE LChuv</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(C * 100.0, 'G', 5)
                + " <b><font color='magenta'>h</font></b>: " + QString::number(v * 360.0, 'G', 5) + "°<br>";

    // L*A*B*
    double a;
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to LAB values
    result += "<b>CIE L*a*b*</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkkhaki'>a</font></b>: " + QString::number(a * 127.0, 'G', 5)
                + " <b><font color='forestgreen'>b</font></b>: " + QString::number(b * 127.0, 'G', 5) + "<br>";

    // LCH*u*v*
    LABtoLCHab(a, b, C, H);
    result += "<b>CIE LChuv</b>.......... <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkorange'>C</font></b>: " + QString::number(C * 100.0, 'G', 5)
                + " <b><font color='magenta'>h</font></b>: " + QString::number(v * 360.0, 'G', 5) + "°<br>";

    // Hunter LAB
    XYZtoHLAB(X, Y, Z, L, a, b); // convert XYZ to Hunter LAB values
    result += "<b>Hunter Lab</b>........ <b><font color='cadetblue'>L</font></b>: " + QString::number(L * 100.0, 'G', 5)
                + " <b><font color='darkkhaki'>a</font></b>: " + QString::number(a * 100.0, 'G', 5)
                + " <b><font color='forestgreen'>b</font></b>: " + QString::number(b * 100.0, 'G', 5) + "<br>";

    return result;
}
