#include "about.h"
#include "ui_about.h"

//RJG - so can access MainWin
#include "mainwindow.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    setWindowTitle("About");
    QFont font;
    font.setWeight(QFont::Bold);

    ui->label_2->setFont(font);
    ui->label_2->setText(MainWin->windowTitle());
    ui->label_2->setAlignment(Qt::AlignCenter);

    QPixmap picture(":/img.png");
    ui->label_3->setPixmap(picture);
    ui->label_3->setAlignment(Qt::AlignCenter);

    ui->label->setWordWrap(true);
    ui->label->setText("This version of EvoSim was compiled on the date shown above. It was coded by:"
                       "\n\nMark Sutton (m.sutton@imperial.ac.uk)\nRussell Garwood (russell.garwood@manchester.ac.uk)\nAlan Spencer (a.spencer09@imperial.ac.uk)"
                       "\n\nBug reports are appreciated, and comments, suggestions, and feature requests are welcome.");
    ui->label->setAlignment(Qt::AlignJustify);

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
}

About::~About()
{
    delete ui;
}
