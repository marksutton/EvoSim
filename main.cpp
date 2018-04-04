/**
 * @file
 * Main Programme
 *
 * All REVOSIM code is released under the GNU General Public License.
 * See LICENSE.md files in the programme directory.
 *
 * All REVOSIM code is Copyright 2018 by Mark Sutton, Russell Garwood,
 * and Alan R.T. Spencer.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY.
 */

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);
    MainWindow w;
    w.show();
    return a.exec();
}
