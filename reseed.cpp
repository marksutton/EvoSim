#include "reseed.h"
#include "simmanager.h"
#include "ui_reseed.h"

//RJG - so can access MainWin
#include "mainwindow.h"


reseed::reseed(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::reseed)
{
    ui->setupUi(this);

    QString newGenome;
    for (int i=0; i<64; i++)
        //---- RJG - if genome bit is 1, number is > 0; else it's zero.
        //---- RJG - Endian-ness to match genome comparison
        if (tweakers64[63-i] & reseedGenome) newGenome.append("1"); else newGenome.append("0");
    ui->genomeTextEdit->appendPlainText(newGenome);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui->genomeTextEdit->setFont(font);

    ui->CheckBoxReseedSession->setChecked(true);

    int length=MainWin->genoneComparison->access_glist_length();
    if (length>10)length=10;

    if(!length)
        {
        QLabel *label = new QLabel("There are currently no genomes recorded in the Genome Docker.",this);
        ui->genomesLayout->addWidget(label);
        }
    else for (int i=0;i<length;i++)
        {
        QRadioButton *radio = new QRadioButton(this);
        radio->setText(MainWin->genoneComparison->access_genome(i));
        ui->genomesLayout->addWidget(radio);
        connect(radio,SIGNAL(toggled(bool)),this, SLOT(radio_toggled()));
        radios.append(radio);
        }
}

void reseed::on_buttonBox_accepted()
{
    QString newGenome(ui->genomeTextEdit->toPlainText());
    bool error=false;
    for (int i=0; i<64; i++)if (newGenome[i]!='1'&& newGenome[i]!='0')error=true;
    if (newGenome.length()!=64||error==true)QMessageBox::warning(this,"Oops","This doesn't look like a valid genome, and so this is not going to be set. Sorry. Please try again by relaunching reseed.");
    else
        {
        for (int i=0; i<64; i++)
            if (newGenome[63-i]=='1')reseedGenome|=tweakers64[i];
                //RJG - ~ is a bitwise not operator.
                else reseedGenome&=(~tweakers64[i]);

        reseedKnown=ui->CheckBoxReseedSession->isChecked();
        }
}


void reseed::radio_toggled()
{
ui->genomeTextEdit->clear();
for(int i=0;i<radios.length();i++) if(radios[i]->isChecked())ui->genomeTextEdit->appendPlainText(radios[i]->text());
}

void reseed::on_buttonBox_rejected()
{
        close();
}

reseed::~reseed()
{
    delete ui;
}
