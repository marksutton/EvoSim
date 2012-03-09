#include <stdlib.h>
#include "settings.h"
#include "simmanager.h"
#include <QDebug>
#include <mainwindow.h>
#include <QImage>
#include <QVector>



SettingsImpl::SettingsImpl( QWidget * parent, Qt::WFlags f)
        : QDialog(parent, f)
{
        setupUi(this);

        SpinBoxSettleTolerance->setValue(settleTolerance);
        SpinBoxStartAge->setValue(startAge);
        SpinBoxDispersal->setValue(dispersal);
        SpinBoxFood->setValue(food);
        SpinBoxBreedCost->setValue(breedCost);
        SpinBoxMutationChance->setValue(mutate);
        SpinBoxDifferenceBreed->setValue(maxDiff);
        SpinBoxBreedThreshold->setValue(breedThreshold);
        SpinBoxSlots->setValue(slotsPerSq);
        SpinBoxGridX->setValue(gridX);
        SpinBoxGridY->setValue(gridY);
        SpinBoxFitnessTarget->setValue(target);
        SpinBoxEnvChangeRate->setValue(envchangerate);

        RedoImages=false;        
}

void SettingsImpl::on_buttonBox_accepted()
{

        if (slotsPerSq != SpinBoxSlots->value() || gridX != SpinBoxGridX->value() || gridY != SpinBoxGridY->value()) RedoImages=true;
        settleTolerance= SpinBoxSettleTolerance->value();
        startAge= SpinBoxStartAge->value();
        dispersal= SpinBoxDispersal->value();
        food= SpinBoxFood->value();
        breedCost= SpinBoxBreedCost->value();
        mutate= SpinBoxMutationChance->value();
        maxDiff= SpinBoxDifferenceBreed->value();
        breedThreshold= SpinBoxBreedThreshold->value();
        slotsPerSq= SpinBoxSlots->value();
        gridX= SpinBoxGridX->value();
        gridY= SpinBoxGridY->value();
        target= SpinBoxFitnessTarget->value();
        EnvChangeCounter=envchangerate=SpinBoxEnvChangeRate->value();
}


void SettingsImpl::on_buttonBox_rejected()
{
        close();
}




