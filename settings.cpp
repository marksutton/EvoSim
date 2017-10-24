#include <stdlib.h>
#include "settings.h"
#include "simmanager.h"
#include <QDebug>
#include "mainwindow.h"
#include <QImage>
#include <QVector>

SettingsImpl::SettingsImpl( QWidget * parent)
        : QDialog(parent)
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
        SpinBoxYearsPerIteration->setValue(yearsPerIteration);
        CheckBoxRecalcFitness->setChecked(recalcFitness);
        CheckBoxToroidal->setChecked(toroidal);
        CheckBoxNonSpatial->setChecked(nonspatial);
        CheckBoxBreedWithinSpecies->setChecked(breedspecies);
        CheckBoxBreedWithDifference->setChecked(breeddiff);
        radioSexual->setChecked(sexual);
        radioAsexual->setChecked(asexual);
        radioVariable->setChecked(variableBreed);
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
        yearsPerIteration=SpinBoxYearsPerIteration->value();
        recalcFitness=CheckBoxRecalcFitness->isChecked();
        toroidal=CheckBoxToroidal->isChecked();
        nonspatial=CheckBoxNonSpatial->isChecked();
        breeddiff=CheckBoxBreedWithDifference->isChecked();
        breedspecies=CheckBoxBreedWithinSpecies->isChecked();
        sexual=radioSexual->isChecked();
        asexual=radioAsexual->isChecked();
        variableBreed=radioVariable->isChecked();

}


void SettingsImpl::on_buttonBox_rejected()
{
        close();
}




