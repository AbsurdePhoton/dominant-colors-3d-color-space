/*#-------------------------------------------------
#
#    Qt file dialog widget with image preview
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#               v1.0 - 2021/04/26
#
#-------------------------------------------------*/

#include "file-dialog.h"
#include <QLabel>
#include <QGridLayout>

PreviewFileDialog::PreviewFileDialog(   QWidget* parent,
                                        const QString & caption, // windows caption
                                        const QString & directory, // directory to start in
                                        const QString & filter, // files filter(s)
                                        const bool &save // save=true for saving a file
                                        ) :
    QFileDialog(parent, caption, directory, filter)
{
    canceled = false;

    this->setOption(QFileDialog::DontUseNativeDialog, true); // useful under Windows
    if (save) { // save file dialog ?
        this->setAcceptMode(QFileDialog::AcceptSave);
        this->setFileMode(QFileDialog::AnyFile);
    }
    else { // load file dialog
        this->setAcceptMode(QFileDialog::AcceptOpen);
        this->setFileMode(QFileDialog::ExistingFile);
    }

    this->setGeometry(QRect(460, 200, 1200, 600)); // window size and position
    setObjectName("PreviewFileDialog"); // object name
    QVBoxLayout* box = new QVBoxLayout(); // create a layout

    mpPreview = new QLabel(tr("Preview"), this); // create QLabel for the thumbnail
    mpPreview->setAlignment(Qt::AlignCenter); // centered
    mpPreview->setObjectName("labelPreview"); // label name
    mpPreview->setMinimumSize(384, 384); // minimum size
    box->addWidget(mpPreview); // add it to the window
    //box->addStretch();

    QGridLayout *layout = static_cast<QGridLayout*>(this->layout()); // add to QFileDialog layout

    QList<QPair<QLayoutItem*, QList<int>>> movedItems; // to organize layout view

    for(int i = 0; i < layout->count(); i++) { // for all objects in layout
        int row, column, rowSpan, columnSpan;

        layout->getItemPosition(i, &row, &column, &rowSpan, &columnSpan); // item position

        if (row > 2) { // not on first row : it is NOT the files list
          QList<int> list;
          list << row << column << rowSpan << columnSpan;
          movedItems << qMakePair(layout->takeAt(i), list);
          i--;
        }
    }

    for(int i = 0; i < movedItems.count(); i++) { // parse items to be moved
        layout->addItem(movedItems[i].first,
                        movedItems[i].second[0],
                        movedItems[i].second[1],
                        movedItems[i].second[2],
                        movedItems[i].second[3]
                       );
    }

    layout->addItem(box, 1, 3, 1, 1); // add the box

    // signals
    connect(this, SIGNAL(currentChanged(const QString&)), this, SLOT(OnCurrentChanged(const QString&))); // when a file is selected
    connect(this, SIGNAL(rejected()), this, SLOT(OnRejected())); // when Cancel button is pressed

    this->exec();
}

QString PreviewFileDialog::GetSelectedFile() // return selected file path
{
    if ((canceled) or (this->selectedFiles().size() == 0)) // no file selected or canceled ?
        return "";
    else // a file was chosen
        return this->selectedFiles().first();
}

void PreviewFileDialog::OnCurrentChanged(const QString & path) // show thumbnail when a file is selected
{
    selectedFilename = path; // save file path

    QPixmap pixmap = QPixmap(path); // create image from path

    if (pixmap.isNull()) { // not an image ?
        mpPreview->setText("Not an image");
    }
    else { // it is an image
        mpPreview->setPixmap(pixmap.scaled(mpPreview->width(), mpPreview->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); // show thumbnail
	}
}

void PreviewFileDialog::OnRejected() // if Cancel is clicked filename is empty
{
    canceled = true; // no file selected
}
