#include "editor_old.h"
#include "ui_editor_old.h"
#include "sound.h"
#include "dialog_rename.h"
#include "dialog_exportlist.h"
#include "sfz/conversion_sfz.h"
#include "dialog_export.h"
#include "duplicator.h"
#include "sfz/import_sfz.h"
#include "dialogchangelog.h"
#include "contextmanager.h"
#include "modalprogressdialog.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QUrl>
#include <QDate>
#include <QLibrary>
#include <QDesktopWidget>
#include <QDesktopServices>

// Constructeurs, destructeurs
MainWindowOld::MainWindowOld(QWidget *parent) : QMainWindow(parent, Qt::Window | Qt::WindowCloseButtonHint |
                                                            Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint |
                                                            Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
                                                            Qt::CustomizeWindowHint
                                                            #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                                            | Qt::WindowFullscreenButtonHint
                                                            #endif
                                                            ),
    ui(new Ui::MainWindowOld),
    about(this),
    dialList(this),
    actionKeyboard(NULL),
    _progressDialog(NULL)
{
    ui->setupUi(this);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    ui->editSearch->setPlaceholderText(trUtf8("Rechercher..."));
#endif
#ifdef Q_OS_MAC
    ui->verticalLayout_3->setSpacing(4);
#endif

    ui->actionPlein_cran->setChecked(this->windowState() & Qt::WindowFullScreen);

    // Initialisation de l'objet pile sf2
    this->sf2 = SoundfontManager::getInstance();

    // Création des pages
    page_sf2 = new PageSf2();
    page_smpl = new PageSmpl();
    page_inst = new PageInst();
    page_prst = new PagePrst();
    _pageOverviewSmpl = new PageOverviewSmpl();
    _pageOverviewInst = new PageOverviewInst();
    _pageOverviewPrst = new PageOverviewPrst();
    ui->stackedWidget->addWidget(page_sf2);
    ui->stackedWidget->addWidget(page_smpl);
    ui->stackedWidget->addWidget(page_inst);
    ui->stackedWidget->addWidget(page_prst);
    ui->stackedWidget->addWidget(_pageOverviewSmpl);
    ui->stackedWidget->addWidget(_pageOverviewInst);
    ui->stackedWidget->addWidget(_pageOverviewPrst);

    // Initialisation dialog liste (pointeur vers les sf2 et mainWindow)
    this->dialList.init(this, this->sf2);

    // Fichiers récents
    //updateFavoriteFiles();

    // Affichage logo logiciel
    ui->stackedWidget->setCurrentWidget(ui->page_Soft);

    // Préférences d'affichage
    if (!ContextManager::configuration()->getValue(ConfManager::SECTION_DISPLAY, "tool_bar", true).toBool())
    {
        ui->actionBarre_d_outils->setChecked(false);
        ui->toolBar->setVisible(false);
    }
    if (!ContextManager::configuration()->getValue(ConfManager::SECTION_DISPLAY, "section_modulateur", true).toBool())
    {
        ui->actionSection_modulateurs->setChecked(false);
        this->page_inst->setModVisible(false);
        this->page_prst->setModVisible(false);
    }

    // Initialisation objet Sound
    Sound::setParent(this);

    // Bug QT: restauration de la largeur d'un QDockWidget si fenêtre maximisée
    int dockWidth = ContextManager::configuration()->getValue(ConfManager::SECTION_DISPLAY, "dock_width", 150).toInt();
    if (ui->dockWidget->width() < dockWidth)
        ui->dockWidget->setMinimumWidth(dockWidth);
    else
        ui->dockWidget->setMaximumWidth(dockWidth);

    ui->actionA_propos->setMenuRole(QAction::AboutRole);
    ui->actionPr_f_rences->setMenuRole(QAction::PreferencesRole);

    connect(&_futureWatcher, SIGNAL (finished()), this, SLOT (futureFinished()));
}

MainWindowOld::~MainWindowOld()
{
    SoundfontManager::kill();
    delete this->page_inst;
    delete this->page_prst;
    delete this->page_sf2;
    delete this->page_smpl;
    delete _pageOverviewSmpl;
    delete _pageOverviewInst;
    delete _pageOverviewPrst;
    delete ui;
}

void MainWindowOld::spaceKeyPressedInTree()
{
    if (ui->stackedWidget->currentWidget() == page_smpl)
        page_smpl->pushPlayPause();
}

void MainWindowOld::supprimerElt()
{
    // Suppression d'un ou plusieurs éléments dans l'arborescence
    int nb = 0;
    if (nb <= 0)
        return;

    QList<EltID> listID;

    // 1er élément à supprimer
    EltID elementToSelect;// = ui->tree->getElementToSelectAfterDeletion();

    int message = 1;
    foreach (EltID id, listID)
        sf2->remove(id, &message);

    if (message % 2 == 0)
        QMessageBox::warning(this, trUtf8("Attention"),
                             trUtf8("Impossible de supprimer un échantillon s'il est utilisé par un instrument."));
    if (message % 3 == 0)
        QMessageBox::warning(this, trUtf8("Attention"),
                             trUtf8("Impossible de supprimer un instrument s'il est utilisé par un preset."));

//    if (message == 1 && elementToSelect.typeElement != elementUnknown)
//        ui->tree->select(elementToSelect, true);

    //updateActions();
    //updateDo();
}

int MainWindowOld::sauvegarder(int indexSf2, bool saveAs)
{
    // Remove the focus from the interface
    this->setFocus();
    if (!sf2->isEdited(indexSf2) && !saveAs)
        return 0;

    EltID id(elementSf2, indexSf2, 0, 0, 0);
    // Avertissement si enregistrement dans une résolution inférieure
    if (sf2->get(id, champ_wBpsSave).wValue < sf2->get(id, champ_wBpsInit).wValue)
    {
        int ret = QMessageBox::Save;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(trUtf8("<b>Perte de résolution ") +
                       QString::number(sf2->get(id, champ_wBpsInit).wValue) + " &#8658; " +
                       QString::number(sf2->get(id, champ_wBpsSave).wValue) +
                       trUtf8(" bits</b>"));
        msgBox.setInformativeText(trUtf8("La qualité des samples sera abaissée suite à cette opération. Continuer ?"));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Save);
        msgBox.button(QMessageBox::Save)->setText(trUtf8("&Oui"));
        msgBox.button(QMessageBox::Cancel)->setText(trUtf8("&Non"));
        msgBox.setDefaultButton(QMessageBox::Cancel);
        ret = msgBox.exec();
        if (ret == QMessageBox::Cancel)
            return 1;
    }

    // Compte du nombre de générateurs utilisés
    int unusedSmpl, unusedInst, usedSmpl, usedInst, usedPrst, instGen, prstGen;
    //this->page_sf2->countUnused(unusedSmpl, unusedInst, usedSmpl, usedInst, usedPrst, instGen, prstGen);
    if ((instGen > 65536 || prstGen > 65536) && ContextManager::configuration()->getValue(ConfManager::SECTION_WARNINGS,
                                                                                          "to_many_generators", true).toBool())
    {
        int ret = QMessageBox::Save;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        QString texte;
        if (instGen > 65536 && prstGen > 65536)
            texte = trUtf8("<b>Trop de paramètres dans les instruments et les presets.</b>");
        else if (instGen > 65536)
            texte = trUtf8("<b>Trop de paramètres dans les instruments.</b>");
        else
            texte = trUtf8("<b>Trop de paramètres dans les presets.</b>");
        msgBox.setText(texte);
        msgBox.setInformativeText(trUtf8("Certains synthétiseurs ne prennent pas en compte "
                                         "les paramètres au delà du 65536ème.\n"
                                         "Diviser le fichier en plusieurs sf2 peut résoudre le problème."));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Save | QMessageBox::YesAll);
        msgBox.button(QMessageBox::Save)->setText(trUtf8("&Sauvegarder"));
        msgBox.button(QMessageBox::YesAll)->setText(trUtf8("Sauvegarder, &désactiver ce message"));
        msgBox.button(QMessageBox::Cancel)->setText(trUtf8("&Annuler"));
        msgBox.setDefaultButton(QMessageBox::Cancel);
        ret = msgBox.exec();
        if (ret == QMessageBox::Cancel)
            return 1;
        if (ret == QMessageBox::YesAll)
            ContextManager::configuration()->setValue(ConfManager::SECTION_WARNINGS, "to_many_generators", false);
    }

    /*QString fileName;
    if (saveAs || sf2->getQstr(id, champ_filename) == "")
    {
        if (sf2->getQstr(id, champ_filename) == "")
        {
            fileName = ContextManager::recentFile()->getLastDirectory(RecentFileManager::FILE_TYPE_SF2) +
                    "/" + sf2->getQstr(id, champ_name) + ".sf2";
            fileName = QFileDialog::getSaveFileName(this, trUtf8("Sauvegarder une soundfont"), fileName, trUtf8("Fichier .sf2 (*.sf2)"));
        }
        else
            fileName = QFileDialog::getSaveFileName(this, trUtf8("Sauvegarder une soundfont"),
                                                    sf2->getQstr(id, champ_filename), trUtf8("Fichier .sf2 (*.sf2)"));
        if (fileName.isNull())
            return 1;
    }
    else
        fileName = sf2->getQstr(id, champ_filename);
    switch (this->sf2->save(indexSf2, fileName))
    {
    case 0:
        // sauvegarde ok
        updateDo();
        ContextManager::recentFile()->addRecentFile(RecentFileManager::FILE_TYPE_SF2, fileName);
        updateFavoriteFiles();
        //        if (ui->stackedWidget->currentWidget() == this->page_sf2)
        //            this->page_sf2->preparePage();
        return 0;
        break;
    case 1:
        QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Extension inconnue."));
        break;
    case 2:
        QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Fichier déjà ouvert, impossible de sauvegarder."));
        break;
    case 3:
        QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Impossible d'enregistrer le fichier."));
        break;
    }*/
    return 1;
}

// Fenetres / affichage
void MainWindowOld::showAbout()
{
    about.show();
}

void MainWindowOld::AfficherBarreOutils()
{
    bool display = ui->actionBarre_d_outils->isChecked();
    ContextManager::configuration()->setValue(ConfManager::SECTION_DISPLAY, "tool_bar", display);
    ui->toolBar->setVisible(display);
}

void MainWindowOld::afficherSectionModulateurs()
{
    bool display = ui->actionSection_modulateurs->isChecked();
    ContextManager::configuration()->setValue(ConfManager::SECTION_DISPLAY, "section_modulateur", display);
    this->page_inst->setModVisible(display);
    this->page_prst->setModVisible(display);
}

void MainWindowOld::onPleinEcranTriggered()
{
    this->setWindowState(this->windowState() ^ Qt::WindowFullScreen);
}

// Modifications
void MainWindowOld::renommer()
{
    bool ok;
    // Nb d'éléments sélectionnés
    int nb = 0;
    EltID ID;
    ElementType type = ID.typeElement;

    if (type != elementSf2 && type != elementSmpl && type != elementInst && type != elementPrst)
        return;

    if (nb > 1)
    {
        DialogRename * dial = new DialogRename(type == elementSmpl, this);
        dial->setAttribute(Qt::WA_DeleteOnClose);
        connect(dial, SIGNAL(updateNames(int, QString, QString, int, int)),
                this, SLOT(renommerEnMasse(int, QString, QString, int, int)));
        dial->show();
    }
    else
    {
        QString msg;
        if (type == elementSmpl) msg = trUtf8("Nom de l'échantillon (max 20 caractères) :");
        else if (type == elementInst) msg = trUtf8("Nom de l'instrument (max 20 caractères) :");
        else if (type == elementPrst) msg = trUtf8("Nom du preset (max 20 caractères) :");
        else if (type == elementSf2) msg = trUtf8("Nom du SF2 (max 255 caractères) :");
        QString text = QInputDialog::getText(this, trUtf8("Question"), msg, QLineEdit::Normal, sf2->getQstr(ID, champ_name), &ok);
        if (ok && !text.isEmpty())
        {
            sf2->set(ID, champ_name, text);
        }
    }
}
void MainWindowOld::renommerEnMasse(int renameType, QString text1, QString text2, int val1, int val2)
{
    if (renameType == 4)
    {
        if (val1 == val2)
            return;
    }
    else
    {
        if (text1.isEmpty())
            return;
    }

    QList<EltID> listID;;

    for (int i = 0; i < listID.size(); i++)
    {
        EltID ID = listID.at(i);

        // Détermination du nom
        QString newName = sf2->getQstr(ID, champ_name);
        switch (renameType)
        {
        case 0:{
            // Remplacement avec note en suffixe
            QString suffix = " " + ContextManager::keyName()->getKeyName(sf2->get(ID, champ_byOriginalPitch).bValue, false, true);
            SFSampleLink pos = sf2->get(ID, champ_sfSampleType).sfLinkValue;
            if (pos == rightSample || pos == RomRightSample)
                suffix += 'R';
            else if (pos == leftSample || pos == RomLeftSample)
                suffix += 'L';

            newName = text1.left(20 - suffix.size()) + suffix;
        }break;
        case 1:
            // Remplacement avec index en suffixe
            if ((i+1) % 100 < 10)
                newName = text1.left(17) + "-0" + QString::number((i+1) % 100);
            else
                newName = text1.left(17) + "-" + QString::number((i+1) % 100);
            break;
        case 2:
            // Remplacement d'une chaîne de caractère
            newName.replace(text1, text2, Qt::CaseInsensitive);
            break;
        case 3:
            // Insertion d'une chaîne de caractère
            if (val1 > newName.size())
                val1 = newName.size();
            newName.insert(val1, text1);
            break;
        case 4:
            // Suppression d'une étendue
            if (val2 > val1)
                newName.remove(val1, val2 - val1);
            else
                newName.remove(val2, val1 - val2);
            break;
        }

        newName = newName.left(20);

        if (sf2->getQstr(ID, champ_name).compare(newName, Qt::CaseInsensitive))
            sf2->set(ID, champ_name, newName);
    }
}

void MainWindowOld::dragAndDrop(EltID idDest, QList<EltID> idSources)
{
    // prepareNewActions() et updateDo() faits à l'extérieur
    Duplicator duplicator(this->sf2, this->sf2, this);
    for (int i = 0; i < idSources.size(); i++)
        duplicator.copy(idSources.at(i), idDest);
    //updateActions();
}

void MainWindowOld::importerSmpl()
{
    // Other allowed format?
    QString ext = "";
    typedef QString (*MyPrototype)();
    MyPrototype myFunction = (MyPrototype) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                             "getExtensionFilter");
    if (myFunction)
        ext = myFunction();

    // Display dialog
    QStringList strList = QFileDialog::getOpenFileNames(this, trUtf8("Importer un fichier audio"),
                                                        ContextManager::recentFile()->getLastDirectory(RecentFileManager::FILE_TYPE_SAMPLE),
                                                        trUtf8("Fichier .wav") + " (*.wav)" + ext);

    if (strList.count() == 0)
        return;

    EltID id;
    int replace = 0;
    while (!strList.isEmpty())
    {
        //ui->tree->clearSelection();
        //this->dragAndDrop(strList.takeAt(0), id, &replace);
    }
}

void MainWindowOld::importerSmpl(QString path, EltID id, int * replace)
{
    id.typeElement = elementSmpl;
    QString qStr, nom;

    qStr = path;
    ContextManager::recentFile()->addRecentFile(RecentFileManager::FILE_TYPE_SAMPLE, qStr);
    QFileInfo qFileInfo = qStr;

    // Récupération des informations d'un sample
    Sound * son = new Sound(qStr);
    int nChannels = son->get(champ_wChannels);

    // Ajout d'un sample
    AttributeValue val;
    nom = qFileInfo.completeBaseName();

    // Remplacement ?
    int indexL = -1;
    int indexR = -1;
    QString qStr3 = "";
    if (*replace != -1)
    {
        foreach (int j, this->sf2->getSiblings(id))
        {
            id.indexElt = j;
            if (nChannels == 2)
            {
                if (this->sf2->getQstr(id, champ_name).compare(nom.left(19).append("L")) == 0)
                {
                    indexL = j;
                    qStr3 = trUtf8("L'échantillon « ") + nom.left(19).toUtf8() +
                            trUtf8("L » existe déjà.<br />Que faire ?");
                }
                else if (this->sf2->getQstr(id, champ_name).compare(nom.left(19).append("R")) == 0)
                {
                    indexR = j;
                    qStr3 = trUtf8("L'échantillon « ") + nom.left(19).toUtf8() +
                            trUtf8("R » existe déjà.<br />Que faire ?");
                }
            }
            else
            {
                if (this->sf2->getQstr(id, champ_name).compare(nom.left(20)) == 0)
                {
                    indexL = j;
                    qStr3 = trUtf8("L'échantillon « ") + nom.left(20).toUtf8() +
                            trUtf8(" » existe déjà.<br />Que faire ?");
                }
            }
        }
        if (*replace != 2 && *replace != 4 && (indexL != -1 || indexR != -1))
        {
            // Remplacement ?
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(qStr3);
            msgBox.setInformativeText("");
            msgBox.setStandardButtons(QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::SaveAll | QMessageBox::Save |
                                      QMessageBox::NoAll | QMessageBox::No);
            msgBox.button(QMessageBox::Yes)->setText(trUtf8("&Remplacer"));
            msgBox.button(QMessageBox::YesAll)->setText(trUtf8("R&emplacer tout"));
            msgBox.button(QMessageBox::Save)->setText(trUtf8("&Dupliquer"));
            msgBox.button(QMessageBox::SaveAll)->setText(trUtf8("D&upliquer tout"));
            msgBox.button(QMessageBox::No)->setText(trUtf8("&Ignorer"));
            msgBox.button(QMessageBox::NoAll)->setText(trUtf8("I&gnorer tout"));
            msgBox.setDefaultButton(QMessageBox::YesAll);
            switch (msgBox.exec())
            {
            case QMessageBox::NoAll:    *replace =  4; break;
            case QMessageBox::No:       *replace =  3; break;
            case QMessageBox::YesAll:   *replace =  2; break;
            case QMessageBox::Yes:      *replace =  1; break;
            case QMessageBox::Save:     *replace =  0; break;
            case QMessageBox::SaveAll:  *replace = -1; break;
            }
        }
    }

    // Ajustement du nom, s'il est déjà utilisé
    if (*replace == 0 || *replace == -1)
    {
        QStringList listSampleName;
        foreach (int j, this->sf2->getSiblings(id))
        {
            id.indexElt = j;
            listSampleName << this->sf2->getQstr(id, champ_name);
        }

        int suffixNumber = 0;
        if (nChannels == 1)
        {
            while (listSampleName.contains(getName(nom, 20, suffixNumber), Qt::CaseInsensitive) && suffixNumber < 100)
            {
                suffixNumber++;
            }
            nom = getName(nom, 20, suffixNumber);
        }
        else
        {
            while ((listSampleName.contains(getName(nom, 19, suffixNumber) + "L", Qt::CaseInsensitive) ||
                    listSampleName.contains(getName(nom, 19, suffixNumber) + "R", Qt::CaseInsensitive)) &&
                   suffixNumber < 100)
            {
                suffixNumber++;
            }
            nom = getName(nom, 19, suffixNumber);
        }
    }

    for (int j = 0; j < nChannels; j++)
    {
        if (*replace < 3 || (nChannels == 2 && j == 0 && indexL == -1) ||
                (nChannels == 2 && j == 1 && indexR == -1) ||
                (nChannels == 1 && indexL == -1)) // Si pas ignorer
        {
            if (((nChannels == 2 && j == 0 && indexL != -1) ||
                 (nChannels == 2 && j == 1 && indexR != -1) ||
                 (nChannels == 1 && indexL != -1)) && (*replace == 2 || *replace == 1))
            {
                if ((nChannels == 2 && j == 0 && indexL != -1) || (nChannels == 1 && indexL != -1))
                    id.indexElt = indexL;
                else
                    id.indexElt = indexR;

                // Mise à jour des données
                AttributeValue valTmp;
                valTmp.wValue = j;
                son->set(champ_wChannel, valTmp);
                QByteArray data = son->getData(24);
                this->sf2->set(id, champ_sampleDataFull24, data);
            }
            else
            {
                // Ajout d'un sample
                id.indexElt = this->sf2->add(id);
                if (nChannels == 2)
                {
                    if (j == 0)
                    {
                        // Gauche
                        this->sf2->set(id, champ_name, nom.left(19).append("L"));
                        val.wValue = id.indexElt + 1;
                        this->sf2->set(id, champ_wSampleLink, val);
                        val.sfLinkValue = leftSample;
                        this->sf2->set(id, champ_sfSampleType, val);
                    }
                    else
                    {
                        // Droite
                        this->sf2->set(id, champ_name, nom.left(19).append("R"));
                        val.wValue = id.indexElt - 1;
                        this->sf2->set(id, champ_wSampleLink, val);
                        val.sfLinkValue = rightSample;
                        this->sf2->set(id, champ_sfSampleType, val);
                    }
                }
                else
                {
                    this->sf2->set(id, champ_name, QString(nom.left(20)));
                    val.wValue = 0;
                    this->sf2->set(id, champ_wSampleLink, val);
                    val.sfLinkValue = monoSample;
                    this->sf2->set(id, champ_sfSampleType, val);
                }
                this->sf2->set(id, champ_filenameForData, qStr);
                val.dwValue = son->get(champ_dwStart16);
                this->sf2->set(id, champ_dwStart16, val);
                val.dwValue = son->get(champ_dwStart24);
                this->sf2->set(id, champ_dwStart24, val);
            }

            // Configuration du sample
            val.wValue = j;
            this->sf2->set(id, champ_wChannel, val);
            val.dwValue = son->get(champ_dwLength);
            this->sf2->set(id, champ_dwLength, val);
            val.dwValue = son->get(champ_dwSampleRate);
            this->sf2->set(id, champ_dwSampleRate, val);
            val.dwValue = son->get(champ_dwStartLoop);
            this->sf2->set(id, champ_dwStartLoop, val);
            val.dwValue = son->get(champ_dwEndLoop);
            this->sf2->set(id, champ_dwEndLoop, val);
            val.bValue = (quint8)son->get(champ_byOriginalPitch);
            this->sf2->set(id, champ_byOriginalPitch, val);
            val.cValue = (char)son->get(champ_chPitchCorrection);
            this->sf2->set(id, champ_chPitchCorrection, val);

            // Automatically remove leading blank?
//            if (ContextManager::configuration()->getValue(ConfManager::SECTION_NONE, "wav_remove_blank", false).toBool())
//                this->page_smpl->enleveBlanc(id);

//            // Automatically trim to loop?
//            if (ContextManager::configuration()->getValue(ConfManager::SECTION_NONE, "wav_auto_loop", false).toBool())
//                this->page_smpl->enleveFin(id);

            //ui->tree->select(id, true);
        }
    }
    delete son;
}

QString MainWindowOld::getName(QString name, int maxCharacters, int suffixNumber)
{
    if (suffixNumber == 0)
        return name.left(maxCharacters);
    QString suffix = QString::number(suffixNumber);
    int suffixSize = suffix.length();
    if (suffixSize > 3 || maxCharacters < suffixSize + 1)
        return name.left(maxCharacters);
    return name.left(maxCharacters - suffixSize - 1) + "-" + suffix;
}

void MainWindowOld::exporterSmpl()
{
    QList<EltID> listIDs;
    if (listIDs.isEmpty()) return;
    QString qDir = QFileDialog::getExistingDirectory(this, trUtf8("Choisir un répertoire de destination"),
                                                     ContextManager::recentFile()->getLastDirectory(RecentFileManager::FILE_TYPE_SAMPLE));
    if (qDir.isEmpty()) return;
    qDir += "/";

    // Mémorisation des samples exportés
    QList<EltID> exportedSamples;

    // Exportation pour chaque sample
    int sampleID = -1;
    int sampleID2 = -1;
    QString qStr;
    EltID id2;
    foreach (EltID id, listIDs)
    {
        // Find EltID of the sample
        id2 = id;
        if (id.typeElement == elementSmpl)
            sampleID = id.indexElt;
        else if (id.typeElement == elementInstSmpl)
            sampleID = this->sf2->get(id, champ_sampleID).wValue;
        else
            sampleID = -1;
        id.typeElement = elementSmpl;
        id.indexElt = sampleID;

        if (sampleID != -1)
        {
            // Vérification que le sample n'a pas déjà été exporté
            if (!exportedSamples.contains(id))
            {
                qStr = qDir;

                // Stéréo ?
                if (this->sf2->get(id, champ_sfSampleType).wValue != monoSample &&
                        this->sf2->get(id, champ_sfSampleType).wValue != RomMonoSample)
                {
                    sampleID2 = this->sf2->get(id, champ_wSampleLink).wValue;
                    id2.indexElt = sampleID2;
                    id2.typeElement = elementSmpl;

                    // Nom du fichier
                    QString nom1 = sf2->getQstr(id, champ_name);
                    QString nom2 = sf2->getQstr(id2, champ_name);
                    int nb = Sound::lastLettersToRemove(nom1, nom2);
                    qStr.append(nom1.left(nom1.size() - nb).replace(QRegExp("[:<>\"/\\\\\\*\\?\\|]"), "_"));

                    if (this->sf2->get(id, champ_sfSampleType).wValue == rightSample &&
                            this->sf2->get(id, champ_sfSampleType).wValue != RomRightSample)
                    {
                        // Inversion smpl1 smpl2
                        EltID idTmp = id;
                        id = id2;
                        id2 = idTmp;
                    }

                    // Mémorisation de l'export
                    exportedSamples << id << id2;
                }
                else
                {
                    sampleID2 = -1;

                    // Nom du fichier
                    qStr.append(this->sf2->getQstr(id, champ_name).replace(QRegExp("[:<>\"/\\\\\\*\\?\\|]"), "_"));

                    // Mémorisation de l'export
                    exportedSamples << id;
                }

                // Nom pour l'exportation
                if (QFile::exists(qStr + ".wav"))
                {
                    // Modification du nom
                    int indice = 1;
                    while (QFile::exists(qStr + "-" + QString::number(indice) + ".wav"))
                        indice++;
                    qStr += "-" + QString::number(indice);
                }

                // Exportation
                if (sampleID2 == -1)
                    Sound::exporter(qStr + ".wav", this->sf2->getSon(id));
                else
                    Sound::exporter(qStr + ".wav", this->sf2->getSon(id), this->sf2->getSon(id2));
                ContextManager::recentFile()->addRecentFile(RecentFileManager::FILE_TYPE_SAMPLE, qStr + ".wav");
            }
        }
    }
}

void MainWindowOld::exporter()
{
    int nbElt;
    if (nbElt == 0)
        return;

    // List of sf2
    QList<EltID> selectedElements;// = ui->tree->getAllIDs();
    QList<EltID> listSf2;
    for (int i = 0; i < selectedElements.size(); i++)
    {
        int present = false;
        for (int j = 0; j < listSf2.size(); j++)
            if (listSf2[j].indexSf2 == selectedElements[i].indexSf2)
                present = true;
        if (!present)
            listSf2 << selectedElements[i];
    }

    DialogExport * dial = new DialogExport(sf2, listSf2, this);
    connect(dial, SIGNAL(accepted(QList<QList<EltID> >,QString,int,bool,bool,bool,int)),
            this, SLOT(exporter(QList<QList<EltID> >,QString,int,bool,bool,bool,int)));
    dial->show();
}

void MainWindowOld::exporter(QList<QList<EltID> > listID, QString dir, int format, bool presetPrefix, bool bankDir, bool gmSort, int quality)
{
    int flags = presetPrefix + bankDir * 0x02 + gmSort * 0x04;

    _progressDialog = new ModalProgressDialog(trUtf8("Opération en cours..."), trUtf8("Annuler"), 0, 0, this);
    _progressDialog->show();
    QApplication::processEvents();

    QFuture<int> future = QtConcurrent::run(this, &MainWindowOld::exporter2, listID, dir, format, flags, quality);
    _futureWatcher.setFuture(future);
}

int MainWindowOld::exporter2(QList<QList<EltID> > listID, QString dir, int format, int flags, int quality)
{
    // Flags
    bool presetPrefix = ((flags & 0x01) != 0);
    bool bankDir = ((flags & 0x02) != 0);
    bool gmSort = ((flags & 0x04) != 0);

    switch (format)
    {
    case 0: case 1: {
        // Export sf2 ou sf3, création d'un nouvel sf2 indépendant
        SoundfontManager * newSf2 = SoundfontManager::getOtherInstance(); // Not linked with tree
        EltID idDest(elementSf2, 0, 0, 0, 0);
        idDest.indexSf2 = newSf2->add(idDest);

        // Infos du nouvel sf2
        QString name, comment;
        if (listID.size() == 1)
        {
            EltID idSf2Source = listID.first().first();
            idSf2Source.typeElement = elementSf2;
            name = sf2->getQstr(idSf2Source, champ_name);
            comment = sf2->getQstr(idSf2Source, champ_ICMT);
            newSf2->set(idDest, champ_ISNG, sf2->getQstr(idSf2Source, champ_ISNG));
            newSf2->set(idDest, champ_IROM, sf2->getQstr(idSf2Source, champ_IROM));
            newSf2->set(idDest, champ_ICRD, sf2->getQstr(idSf2Source, champ_ICRD));
            newSf2->set(idDest, champ_IENG, sf2->getQstr(idSf2Source, champ_IENG));
            newSf2->set(idDest, champ_IPRD, sf2->getQstr(idSf2Source, champ_IPRD));
            newSf2->set(idDest, champ_ICOP, sf2->getQstr(idSf2Source, champ_ICOP));
            newSf2->set(idDest, champ_ISFT, sf2->getQstr(idSf2Source, champ_ISFT));
        }
        else
        {
            name = "soundfont";
            comment = trUtf8("Fusion des soundfonts :");
            foreach (QList<EltID> subList, listID)
            {
                EltID idSf2Source = subList.first();
                idSf2Source.typeElement = elementSf2;
                comment += "\n - " + sf2->getQstr(idSf2Source, champ_name);
            }
        }
        newSf2->set(idDest, champ_name, name);
        newSf2->set(idDest, champ_ICMT, comment);

        // Ajout des presets
        Duplicator duplicator(this->sf2, newSf2, this);
        for (int nbBank = 0; nbBank < listID.size(); nbBank++)
        {
            QList<EltID> subList = listID[nbBank];
            for (int nbPreset = 0; nbPreset < subList.size(); nbPreset++)
            {
                EltID id = subList[nbPreset];

                if (listID.size() == 1)
                {
                    duplicator.copy(id, idDest);
                }
                else
                {
                    int originalBank = sf2->get(id, champ_wBank).wValue;
                    int originalPreset = sf2->get(id, champ_wPreset).wValue;
                    AttributeValue value;
                    value.wValue = nbBank;
                    sf2->set(id, champ_wBank, value);
                    value.wValue = nbPreset;
                    sf2->set(id, champ_wPreset, value);

                    duplicator.copy(id, idDest);

                    value.wValue = originalBank;
                    sf2->set(id, champ_wBank, value);
                    value.wValue = originalPreset;
                    sf2->set(id, champ_wPreset, value);
                }
            }
        }
        sf2->clearNewEditing();

        // Détermination du nom de fichier
        name = name.replace(QRegExp("[:<>\"/\\\\\\*\\?\\|]"), "_");

        QString extension = (format == 0 ? ".sf2" : ".sf3");
        QFile fichier(dir + "/" + name + extension);
        if (fichier.exists())
        {
            int i = 1;
            while (QFile(dir + "/" + name + "-" + QString::number(i) + extension).exists())
                i++;
            name += "-" + QString::number(i);
        }
        name = dir + "/" + name + extension;

        // Sauvegarde
        newSf2->save(idDest.indexSf2, name, quality);
        delete newSf2;
    }break;
    case 2:
        // Export sfz
        foreach (QList<EltID> sublist, listID)
            ConversionSfz(sf2).convert(dir, sublist, presetPrefix, bankDir, gmSort);
        break;
    default:
        break;
    }

    return -10; // For QFuture
}

void MainWindowOld::futureFinished()
{
    int result = _futureWatcher.result();
    if (result > -10)
    {
        // An sf2 has been just loaded
        switch (result)
        {
        case -1: // Warning and continue with 0
            QMessageBox::warning(this, QObject::trUtf8("Attention"),
                                 trUtf8("Fichier corrompu : utilisation des échantillons en qualité 16 bits."));
        case 0:
            // Loading ok
            //updateFavoriteFiles();
            //updateActions();
            break;
        case 1:
            QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Format inconnu."));
            break;
        case 2:
            QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Le fichier est déjà chargé."));
            break;
        case 3:
            QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Impossible d'ouvrir le fichier."));
            break;
        case 4:
            QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Lecture impossible."));
            break;
        case 5: case 6:
            QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Le fichier est corrompu."));
            break;
        }
    }

    _progressDialog->deleteLater();
    _progressDialog = NULL;
}

void MainWindowOld::nouvelInstrument()
{
//    int nb = ui->tree->getSelectedItemsNumber();
//    if (nb == 0) return;
//    EltID id = ui->tree->getFirstID();
//    bool ok;
//    QString name = QInputDialog::getText(this, trUtf8("Créer un nouvel instrument"), trUtf8("Nom du nouvel instrument :"), QLineEdit::Normal, "", &ok);
//    if (ok && !name.isEmpty())
//    {
//        // Reprise ID si modif
//        id = ui->tree->getFirstID();
//        id.typeElement = elementInst;
//        id.indexElt = this->sf2->add(id);
//        this->sf2->set(id, champ_name, name.left(20));
//        updateDo();
//        ui->tree->clearSelection();
//        ui->tree->select(id, true);
//    }
}
void MainWindowOld::nouveauPreset()
{
//    int nb = ui->tree->getSelectedItemsNumber();
//    if (nb == 0) return;
//    EltID id = ui->tree->getFirstID();
//    // Vérification qu'un preset est disponible
//    int nPreset = -1;
//    int nBank = -1;
//    sf2->firstAvailablePresetBank(id, nBank, nPreset);
//    if (nPreset == -1)
//    {
//        QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Aucun preset n'est disponible."));
//        return;
//    }
//    bool ok;
//    QString text = "";
//    if (id.typeElement == elementInst || id.typeElement == elementInstSmpl)
//    {
//        id.typeElement = elementInst;
//        text = this->sf2->getQstr(id, champ_name);
//    }
//    QString name = QInputDialog::getText(this, trUtf8("Créer un nouveau preset"), trUtf8("Nom du nouveau preset :"), QLineEdit::Normal, text, &ok);
//    if (ok && !name.isEmpty())
//    {
//        Valeur val;
//        // Reprise de l'identificateur si modification
//        id = ui->tree->getFirstID();
//        id.typeElement = elementPrst;
//        id.indexElt = this->sf2->add(id);
//        this->sf2->set(id, champ_name, name.left(20));
//        val.wValue = nPreset;
//        this->sf2->set(id, champ_wPreset, val);
//        val.wValue = nBank;
//        this->sf2->set(id, champ_wBank, val);
//        updateDo();
//        ui->tree->clearSelection();
//        ui->tree->select(id, true);
//    }
}
void MainWindowOld::associer()
{
//    if (!ui->tree->getSelectedItemsNumber())
//        return;
//    EltID id = ui->tree->getFirstID();
//    this->dialList.showDialog(id, DialogList::MODE_ASSOCIATION);
}
void MainWindowOld::associer(EltID idDest)
{
    int nb;// = ui->tree->getSelectedItemsNumber();
    if (nb == 0 || (idDest.typeElement != elementInst && idDest.typeElement != elementPrst))
    {
        //updateDo();
        return;
    }
    AttributeType champ;
    if (idDest.typeElement == elementInst)
    {
        idDest.typeElement = elementInstSmpl;
        champ = champ_sampleID;
    }
    else
    {
        idDest.typeElement = elementPrstInst;
        champ = champ_instrument;
    }
    AttributeValue val;

    // Liste des éléments sources
    QList<EltID> listeSrc;// = ui->tree->getAllIDs();
    foreach (EltID idSrc, listeSrc)
    {
        // Création élément lié
        idDest.indexElt2 = this->sf2->add(idDest);
        // Association de idSrc vers idDest
        val.wValue = idSrc.indexElt;
        this->sf2->set(idDest, champ, val);
        if (champ == champ_sampleID)
        {
            // Balance
            if (this->sf2->get(idSrc, champ_sfSampleType).sfLinkValue == rightSample ||
                    this->sf2->get(idSrc, champ_sfSampleType).sfLinkValue == RomRightSample)
            {
                val.shValue = 500;
                this->sf2->set(idDest, champ_pan, val);
            }
            else if (this->sf2->get(idSrc, champ_sfSampleType).sfLinkValue == leftSample ||
                     this->sf2->get(idSrc, champ_sfSampleType).sfLinkValue == RomLeftSample)
            {
                val.shValue = -500;
                this->sf2->set(idDest, champ_pan, val);
            }
            else
            {
                val.shValue = 0;
                this->sf2->set(idDest, champ_pan, val);
            }
        }
        else
        {
            // Key range
            int keyMin = 127;
            int keyMax = 0;
            EltID idLinked = idSrc;
            idLinked.typeElement = elementInstSmpl;
            foreach (int i, this->sf2->getSiblings(idLinked))
            {
                idLinked.indexElt2 = i;
                if (this->sf2->isSet(idLinked, champ_keyRange))
                {
                    keyMin = qMin(keyMin, (int)this->sf2->get(idLinked, champ_keyRange).rValue.byLo);
                    keyMax = qMax(keyMax, (int)this->sf2->get(idLinked, champ_keyRange).rValue.byHi);
                }
            }
            AttributeValue value;
            if (keyMin < keyMax)
            {
                value.rValue.byLo = keyMin;
                value.rValue.byHi = keyMax;
            }
            else
            {
                value.rValue.byLo = 0;
                value.rValue.byHi = 127;
            }
            this->sf2->set(idDest, champ_keyRange, value);
        }
    }
}
void MainWindowOld::remplacer()
{
    int nb;// = ui->tree->getSelectedItemsNumber();
    if (nb != 1) return;
    EltID id;// = ui->tree->getFirstID();
    this->dialList.showDialog(id, DialogList::MODE_REMPLACEMENT);
}
void MainWindowOld::remplacer(EltID idSrc)
{
    int nb;// = ui->tree->getSelectedItemsNumber();
    if (nb != 1 || (idSrc.typeElement != elementSmpl && idSrc.typeElement != elementInst))
    {
        //updateDo();
        return;
    }
    EltID idDest;// = ui->tree->getFirstID();
    if (idDest.typeElement != elementInstSmpl && idDest.typeElement != elementPrstInst)
        return;
    AttributeType champ;
    if (idSrc.typeElement == elementSmpl)
        champ = champ_sampleID;
    else
        champ = champ_instrument;

    // Remplacement d'un sample ou instrument lié
    AttributeValue val;
    val.wValue = idSrc.indexElt;
    this->sf2->set(idDest, champ, val);
    //updateDo();
    //updateActions();
}

// Envoi de signaux
void MainWindowOld::copier()
{
    // émission d'un signal "copier"
    QWidget* focused = QApplication::focusWidget();
    if( focused != 0 )
    {
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
    }
}
void MainWindowOld::coller()
{
    // émission d'un signal "coller"
    QWidget* focused = QApplication::focusWidget();
    if( focused != 0 )
    {
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
    }
}
void MainWindowOld::supprimer()
{
    // émission d'un signal "supprimer"
    QWidget* focused = QApplication::focusWidget();
    if( focused != 0 )
    {
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, 0));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Delete, 0));
    }
}

void MainWindowOld::duplication()
{
    //if (ui->tree->getSelectedItemsNumber() == 0) return;
    ElementType type;// = ui->tree->getFirstID().typeElement;
    if (type == elementInst || type == elementInstSmpl)
        this->page_inst->duplication();
    else if (type == elementPrst || type == elementPrstInst)
        this->page_prst->duplication();
}

void MainWindowOld::spatialisation()
{
    //if (ui->tree->getSelectedItemsNumber() == 0) return;
    ElementType type;// = ui->tree->getFirstID().typeElement;
    if (type == elementInst || type == elementInstSmpl)
        this->page_inst->spatialisation();
    else if (type == elementPrst || type == elementPrstInst)
        this->page_prst->spatialisation();
}

void MainWindowOld::on_action_Transposer_triggered()
{
    this->page_inst->transposer();
}

void MainWindowOld::visualize()
{
    //if (ui->tree->getSelectedItemsNumber() == 0) return;
    ElementType type;// = ui->tree->getFirstID().typeElement;
    if (type == elementInst || type == elementInstSmpl)
        this->page_inst->visualize();
    else if (type == elementPrst || type == elementPrstInst)
        this->page_prst->visualize();
}
void MainWindowOld::purger()
{
    // Suppression des éléments non utilisés
    //if (ui->tree->getSelectedItemsNumber() == 0) return;
    //if (!ui->tree->isSelectedItemsSf2Unique()) return;
    EltID id;// = ui->tree->getFirstID();

    // Nombre de samples et instruments non utilisés
    int unusedSmpl = 0;
    int unusedInst = 0;
    id.typeElement = elementSmpl;
    QList<int> nbSmpl = sf2->getSiblings(id);
    id.typeElement = elementInst;
    QList<int> nbInst = sf2->getSiblings(id);
    id.typeElement = elementPrst;
    QList<int> nbPrst = sf2->getSiblings(id);
    bool smplUsed, instUsed;

    // Suppression des instruments
    foreach (int i, nbInst)
    {
        instUsed = false;

        // on regarde chaque preset
        foreach (int j, nbPrst)
        {
            // composé d'instruments
            id.indexElt = j;
            id.typeElement = elementPrstInst;
            foreach (int k, sf2->getSiblings(id))
            {
                id.indexElt2 = k;
                if (sf2->get(id, champ_instrument).wValue == i)
                    instUsed = true;
            }
        }
        if (!instUsed)
        {
            // suppression de l'instrument
            unusedInst++;
            id.typeElement = elementInst;
            id.indexElt = i;
            int message;
            sf2->remove(id, &message);
        }
    }

    // Suppression des samples
    // pour chaque sample
    foreach (int i, nbSmpl)
    {
        smplUsed = false;

        // on regarde chaque instrument
        foreach (int j, nbInst)
        {
            // composé de samples
            id.indexElt = j;
            id.typeElement = elementInstSmpl;
            foreach (int k, sf2->getSiblings(id))
            {
                id.indexElt2 = k;
                if (sf2->get(id, champ_sampleID).wValue == i)
                    smplUsed = true;
            }
        }

        if (!smplUsed)
        {
            // suppression du sample
            unusedSmpl++;
            id.typeElement = elementSmpl;
            id.indexElt = i;
            int message;
            sf2->remove(id, &message);
        }
    }
    // Bilan
    QString qStr;
    if (unusedSmpl < 2)
        qStr = QString::number(unusedSmpl) + trUtf8(" échantillon et ");
    else
        qStr = QString::number(unusedSmpl) + trUtf8(" échantillons et ");
    if (unusedInst < 2)
        qStr += QString::number(unusedInst) + trUtf8(" instrument ont été supprimés.");
    else
        qStr += QString::number(unusedInst) + trUtf8(" instruments ont été supprimés.");

    QMessageBox::information(this, "", qStr);
    //updateDo();
    //updateActions();
}

void MainWindowOld::exportPresetList()
{
    //if (ui->tree->getSelectedItemsNumber() == 0) return;
    EltID id;// = ui->tree->getFirstID();
    DialogExportList * dial = new DialogExportList(sf2, id, this);
    dial->setAttribute(Qt::WA_DeleteOnClose);
    dial->show();
}
void MainWindowOld::on_actionExporter_pics_de_fr_quence_triggered()
{
    EltID id;// = ui->tree->getFirstID();
    id.typeElement = elementSf2;
    QString defaultFile = ContextManager::recentFile()->getLastDirectory(RecentFileManager::FILE_TYPE_FREQUENCIES) + "/" +
            sf2->getQstr(id, champ_name).replace(QRegExp(QString::fromUtf8("[`~*|:<>«»?/{}\"\\\\]")), "_") + ".csv";
    QString fileName = QFileDialog::getSaveFileName(this, trUtf8("Exporter les pics de fréquence"),
                                                    defaultFile, trUtf8("Fichier .csv (*.csv)"));
    if (!fileName.isEmpty())
    {
        ContextManager::recentFile()->addRecentFile(RecentFileManager::FILE_TYPE_FREQUENCIES, fileName);
        exporterFrequences(fileName);
    }
}

void MainWindowOld::exporterFrequences(QString fileName)
{
    // Création fichier csv
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QString sep = trUtf8(";");
    QTextStream stream(&file);
    stream << "\"" << trUtf8("Echantillon") << "\"" << sep << "\"" << trUtf8("Numéro de pic") << "\"" << sep << "\""
           << trUtf8("Facteur") << "\"" << sep << "\"" << trUtf8("Fréquence") << "\"" << sep << "\""
           << trUtf8("Note") << "\"" << sep << "\"" << trUtf8("Correction") << "\"";

    EltID id;// = ui->tree->getFirstID();
    id.typeElement = elementSmpl;
    QString nomSmpl;
    QList<double> listeFrequences, listeFacteurs;
    QList<int> listeNotes, listeCorrections;
    foreach (int i, sf2->getSiblings(id))
    {
        id.indexElt = i;
        nomSmpl = sf2->getQstr(id, champ_name).replace(QRegExp(QString::fromUtf8("[`~*|:<>«»?/{}\"\\\\]")), "_");

        // Ecriture valeurs pour l'échantillon
        page_smpl->getPeakFrequencies(id, listeFrequences, listeFacteurs, listeNotes, listeCorrections);

        for (int j = 0; j < listeFrequences.size(); j++)
        {
            stream << endl;
            stream << "\"" << nomSmpl << "\"" << sep;
            stream << j << sep;
            stream << QString::number(listeFacteurs.at(j)).replace(".", trUtf8(",")) << sep;
            stream << QString::number(listeFrequences.at(j)).replace(".", trUtf8(",")) << sep;
            stream << ContextManager::keyName()->getKeyName(listeNotes.at(j)) << sep;
            stream << listeCorrections.at(j);
        }
    }
    file.close();
}

void MainWindowOld::on_actionEnlever_tous_les_modulateurs_triggered()
{
    EltID id;// = ui->tree->getFirstID();

    int count = 0;

    // Suppression des modulateurs liés aux instruments
    id.typeElement = elementInst;
    count += deleteMods(id);

    // Suppression des modulateurs liés aux presets
    id.typeElement = elementPrst;
    count += deleteMods(id);

    if (count == 0)
        QMessageBox::warning(this, trUtf8("Attention"), trUtf8("Le fichier ne contient aucun modulateur."));
    else if (count == 1)
        QMessageBox::information(this, trUtf8("Information"), trUtf8("1 modulateur a été supprimé."));
    else
        QMessageBox::information(this, trUtf8("Information"), QString::number(count) + " " +
                                 trUtf8("modulateurs ont été supprimés."));
    //updateDo();
    //updateActions();
}

int MainWindowOld::deleteMods(EltID id)
{
    int count = 0;

    foreach (int i, sf2->getSiblings(id))
    {
        id.indexElt = i;
        // Modulateurs globaux
        EltID idMod = id;
        if (id.typeElement == elementInst)
            idMod.typeElement = elementInstMod;
        else
            idMod.typeElement = elementPrstMod;
        foreach (int j, sf2->getSiblings(idMod))
        {
            idMod.indexMod = j;
            sf2->remove(idMod);
            count++;
        }

        // Modulateurs de chaque division
        EltID idSub = id;
        if (id.typeElement == elementInst)
            idSub.typeElement = elementInstSmpl;
        else
            idSub.typeElement = elementPrstInst;
        foreach (int j, sf2->getSiblings(idSub))
        {
            idSub.indexElt2 = j;
            idMod = idSub;
            if (id.typeElement == elementInst)
                idMod.typeElement = elementInstSmplMod;
            else
                idMod.typeElement = elementPrstInstMod;
            foreach (int k, sf2->getSiblings(idMod))
            {
                idMod.indexMod = k;
                sf2->remove(idMod);
                count++;
            }
        }
    }

    return count;
}
