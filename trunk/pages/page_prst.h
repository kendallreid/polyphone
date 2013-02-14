/***************************************************************************
**                                                                        **
**  Polyphone, a soundfont editor                                         **
**  Copyright (C) 2013 Davy Triponney                                     **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Davy Triponney                                       **
**  Website/Contact: http://www.polyphone.fr/                             **
**             Date: 01.01.2013                                           **
***************************************************************************/

#ifndef PAGE_PRST_H
#define PAGE_PRST_H

#include <QWidget>
#include "page.h"

namespace Ui
{
    class Page_Prst;
}


class SpinBox; // Déclaration anticipée

class Page_Prst : public PageTable
{
    Q_OBJECT
public:
    explicit Page_Prst(QWidget *parent = 0);
    ~Page_Prst();
    void setModVisible(bool visible);
    void afficher();
    void spinUpDown(int steps, SpinBox *spin);
    void firstAvailablePresetBank(EltID id, int &nBank, int &nPreset);
    void duplication();
public slots:
    void setBank();
    void setPreset();
private:
    Ui::Page_Prst *ui;
    int getDestNumber(int i);
    int getDestIndex(int i);
    static int closestAvailablePreset(EltID id, WORD wBank, WORD wPreset);
    static bool isAvailable(EltID id, WORD wBank, WORD wPreset);
    // Outils
    void duplication(EltID id);
};

// Classe TableWidget pour presets
class TableWidgetPrst : public TableWidget
{
    Q_OBJECT
public:
    // Constructeur
    TableWidgetPrst(QWidget *parent = 0);
    ~TableWidgetPrst();
    // Association champ - ligne
    Champ getChamp(int row);
    int getRow(WORD champ);
};


// SpinBox
class SpinBox : public QSpinBox
{
    Q_OBJECT
public:
    // Constructeur
    SpinBox(QWidget *parent = 0) : QSpinBox(parent) {}
    // Initialisation du sf2
    void init(Page_Prst *page) {this->page = page;}
public slots:
    virtual void stepBy(int steps) {this->page->spinUpDown(steps, this);}
private:
    Page_Prst *page;
};

#endif // PAGE_PRST_H
