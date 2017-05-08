#include "reseed.h"
#include "simmanager.h"
#include "ui_reseed.h"

#include <QMessageBox>

reseed::reseed(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::reseed)
{
    ui->setupUi(this);

    QString newGenome;
    for (int i=0; i<64; i++)
        //---- RJG - if genome bit is 1, number is > 0; else it's zero.
        if (tweakers64[i] & reseedGenome) newGenome.append("1"); else newGenome.append("0");
    ui->genomeTextEdit->appendPlainText(newGenome);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui->genomeTextEdit->setFont(font);

    ui->CheckBoxReseedSession->setChecked(reseedKnown);
    //ui->genomesLayout->



}

void reseed::on_buttonBox_accepted()
{
    QString newGenome(ui->genomeTextEdit->toPlainText());
    if (newGenome.length()!=64)QMessageBox::warning(this,"Oops","This doesn't look like a valid genome, and so this is not going to be set. Sorry. Please try again by relaunching reseed.");
    else
        {
        //RJG - Need to check whether Alan's go other way, i.e. are little endian? Check when docker implemented.
        for (int i=0; i<64; i++)
            if (newGenome[i]=='1')reseedGenome|=tweakers64[i];
                //RJG - ~ is a bitwise not operator.
                else reseedGenome&=(~tweakers64[i]);

       /* QString testGenome;
        for (int i=0; i<64; i++)
            if (tweakers64[i] & reseedGenome) testGenome.append("1"); else testGenome.append("0");
        qDebug()<<testGenome;*/

        reseedKnown=ui->CheckBoxReseedSession->isChecked();
        }
}



void reseed::on_buttonBox_rejected()
{
        close();
}

reseed::~reseed()
{
    delete ui;
}
