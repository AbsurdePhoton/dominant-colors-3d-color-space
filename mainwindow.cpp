/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0.1 - 2019/10/24
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
    ui->comboBox_color_space->blockSignals(true);
    ui->comboBox_color_space->addItem(tr("RGB"));
    ui->comboBox_color_space->addItem(tr("HSV"));
    ui->comboBox_color_space->addItem(tr("HCV"));
    ui->comboBox_color_space->addItem(tr("HSL"));
    ui->comboBox_color_space->addItem(tr("HCL"));
    ui->comboBox_color_space->addItem(tr("HWB"));
    ui->comboBox_color_space->addItem(tr("CIE XYZ"));
    ui->comboBox_color_space->addItem(tr("CIE L*a*b*"));
    //ui->comboBox_color_space->addItem(tr("CIE LCh"));
    ui->comboBox_color_space->addItem(tr("Hunter Lab"));
    ui->comboBox_color_space->addItem(tr("Wheel"));
    ui->comboBox_color_space->blockSignals(false);

    // populate sort combobox
    ui->comboBox_sort->blockSignals(true);
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
    ui->comboBox_sort->blockSignals(false);

    // Timer
    ui->timer->setPalette(Qt::red);

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

    basedirinifile = QDir::currentPath().toUtf8().constData(); // where to store the folder ini file
    basedirinifile += "/dir.ini";
    cv::FileStorage fs(basedirinifile, FileStorage::READ); // open dir ini file
    if (fs.isOpened()) {
        fs["BaseDir"] >> basedir; // load dir
    }
        else basedir = "/home/"; // default base path and file
    basefile = "example";

    // Color names
    std::string line; // line to read in text file
    ifstream names; // file to read
    names.open("color-names.txt"); // read color names file

    if (names) { // if successfully read
        nb_color_names = -1; // index of color names array
        size_t pos; // index for find function
        std::string s; // used for item extraction
        while (getline(names, line)) { // read each line of text file: R G B name
            pos = 0; // find index at the beginning of the line
            nb_color_names++; // current index in color names array
            int pos2 = line.find(" ", pos); // find first space char
            s = line.substr(pos, pos2 - pos); // extract R value
            color_names[nb_color_names].R = std::stoi(s); // R in array
            pos = pos2 + 1; // next char
            pos2 = line.find(" ", pos); // find second space char
            s = line.substr(pos, pos2 - pos); // extract G value
            color_names[nb_color_names].G = std::stoi(s); // G in array
            pos = pos2 + 1; // next char
            pos2 = line.find(" ", pos); // find third space char
            s = line.substr(pos, pos2 - pos); // extract B value
            color_names[nb_color_names].B = std::stoi(s); // B in array
            s = line.substr(pos2 + 1, line.length() - pos2); // color name is at the end of the line
            color_names[nb_color_names].name = QString::fromStdString(s); // color name in array
        }

        names.close(); // close text file
    }
    else {
        QMessageBox::critical(this, "Colors text file not found!", "You forgot to put 'color-names.txt' in the same folder as the executable! This tool will crash if you click on a color...");
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
    Compute();
}

void MainWindow::on_button_3d_reset_clicked() // recenter position zoom for 3D scene
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
        ui->openGLWidget_3d->SetZRotation(120);
    }
    if (ui->openGLWidget_3d->color_space == "CIE XYZ") {
        ui->openGLWidget_3d->SetXRotation(198);
        ui->openGLWidget_3d->SetYRotation(205);
        ui->openGLWidget_3d->SetZRotation(270);
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
    ui->button_save_3d->setGeometry(3, 3, ui->button_save_3d->width(), ui->button_save_3d->height()); // set it to upper-left corner

    QRect screenSize = qApp->desktop()->availableGeometry(qApp->desktop()->primaryScreen()); // get screen size in which app is run
    int newW = screenSize.width(); // set 3D widget size to screen
    int newH = screenSize.height();

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    show();
    ui->openGLWidget_3d->move(QPoint(0, 0)); // move widget to upper-left position in window
    ui->openGLWidget_3d->resize(QSize(newW, newH)); // resize openGL widget
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

///////////////////  Keyboard events  //////////////////////

void MainWindow::keyPressEvent(QKeyEvent *keyEvent) // keyboard events
{
    if ((keyEvent->key() == Qt::Key_Escape) & (ui->checkBox_3d_fullscreen->isChecked())) { // only way to get out of fullscreen view
        ui->button_save_3d->setGeometry(saveXButtonSave,saveYButtonSave, ui->button_save_3d->width(), ui->button_save_3d->height()); // move back save image button
        this->setWindowFlags((((windowFlags() | Qt::CustomizeWindowHint)
                                & ~Qt::WindowCloseButtonHint) | Qt::WindowMinMaxButtonsHint));
        show();
        ui->openGLWidget_3d->move(QPoint(saveXOpenGL, saveYOpenGL)); // restore openGL widget to its previous position
        ui->openGLWidget_3d->resize(QSize(saveWidthOpenGL, saveHeightOpenGL)); // ... and size
        ui->checkBox_3d_fullscreen->setChecked(false); // uncheck fullscreen button
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

    bool color_found = false; // valid color found ?
    Vec3b color; // BGR values of picked color

    if (ui->label_palette->underMouse()) { // mouse over palette ?
        mouse_pos = ui->label_palette->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!palette.empty())) { // mouse left button clicked
            int x = round(float(palette_width) * mouse_pos.x() / ui->label_palette->width()); // x position in palette
            color = palette.at<Vec3b>(0, x); // pick color in palette
            color_found = true; // found !
        }
    }

    if (ui->label_quantized->underMouse()) { // mouse over quantized image ?
        mouse_pos = ui->label_quantized->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!quantized.empty())) { // mouse left button clicked
            const QPixmap* q = ui->label_quantized->pixmap(); // stored reduced quantized image in GUI
            Mat img = QPixmap2Mat(*q); // convert it to openCV Mat
            color = img.at<Vec3b>(mouse_pos.y() - (ui->label_quantized->height() - img.rows) / 2,
                                  mouse_pos.x() - (ui->label_quantized->width() - img.cols) / 2); // pick color at x,y
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
        QString hex = QString(" %1").arg(((R & 0xff) << 16) + ((G & 0xff) << 8) + (B & 0xff), 6, 16, QChar('0')); // compute hexa RGB value
        ui->label_color_hex->setText("#" + hex.toUpper()); // show it

        // find color in palette
        bool found = false; // picked color found in palette ?
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // search in palette
            if ((round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f) == R) and (round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f) == G) and (round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f) == B)) { // identical RGB values found
                QString value = QString::number(ui->openGLWidget_3d->palettes[n].percentage * 100, 'f', 2) + " %"; // picked color percentage in quantized image
                ui->label_color_percentage->setText(value); // display percentage
                found = true; // color found in palette
                break; // get out of loop
            }
        }
        if (!found) // picked color not found in palette
            ui->label_color_percentage->setText(""); // no percentage displayed

        // find nearest color name by euclidian distance
        found = false;
        int distance = 1000000; // distance formula can never reach this high
        int index; // to keep nearest color index in color names table

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
                    index = n; // keep index
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
    size_t period_idx = basefile.rfind('.');
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

    ui->label_filename->setText(filename); // display file name in ui

    thumbnail = ResizeImageAspectRatio(image, cv::Size(ui->label_thumbnail->width(),ui->label_thumbnail->height())); // create thumbnail
    quantized.release(); // no quantized image yet
    palette.release(); // no palette image yet
    //classification.release();

    ShowResults(); // show images in GUI
    ui->openGLWidget_3d->nb_palettes = -1; // no palette to show
    ui->openGLWidget_3d->update(); // show 3D view

    ui->timer->display("0"); // reset timer
}

void MainWindow::on_button_save_clicked() // save dominant colors results
{
    /*if (!computed) { // nothing loaded yet = get out
        QMessageBox::critical(this, "Nothing to do!", "You have to load then compute before saving the images");
        return;
    }*/

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
        setlocale(LC_ALL, "C"); // for numeric separator=dot instead of comma when using std functions
        save << "RGB.R byte;RGB.G byte;RGB.G byte;RGB hexadecimal;HSV.H °;HSV.S %;HSV.V %;HSV.C %;HSL.H °;HSL.S %;HSL.L %;HSL.C %;XYZ.X %;XYZ.Y %;XYZ.Z %;L*A*B*.L %;L*A*B*.a signed byte;L*A*B*.b signed byte;HWB.H °;HWB.W %;HWB.B %;Hunter LAB.L %;Hunter LAB.a signed byte;Hunter LAB.b signed byte;LCH.L %;LCH.C %;LCH.H °;Percentage\n"; // CSV header
        for (int n = 0; n < ui->openGLWidget_3d->nb_palettes; n++) { // read palette
            // RGB
            save << round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f) << ";";
            save << round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f) << ";";
            save << round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f) << ";";
            // RGB hexa
            QString hex = QString(" %1").arg(((int(round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f)) & 0xff) << 16)
                                            + ((int(round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f)) & 0xff) << 8)
                                            + (int(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f)) & 0xff), 6, 16,
                                            QChar('0')); // hexa from RGB value
            save << "#" << hex.toUpper().toUtf8().constData() << ";";
            // HSV+C
            save << ui->openGLWidget_3d->palettes[n].HSV.H * 360.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.S * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.V * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSV.C * 100.0f << ";";
            // HSL+C
            save << ui->openGLWidget_3d->palettes[n].HSL.H * 360.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.S * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.L * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HSL.C * 100.0f << ";";
            // XYZ
            save << ui->openGLWidget_3d->palettes[n].XYZ.X * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].XYZ.Y * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].XYZ.Z * 100.0f << ";";
            // LAB
            save << ui->openGLWidget_3d->palettes[n].CIELAB.L * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].CIELAB.A * 127.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].CIELAB.B * 127.0f << ";";
            // HWB
            save << ui->openGLWidget_3d->palettes[n].HWB.H * 360.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HWB.W * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HWB.B * 100.0f << ";";
            // Hunter LAB
            save << ui->openGLWidget_3d->palettes[n].HLAB.L * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HLAB.A * 127.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].HLAB.B * 127.0f << ";";
            // CIE LCH
            save << ui->openGLWidget_3d->palettes[n].LCH.L * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].LCH.C * 100.0f << ";";
            save << ui->openGLWidget_3d->palettes[n].LCH.H * 360.0f << ";";
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
    qApp->processEvents();

    ui->openGLWidget_3d->nb_palettes = ui->spinBox_nb_palettes->value(); // how many dominant colors to compute

    if (ui->radioButton_eigenvectors->isChecked()) { // eigen method
        std::vector<cv::Vec3b> palette_vec = DominantColorsEigen(image, ui->openGLWidget_3d->nb_palettes, &quantized); // get dominant palette, palette image and quantized image

        for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // store palette values in structured array
        {
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = palette_vec[n][2] / 255.0f;
            ui->openGLWidget_3d->palettes[n].RGB.G = palette_vec[n][1] / 255.0f;
            ui->openGLWidget_3d->palettes[n].RGB.B = palette_vec[n][0] / 255.0f;
        }
        ui->openGLWidget_3d->ConvertPalette(); // convert RGB to other values
    }
    else if (ui->radioButton_k_means->isChecked()) { // K-means method
        cv::Mat1f colors; // to store palette from K-means
        quantized = DominantColorsKMeans(image, ui->spinBox_nb_palettes->value(), &colors); // get quantized image and palette

        for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // store palette in structured array
        {
            // RGB
            ui->openGLWidget_3d->palettes[n].RGB.R = colors(n, 2) / 255.0f;
            ui->openGLWidget_3d->palettes[n].RGB.G = colors(n, 1) / 255.0f;
            ui->openGLWidget_3d->palettes[n].RGB.B = colors(n, 0) / 255.0f;
        }
        ui->openGLWidget_3d->ConvertPalette(); // convert RGB to other values

        //classification.release();
    }

    // count occurences of colors in quantized image
    int total = quantized.rows * quantized.cols; // size of quantized image in pixels
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) { // for each color in palette
        Mat1b mask; // current color mask
        inRange(quantized, Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f)),
                           Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f),
                                 round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f)),
                           mask); // create mask for current color
        ui->openGLWidget_3d->palettes[n].count = cv::countNonZero(mask); // count pixels in mask
        ui->openGLWidget_3d->palettes[n].percentage = float(ui->openGLWidget_3d->palettes[n].count) / float(total); // color use percentage
    }

    SortPalettes(); // sort and create palette image

    ui->openGLWidget_3d->update(); // show 3D view
    ShowResults(); // show result images

    ShowTimer(); // show elapsed time
    QApplication::restoreOverrideCursor(); // Restore cursor

    computed = true; // success !
}

void MainWindow::ShowResults() // display result images in GUI
{
    if (!thumbnail.empty())
        ui->label_thumbnail->setPixmap(Mat2QPixmap(thumbnail)); // thumbnail
        else ui->label_thumbnail->setPixmap(QPixmap());
    if (!quantized.empty())
        ui->label_quantized->setPixmap(Mat2QPixmapResized(quantized, ui->label_quantized->width(), ui->label_quantized->height(), true)); // quantized image
        else ui->label_quantized->setPixmap(QPixmap());
    if (!palette.empty())
        ui->label_palette->setPixmap(Mat2QPixmapResized(palette, ui->label_palette->width(), ui->label_palette->height(), true)); // palette image
        else ui->label_palette->setPixmap(QPixmap());
    /*if (!classification.empty())
        ui->label_classification->setPixmap(Mat2QPixmapResized(classification, ui->label_classification->width(), ui->label_classification->height(), true)); // classification image
        else ui->label_classification->setPixmap(QPixmap());*/
}

void MainWindow::ShowTimer() // show elapsed time
{
    int milliseconds = int(timer.elapsed() %1000); // milliseconds
    int seconds = int(timer.elapsed()/1000 %60); // seconds
    ui->timer->display(QString(" %1").arg(seconds, 3, 10, QChar('0'))
                      + "."
                      + QString(" %1").arg(milliseconds, 3, 10, QChar('0'))); // show elapsed time
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

    // create palette image - doesn't work well over 200-250 values
    palette = Mat::zeros(cv::Size(palette_width, palette_height), CV_8UC3); // create blank image
    float offset = 0; // current x position in palette
    for (int n = 0;n < ui->openGLWidget_3d->nb_palettes; n++) // for each color in palette
    {
        cv::rectangle(palette, Rect(round(offset), 0,
                                    round(ui->openGLWidget_3d->palettes[n].percentage * float(palette_width)), palette_height),
                                    Vec3b(round(ui->openGLWidget_3d->palettes[n].RGB.B * 255.0f),
                                          round(ui->openGLWidget_3d->palettes[n].RGB.G * 255.0f),
                                          round(ui->openGLWidget_3d->palettes[n].RGB.R * 255.0f)), -1); // rectangle of current color
        offset += round(ui->openGLWidget_3d->palettes[n].percentage * float(palette_width)); // next x position in palette
    }

    ShowResults();
}
