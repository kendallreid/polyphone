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

#include "sound.h"
#include <QMessageBox>

// Constructeur / destructeur
Sound::Sound(QString filename)
{
    this->sm24.clear();
    this->smpl.clear();
    this->info.wChannel = 0;
    this->info.wChannels = 0;
    this->info.dwLength = 0;
    this->info.dwStart = 0;
    this->info.dwStart2 = 0;
    this->info.dwSampleRate = 0;
    this->info.dwStartLoop = 0;
    this->info.dwEndLoop = 0;
    this->info.dwNote = 0;
    this->info.iCent = 0;
    this->info.wBpsFile = 0;
    this->setFileName(filename);
}
Sound::~Sound() {}

////////////////////////////////////////////////////////////////////////////
//////////////////////////   METHODES PUBLIQUES   //////////////////////////
////////////////////////////////////////////////////////////////////////////

QByteArray Sound::getData(WORD wBps)
{
    // Copie des données dans data, résolution wBps
    // wBps = 16 : chargement smpl, 16 bits de poids fort
    // wBps =  8 : chargement sm24, 8 bits suivant le 16 de poids fort
    // wBps = 24 : chargement 24 bits de poids fort
    // wBps = 32 : chargement en 32 bits
    QByteArray baRet;
    switch (wBps)
    {
    case 8:
        // chargement des 8 bits de poids faible
        if (!this->sm24.isEmpty())
           baRet = this->sm24;
        else if (this->info.wBpsFile >= 24)
        {
            // lecture fichier
            QFile *fi = new QFile(fileName);
            if (!fi->exists())
            {
                // Si impossible à ouvrir : pas de message d'erreur et remplissage 0
                baRet.resize(this->info.dwLength);
                baRet.fill(0);
            }
            else
            {
                fi->open(QFile::ReadOnly | QFile::Unbuffered);
                switch (this->getFileType())
                {
                case fileSf2:
                    baRet = getDataSf2(fi, 1);
                    break;
                case fileWav:
                    baRet = getDataWav(fi, 1);
                    break;
                case fileCustom1:{
                    typedef QByteArray (*GetDataSound)(QFile*, WORD, InfoSound);
                    GetDataSound getDataSound = (GetDataSound) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                                             "getDataSound");
                    if (getDataSound)
                        baRet = getDataSound(fi, 1, this->info);
                    else
                    {
                        baRet.resize(this->info.dwLength);
                        baRet.fill(0);
                    }
                    }break;
                default:
                    QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Fichier non pris en charge."));
                }
                // Fermeture du fichier
                fi->close();
            }
            delete fi;
        }
        else
        {
            // remplissage 0
            baRet.resize(this->info.dwLength);
            baRet.fill(0);
        }
        break;
    case 16:
        // chargement des 16 bits de poids fort
        if (!this->smpl.isEmpty())
            baRet = this->smpl;
        else
        {
            // lecture fichier
            QFile *fi = new QFile(fileName);
            if (!fi->exists())
            {
                // Si impossible à ouvrir : pas de message d'erreur et remplissage 0
                baRet.resize(this->info.dwLength*2);
                baRet.fill(0);
            }
            else
            {
                fi->open(QFile::ReadOnly | QFile::Unbuffered);
                switch (this->getFileType())
                {
                case fileSf2:
                    baRet = getDataSf2(fi, 2);
                    break;
                case fileWav:
                    baRet = getDataWav(fi, 2);
                    break;
                case fileCustom1:{
                    typedef QByteArray (*GetDataSound)(QFile*, WORD, InfoSound);
                    GetDataSound getDataSound = (GetDataSound) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                                             "getDataSound");
                    if (getDataSound)
                        baRet = getDataSound(fi, 2, this->info);
                    else
                    {
                        baRet.resize(this->info.dwLength*2);
                        baRet.fill(0);
                    }
                    }break;
                default:
                    QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Fichier non pris en charge."));
                }
                // Fermeture du fichier
                fi->close();
            }
            delete fi;
        }
        break;
    case 24:
        // chargement des 24 bits
        if (!this->smpl.isEmpty() && !this->sm24.isEmpty() && this->info.wBpsFile >= 24)
        {
            // copie 24 bits
            baRet.resize(this->info.dwLength*3);
            char * cDest = baRet.data();
            char * cFrom = this->smpl.data();
            char * cFrom24 = this->smpl.data();
            int len = (int)this->info.dwLength;
            for (int i = 0; i < len; i++)
            {
                cDest[3*i] = cFrom24[i];
                cDest[3*i+1] = cFrom[2*i];
                cDest[3*i+2] = cFrom[2*i+1];
            }
        }
        else if (!this->smpl.isEmpty() && this->info.wBpsFile < 24)
        {
            // copie 16 bits
            baRet.resize(this->info.dwLength*3);
            char * cDest = baRet.data();
            char * cFrom = this->smpl.data();
            int len = (int)this->info.dwLength;
            for (int i = 0; i < len; i++)
            {
                cDest[3*i] = 0;
                cDest[3*i+1] = cFrom[2*i];
                cDest[3*i+2] = cFrom[2*i+1];
            }
        }
        else
        {
            // lecture fichier
            QFile *fi = new QFile(fileName);
            if (!fi->exists())
            {
                // Si impossible à ouvrir : pas de message d'erreur et remplissage 0
                baRet.resize(this->info.dwLength*3);
                baRet.fill(0);
            }
            else
            {
                fi->open(QFile::ReadOnly | QFile::Unbuffered);
                switch (this->getFileType())
                {
                case fileSf2:
                    baRet = getDataSf2(fi, 3);
                    break;
                case fileWav:
                    baRet = getDataWav(fi, 3);
                    break;
                case fileCustom1:{
                    typedef QByteArray (*GetDataSound)(QFile*, WORD, InfoSound);
                    GetDataSound getDataSound = (GetDataSound) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                                             "getDataSound");
                    if (getDataSound)
                        baRet = getDataSound(fi, 3, this->info);
                    else
                    {
                        baRet.resize(this->info.dwLength*3);
                        baRet.fill(0);
                    }
                    }break;
                default:
                    QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Fichier non pris en charge."));
                }
                // Fermeture du fichier
                fi->close();
            }
            delete fi;
        }
        break;
    case 32:
        // chargement en 32 bits
        if (!this->smpl.isEmpty() && !this->sm24.isEmpty() && this->info.wBpsFile >= 24)
        {
            // copie 24 bits
            baRet.resize(this->info.dwLength*4);
            char * cDest = baRet.data();
            char * cFrom = this->smpl.data();
            char * cFrom24 = this->smpl.data();
            int len = (int)this->info.dwLength;
            for (int i = 0; i < len; i++)
            {
                cDest[4*i] = 0;
                cDest[4*i+1] = cFrom24[i];
                cDest[4*i+2] = cFrom[2*i];
                cDest[4*i+3] = cFrom[2*i+1];
            }
        }
        else if (!this->smpl.isEmpty() && this->info.wBpsFile < 24)
        {
            // copie 16 bits
            baRet.resize(this->info.dwLength*4);
            char * cDest = baRet.data();
            char * cFrom = this->smpl.data();
            int len = (int)this->info.dwLength;
            for (int i = 0; i < len; i++)
            {
                cDest[4*i] = 0;
                cDest[4*i+1] = 0;
                cDest[4*i+2] = cFrom[2*i];
                cDest[4*i+3] = cFrom[2*i+1];
            }
        }
        else
        {
            // lecture fichier
            QFile *fi = new QFile(fileName);
            if (!fi->exists())
            {
                // Si impossible à ouvrir : pas de message d'erreur et remplissage 0
                baRet.resize(this->info.dwLength*4);
                baRet.fill(0);
            }
            else
            {
                fi->open(QFile::ReadOnly | QFile::Unbuffered);
                switch (this->getFileType())
                {
                case fileSf2:
                    baRet = getDataSf2(fi, 4);
                    break;
                case fileWav:
                    baRet = getDataWav(fi, 4);
                    break;
                case fileCustom1:{
                    typedef QByteArray (*GetDataSound)(QFile*, WORD, InfoSound);
                    GetDataSound getDataSound = (GetDataSound) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                                             "getDataSound");
                    if (getDataSound)
                        baRet = getDataSound(fi, 4, this->info);
                    else
                    {
                        baRet.resize(this->info.dwLength*4);
                        baRet.fill(0);
                    }
                    }break;
                default:
                    QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Fichier non pris en charge."));
                }
                // Fermeture du fichier
                fi->close();
            }
            delete fi;
        }
        break;
    default:
        QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Erreur dans Sound::getData."));
    }
    return baRet;
}
DWORD Sound::get(Champ champ)
{
    DWORD result = 0;
    switch (champ)
    {
    case champ_dwStart16:
        result = this->info.dwStart;
        break;
    case champ_dwStart24:
        result = this->info.dwStart2;
        break;
    case champ_dwLength:
        result = this->info.dwLength;
        break;
    case champ_dwStartLoop:
        result = this->info.dwStartLoop;
        break;
    case champ_dwEndLoop:
        result = this->info.dwEndLoop;
        break;
    case champ_dwSampleRate:
        result = this->info.dwSampleRate;
        break;
    case champ_bpsFile:
        result = this->info.wBpsFile;
        break;
    case champ_wChannel:
        result = this->info.wChannel;
        break;
    case champ_wChannels:
        result = this->info.wChannels;
        break;
    case champ_byOriginalPitch:
        result = this->info.dwNote;
        break;
    case champ_chPitchCorrection:
        result = this->info.iCent;
        break;
    default:
        break;
    }
    return result;
}
QString Sound::getFileName() {return this->fileName;}

void Sound::setData(QByteArray data, WORD wBps)
{
    if (wBps == 8)
    {
        // Remplacement des données 17-24 bits
        this->sm24 = data;
    }
    else if (wBps == 16)
    {
        // Remplacement des données 16 bits
        this->smpl = data;
    }
    else
        QMessageBox::information(NULL, QObject::tr("Attention"),
                                 QString::fromUtf8(QObject::tr("Dans setData : opération non autorisée.").toStdString().c_str()));
}
void Sound::set(Champ champ, Valeur value)
{
    switch (champ)
    {
    case champ_dwStart16:
        // modification du départ
        this->info.dwStart = value.dwValue;
        break;
    case champ_dwStart24:
        // modification du départ
        this->info.dwStart2 = value.dwValue;
        break;
    case champ_dwLength:
        // modification de la longueur
        this->info.dwLength = value.dwValue;
        break;
    case champ_dwStartLoop:
        // modification du début de la boucle
        this->info.dwStartLoop = value.dwValue;
        break;
    case champ_dwEndLoop:
        // modification de la fin de la boucle
        this->info.dwEndLoop = value.dwValue;
        break;
    case champ_dwSampleRate:
        // modification de l'échantillonnage
        this->info.dwSampleRate = value.dwValue;
        break;
    case champ_wChannel:
        // modification du canal utilisé
        this->info.wChannel = value.wValue;
        break;
    case champ_wChannels:
        // modification du nombre de canaux
        this->info.wChannels = value.wValue;
        break;
    case champ_bpsFile:
        // modification de la résolution
        this->info.wBpsFile = value.wValue;
        if (value.wValue < 24)
            this->sm24.clear();
        break;
    case champ_byOriginalPitch:
        // Modification de la note en demi tons
        this->info.dwNote = value.bValue;
        break;
    case champ_chPitchCorrection:
        // Modification de la note en centième de ton
        this->info.iCent = value.cValue;
        break;
    default:
        break;
    }
}
void Sound::setFileName(QString qStr)
{
    this->fileName = qStr;
    // Récupération des infos sur le son
    this->getInfoSound();
}
void Sound::setRam(bool ram)
{
    if (ram)
    {
        if (this->smpl.isEmpty())
        {
            // Chargement des données en ram, 16 bits de poids fort
            this->smpl = this->getData(16);
        }
        if (this->sm24.isEmpty() && this->info.wBpsFile >= 24)
        {
            // chargement des 8 bits de poids faible
            this->sm24 = this->getData(8);
        };
    }
    else
    {
        // libération des données
        this->smpl.clear();
        this->sm24.clear();
    }
}

void Sound::exporter(QString fileName, Sound son)
{
    // Exportation d'un sample mono au format wav
    WORD wBps = son.info.wBpsFile;
    if (wBps > 16)
        wBps = 24;
    else
        wBps = 16;
    QByteArray baData = son.getData(wBps);

    // Création d'un fichier
    InfoSound info = son.info;
    info.wBpsFile = wBps;
    info.wChannels = 1;
    exporter(fileName, baData, info);
}
void Sound::exporter(QString fileName, Sound son1, Sound son2)
{
    // Exportation d'un sample stereo au format wav
    // bps (max des 2 canaux)
    quint16 wBps = son1.info.wBpsFile;
    if (son2.info.wBpsFile > wBps)
        wBps = son2.info.wBpsFile;
    if (wBps > 16)
        wBps = 24;
    else
        wBps = 16;
    // Récupération des données
    QByteArray channel1 = son1.getData(wBps);
    QByteArray channel2 = son2.getData(wBps);
    // sample rate (max des 2 canaux)
    quint32 dwSmplRate = son1.info.dwSampleRate;
    if (son2.info.dwSampleRate > dwSmplRate)
    {
        // Ajustement son1
        channel1 = resampleMono(channel1, dwSmplRate, son2.info.dwSampleRate, wBps);
        dwSmplRate = son2.info.dwSampleRate;
    }
    else if (son2.info.dwSampleRate < dwSmplRate)
    {
        // Ajustement son2
        channel2 = resampleMono(channel2, son2.info.dwSampleRate, dwSmplRate, wBps);
    }
    // Taille et mise en forme des données
    quint32 dwLength = channel1.size();
    if (dwLength < (unsigned)channel2.size())
    {
        // On complète chanel1
        QByteArray baTemp;
        baTemp.resize(channel2.size() - dwLength);
        baTemp.fill(0);
        channel1.append(baTemp);
        dwLength = channel2.size();
    }
    else if (dwLength > (unsigned)channel2.size())
    {
        // On complète chanel2
        QByteArray baTemp;
        baTemp.resize(dwLength - channel2.size());
        baTemp.fill(0);
        channel2.append(baTemp);
    }

    // Assemblage des canaux
    QByteArray baData = from2MonoTo1Stereo(channel1, channel2, wBps);

    // Création d'un fichier
    InfoSound info = son1.info;
    info.wBpsFile = wBps;
    info.dwSampleRate = dwSmplRate;
    info.wChannels = 2;
    exporter(fileName, baData, info);
}

////////////////////////////////////////////////////////////////////////////
//////////////////////////    METHODES PRIVEES    //////////////////////////
////////////////////////////////////////////////////////////////////////////

Sound::FileType Sound::getFileType()
{
    QFileInfo fileInfo = this->fileName;
    QString ext = fileInfo.suffix().toLower();

    QString extCustom = "";
    typedef QString (*MyPrototype)();
    MyPrototype myFunction = (MyPrototype) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                             "getExtension");
    if (myFunction)
        extCustom = myFunction();

    if (ext.compare("sf2") == 0)
        return fileSf2;
    else if (ext.compare("wav") == 0)
        return fileWav;
    else if (ext.compare(extCustom) == 0 && !extCustom.isEmpty())
        return fileCustom1;
    else
        return fileUnknown;
}

void Sound::getInfoSound()
{
    // Chargement des caractéristiques du sample
    switch (this->getFileType())
    {
    case fileWav:
        getInfoSoundWav();
        break;
    case fileSf2:
        this->info.wChannel = 0;
        this->info.wChannels = 1;
        break;
    case fileCustom1:{
        typedef bool (*GetInfoSound)(QString, InfoSound*);
        GetInfoSound getInfoSound = (GetInfoSound) QLibrary::resolve(QCoreApplication::applicationDirPath() + "/extension_lecture",
                                                                 "getInfoSound");
        if (getInfoSound)
        {
            if (!getInfoSound(this->fileName, &this->info))
            {
                this->info.wFormat = 0;
                this->info.dwLength = 0;
                this->info.wChannel = 0;
                this->info.wChannels = 0;
                this->info.dwStart = 0;
                this->info.dwStart2 = 0;
                this->info.dwSampleRate = 0;
                this->info.dwStartLoop = 0;
                this->info.dwEndLoop = 0;
                this->info.dwNote = 0;
                this->info.iCent = 0;
                this->info.wBpsFile = 0;
            }
        }
        }break;
    default:
        this->info.wFormat = 0;
        this->info.dwLength = 0;
        this->info.wChannel = 0;
        this->info.wChannels = 0;
        this->info.dwStart = 0;
        this->info.dwStart2 = 0;
        this->info.dwSampleRate = 0;
        this->info.dwStartLoop = 0;
        this->info.dwEndLoop = 0;
        this->info.dwNote = 0;
        this->info.iCent = 0;
        this->info.wBpsFile = 0;
    }
}
void Sound::getInfoSoundWav()
{
    // Caractéristiques d'un fichier wav
    info.wFormat = 0;
    info.dwLength = 0;
    info.dwSampleRate = 0;
    info.dwStart = 0;
    info.wBpsFile = 0;
    info.wChannels = 0;
    info.dwStartLoop = 0;
    info.dwEndLoop = 0;
    info.dwNote = 0;
    info.iCent = 0;
    QFile fi(fileName);
    if (!fi.exists())
    {
        QMessageBox::warning(NULL, QObject::tr("Attention"),
                             QString::fromUtf8(QObject::tr("Impossible d'ouvrir le fichier").toStdString().c_str()));
        return;
    }
    fi.open(QFile::ReadOnly | QFile::Unbuffered);
    QByteArray baData = fi.readAll();
    fi.close();
    this->getInfoSoundWav(baData);
}
void Sound::getInfoSoundWav(QByteArray baData)
{
    int taille, taille2, pos;
    if (strcmp("RIFF", baData.left(4)))
    {
        QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Le fichier est corrompu."));
        return;
    }
    // Taille totale du fichier - 8 octets
    taille = readDWORD(baData, 4);
    if (taille == 0)
    {
        QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Le fichier est corrompu."));
        return;
    }
    taille = taille + 8;
    if (strcmp("WAVE", baData.mid(8, 4)))
    {
        QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Le fichier est corrompu."));
        return;
    }
    pos = 12;
    while (pos < baData.size()-8)
    {
        if (!strcmp("fmt ", baData.mid(pos, 4)))
        {
            pos += 4;
            // informations concernant le signal audio
            taille2 = readDWORD(baData, pos);
            pos += 4;
            if (taille2 < 16 || taille2 > 40)
            {
                QMessageBox::warning(NULL, QObject::tr("Attention"), QObject::tr("Le fichier est corrompu."));
                return;
            }
            info.wFormat = readWORD(baData, pos);
            info.wChannels = readWORD(baData, pos + 2);
            info.dwSampleRate = readDWORD(baData, pos + 4);
            info.wBpsFile = readWORD(baData, pos + 14);
            if (info.wBpsFile < 16)
                QMessageBox::warning(NULL, QObject::tr("Attention"),
                                     QString::fromUtf8(QObject::tr("Résolution insuffisante").toStdString().c_str()));
            pos += taille2;
        }
        else if (!strcmp("smpl", baData.mid(pos, 4)))
        {
            pos += 4;
            taille2 = readDWORD(baData, pos);
            pos += 4;
            // informations sur le sample
            if (taille2 >= 36)
            {
                // numéro note
                info.dwNote = readDWORD(baData, pos + 12);
                if (info.dwNote > 127)
                    info.dwNote = 127;
                // accordage
                info.iCent = (int)readDWORD(baData, pos + 16);
                info.iCent = (double)info.iCent / 2147483648. * 50. + 0.5;
            }
            if (taille2 >= 60)
            {
                // boucle
                info.dwStartLoop = readDWORD(baData, pos + 44);
                info.dwEndLoop = readDWORD(baData, pos + 48) + 1;
            }
            pos += taille2;
        }
        else if (!strcmp("data", baData.mid(pos, 4)))
        {
            pos += 4;
            taille2 = readDWORD(baData, pos);
            pos += 4;
            if (taille2 == 0) taille2 = baData.size() - pos;
            info.dwStart = pos;
            if (info.wBpsFile != 0 && info.wChannels != 0)
                info.dwLength = qMin(taille2, baData.size() - pos) / (info.wBpsFile * info.wChannels / 8);
            pos += taille2;
        }
        else
        {
            pos += 4;
            // On saute la section
            taille2 = readDWORD(baData, pos);
            pos += taille2 + 4;
        }
    }
}

QByteArray Sound::getDataSf2(QFile * fi, WORD byte)
{
    QByteArray baRet;
    // Fichier sf2
    switch (byte)
    {
    case 1:
        baRet.resize(this->info.dwLength);
        // Chargement 8 bits poids faible
        if (this->info.wBpsFile >= 24 && this->info.dwLength)
        {
            // Copie des données
            fi->seek(this->info.dwStart2);  // saut
            baRet = fi->read(this->info.dwLength);
        }
        else
        {
            // Remplissage de 0
            baRet.fill(0);
        }
        break;
    case 2:
        baRet.resize(this->info.dwLength*2);
        // Chargement 16 bits poids fort
        if (this->info.dwLength)
        {
            // Copie des données
            fi->seek(this->info.dwStart);  // saut
            baRet = fi->read(this->info.dwLength*2);
        }
        else
        {
            // remplissage de 0
            baRet.fill(0);
        }
        break;
    case 3:
        baRet.resize(this->info.dwLength*3);
        // Chargement des 24 bits
        if (this->info.dwLength)
        {
            char * data = baRet.data();
            QByteArray baTmp;

            // Assemblage des données, partie 16 bits
            fi->seek(this->info.dwStart);  // saut
            baTmp.resize(this->info.dwLength*2);
            baTmp = fi->read(this->info.dwLength*2);
            const char * dataTmp = baTmp.constData();
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[3*i+1] = dataTmp[2*i];
                data[3*i+2] = dataTmp[2*i+1];
            }
            // Partie 24 bits
            if (this->info.wBpsFile >= 24)
            {
                fi->seek(this->info.dwStart2);  // saut
                baTmp.resize(this->info.dwLength);
                baTmp = fi->read(this->info.dwLength);
                dataTmp = baTmp.constData();
                for (DWORD i = 0; i < this->info.dwLength; i++)
                    data[3*i] = dataTmp[i];
            }
            else
            {
                // Remplissage de 0
                for (DWORD i = 0; i < this->info.dwLength; i++)
                    data[3*i] = 0;
            }
        }
        else
        {
            // remplissage de 0
            baRet.fill(0);
        }
        break;
    case 4:
        baRet.resize(this->info.dwLength*4);
        // Chargement des 24 bits
        if (this->info.dwLength)
        {
            char * data = baRet.data();
            QByteArray baTmp;

            // Assemblage des données, partie 16 bits
            fi->seek(this->info.dwStart);  // saut
            baTmp.resize(this->info.dwLength*2);
            baTmp = fi->read(this->info.dwLength*2);
            const char * dataTmp = baTmp.constData();
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[4*i+2] = dataTmp[2*i];
                data[4*i+3] = dataTmp[2*i+1];
            }
            // Partie 24 bits
            if (this->info.wBpsFile >= 24)
            {
                fi->seek(this->info.dwStart2);  // saut
                baTmp.resize(this->info.dwLength);
                baTmp = fi->read(this->info.dwLength);
                dataTmp = baTmp.constData();
                for (DWORD i = 0; i < this->info.dwLength; i++)
                    data[4*i+1] = dataTmp[i];
            }
            else
            {
                // Remplissage de 0
                for (DWORD i = 0; i < this->info.dwLength; i++)
                    data[4*i+1] = 0;
            }
            // Partie 32 bits
            for (DWORD i = 0; i < this->info.dwLength; i++)
                data[4*i] = 0;
        }
        else
        {
            // remplissage de 0
            baRet.fill(0);
        }
        break;
    }
    return baRet;
}
QByteArray Sound::getDataWav(QFile * fi, WORD byte)
{
    // Fichier wav
    int byte_fichier = this->info.wBpsFile / 8;
    // Saut de l'entête + positionnement sur le canal
    fi->seek(this->info.dwStart + this->info.wChannel * byte_fichier);
    // Lecture des données
    QByteArray baTmp = fi->read(this->info.dwLength * byte_fichier * this->info.wChannels);
    return this->getDataWav(baTmp, byte);
}
QByteArray Sound::getDataWav(QByteArray baData, WORD byte)
{
    // Fichier wav
    QByteArray baRet;
    baRet.resize(this->info.dwLength * byte);
    char * data = baRet.data();
    int a, b;
    int byte_fichier = this->info.wBpsFile / 8;
    if (info.wFormat == 3 && byte_fichier == 4)
    {
        // conversion WAVE_FORMAT_IEEE_FLOAT -> PCM 32
        float * dataF = (float *)baData.data();
        qint32 * data32 = (qint32 *)baData.data();
        for (int i = 0; i < baData.size() / 4; i++)
            data32[i] = (qint32) (dataF[i] * 2147483647);
    }
    const char * dataTmp = baData.constData();
    b = this->info.wChannels * byte_fichier;
    // remplissage data
    switch (byte)
    {
    case 1:
        // 8 bits (après 16)
        if (byte_fichier < 3)
            baRet.fill(0); // Résolution insuffisante, remplissage de zéros
        else
        {
            a = byte_fichier - 3;
            for (DWORD i = 0; i < this->info.dwLength; i++)
                data[i] = dataTmp[a + b * i];
        }
        break;
    case 2:
        // 16 bits
        if (byte_fichier < 2)
        {
            // Résolution insuffisante
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = dataTmp[b * i];
            }
        }
        else
        {
            a = byte_fichier - byte;
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = dataTmp[a + b * i];
                data[byte*i+1] = dataTmp[1 + a + b * i];
            }
        }
        break;
    case 3:
        // 24 bits
        if (byte_fichier == 1)
        {
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = 0;
                data[byte*i+2] = dataTmp[b * i];
            }
        }
        else if (byte_fichier == 2)
        {
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = dataTmp[b * i];
                data[byte*i+2] = dataTmp[1 + b * i];
            }
        }
        else
        {
            a = byte_fichier - byte;
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = dataTmp[a + b * i];
                data[byte*i+1] = dataTmp[1 + a + b * i];
                data[byte*i+2] = dataTmp[2 + a + b * i];
            }
        }
        break;
    case 4:
        // 32 bits
        if (byte_fichier == 1)
        {
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = 0;
                data[byte*i+2] = 0;
                data[byte*i+3] = dataTmp[b * i];
            }
        }
        else if (byte_fichier == 2)
        {
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = 0;
                data[byte*i+2] = dataTmp[b * i];
                data[byte*i+3] = dataTmp[1 + b * i];
            }
        }
        else if (byte_fichier == 3)
        {
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = 0;
                data[byte*i+1] = dataTmp[b * i];
                data[byte*i+2] = dataTmp[1 + b * i];
                data[byte*i+3] = dataTmp[2 + b * i];
            }
        }
        else
        {
            a = byte_fichier - byte;
            for (DWORD i = 0; i < this->info.dwLength; i++)
            {
                data[byte*i] = dataTmp[a + b * i];
                data[byte*i+1] = dataTmp[1 + a + b * i];
                data[byte*i+2] = dataTmp[2 + a + b * i];
                data[byte*i+3] = dataTmp[3 + a + b * i];
            }
        }
        break;
    }
    return baRet;
}

void Sound::exporter(QString fileName, QByteArray baData, InfoSound info)
{
    // Création d'un fichier
    QFile fi(fileName);
    if (!fi.open(QIODevice::WriteOnly)) return;
    // Ecriture
    quint32 dwTemp;
    quint16 wTemp;
    QDataStream out(&fi);
    out.setByteOrder(QDataStream::LittleEndian);
    quint32 dwTailleFmt = 18;
    quint32 dwTailleSmpl = 60;
    quint32 dwLength = baData.size();
    dwTemp = dwLength + dwTailleFmt + dwTailleSmpl + 12 + 8 + 8;
    // Entete
    out.writeRawData("RIFF", 4);
    out << dwTemp;
    out.writeRawData("WAVE", 4);
    ///////////// BLOC FMT /////////////
    out.writeRawData("fmt ", 4);
    out << dwTailleFmt;
    // Compression code
    wTemp = 1; // no compression
    out << wTemp;
    // Number of channels
    wTemp = info.wChannels;
    out << wTemp;
    // Sample rate
    out << (quint32)info.dwSampleRate;
    // Average byte per second
    dwTemp = info.dwSampleRate * info.wChannels * info.wBpsFile / 8;
    out << dwTemp;
    // Block align
    wTemp = info.wChannels * info.wBpsFile / 8;
    out << wTemp;
    // Significants bits per smpl
    out << info.wBpsFile;
    // Extra format bytes
    wTemp = 0;
    out << wTemp;
    ///////////// BLOC SMPL /////////////
    out.writeRawData("smpl", 4);
    out << dwTailleSmpl;
    dwTemp = 0;
    out << dwTemp; // manufacturer
    out << dwTemp; // product
    dwTemp = 1000000000 / info.dwSampleRate;
    out << dwTemp; // smpl period
    // Note et correction
    if (info.iCent > 50)
    {
        dwTemp = qMin((DWORD)127, info.dwNote + 1);
        out << dwTemp;
        dwTemp = ((double)(info.iCent - 50) / 50.) * 2147483648. + 0.5;
        out << dwTemp;
    }
    else if (info.iCent < -50)
    {
        dwTemp = qMax((DWORD)0, info.dwNote - 1);
        out << dwTemp;
        dwTemp = ((double)(info.iCent + 50) / 50.) * 2147483648. + 0.5;
        out << dwTemp;
    }
    else
    {
        dwTemp = info.dwNote;
        out << dwTemp;
        dwTemp = ((double)info.iCent / 50.) * 2147483648. + 0.5;
        out << dwTemp;
    }
    dwTemp = 0;
    out << dwTemp; // smpte format
    out << dwTemp; // smpte offset
    dwTemp = 1;
    out << dwTemp; // nombre boucles
    dwTemp = 0;
    out << dwTemp; // taille sampler data (après les boucles)
    dwTemp = 1;
    out << dwTemp; // CUE point id
    dwTemp = 0;
    out << dwTemp; // type
    dwTemp = info.dwStartLoop;
    out << dwTemp; // début boucle
    dwTemp = info.dwEndLoop-1;
    out << dwTemp; // fin boucle
    dwTemp = 0;
    out << dwTemp; // fraction
    out << dwTemp; // play count (infinite)
    ///////////// BLOC DATA /////////////
    out.writeRawData("data", 4);
    out << dwLength;
    out.writeRawData(baData.constData(), baData.size());
    // Fermeture du fichier
    fi.close();
}

////////////////////////////////////////////////////////////////////////////
//////////////////////////       UTILITAIRES      //////////////////////////
////////////////////////////////////////////////////////////////////////////

QByteArray Sound::resampleMono(QByteArray baData, double echInit, qint32 echFinal, WORD wBps)
{
    // Paramètres
    double alpha = 3;
    qint32 nbPoints = 10;

    // Préparation signal d'entrée
    baData = bpsConversion(baData, wBps, 32);
    if (echFinal < echInit)
    {
        // Filtre passe bas (voir sinc filter)
        baData = Sound::bandFilter(baData, 32, echInit, echFinal/2, 0, -1);
    }
    qint32 sizeInit = baData.size() / 4;
    qint32 * dataI = (qint32*)baData.data();
    double * data = new double[sizeInit]; // utilisation de new sinon possibilité de dépasser une limite de mémoire
    for (int i = 0; i < sizeInit; i++)
        data[i] = (double)dataI[i] / 2147483648.;

    // Création fenêtre Kaiser-Bessel 2048 points
    double kbdWindow[2048];
    KBDWindow(kbdWindow, 2048, alpha);

    // Nombre de points à trouver
    qint32 sizeFinal = (double)(sizeInit - 1.0) * (double)echFinal / echInit + 1;
    double * dataRet = new double[sizeFinal]; // utilisation de new : même raison

    // Calcul des points par interpolation à bande limitée
    double pos, delta;
    qint32 pos1, pos2;
    double sincCoef[1 + 2 * nbPoints];
    double valMax = 0;
    for (qint32 i = 0; i < sizeFinal; i++)
    {
        // Position à interpoler
        pos = (echInit * i) / (double)echFinal;
        // Calcul des coefs
        for (qint32 j = -nbPoints; j <= nbPoints; j++)
        {
            delta = pos - floor(pos);
            // Calcul sinus cardinal
            sincCoef[j + nbPoints] = sinc(M_PI * ((double)j - delta));
            // Application fenêtre
            delta = (double)(j + nbPoints - delta) / (1 + 2 * nbPoints) * 2048;
            pos1 = qMax(0., qMin(floor(delta), 2047.)) + .5;
            pos2 = qMax(0., qMin(ceil (delta), 2047.)) + .5;
            sincCoef[j + nbPoints] *= kbdWindow[pos1] * (ceil((delta)) - delta)
                    + kbdWindow[pos2] * (1. - ceil((delta)) + delta);
        }
        // Valeur
        dataRet[i] = 0;
        for (int j = qMax(0, (qint32)pos - nbPoints); j <= qMin(sizeInit-1, (qint32)pos + nbPoints); j++)
            dataRet[i] += sincCoef[j - (qint32)pos + nbPoints] * data[j];

        valMax = qMax(valMax, qAbs(dataRet[i]));
    }

    // Passage qint32 et limitation si besoin
    QByteArray baRet;
    baRet.resize(sizeFinal * 4);
    qint32 * dataRetI = (qint32*)baRet.data();
    double coef;
    if (valMax > 1)
        coef = 2147483648. / valMax;
    else
        coef = 2147483648LL;
    for (int i = 0; i < sizeFinal; i++)
        dataRetI[i] = dataRet[i] * coef;

    delete dataRet;
    delete data;

    // Filtre passe bas après resampling
    baRet = Sound::bandFilter(baRet, 32, echFinal, echFinal/2, 0, -1);
    baRet = bpsConversion(baRet, 32, wBps);
    return baRet;
}
QByteArray Sound::bandFilter(QByteArray baData, WORD wBps, double dwSmplRate, double fBas, double fHaut, int ordre)
{
    /******************************************************************************
     ***********************    passe_bande_frequentiel    ************************
     ******************************************************************************
     * But :
     *  - filtre un signal par un filtre passe-bande de Butterworth
     * Entrees :
     *  - baData : tableau contenant les donnees a filtrer
     *  - dwSmplRate : frequence d'echantillonnage du signal
     *  - fHaut : frequence de coupure du passe-haut
     *  - fBas : frequence de coupure du passe-bas
     *  - ordre : ordre du filtre
     * Sorties :
     *  - tableau contenant les donnees apres filtrage
     ******************************************************************************/

    // Paramètres valides ?
    if (!dwSmplRate || (fHaut <= 0 && fBas <= 0) || 2 * fHaut > dwSmplRate || 2 * fBas > dwSmplRate)
    {
        // Controle des fréquences de coupures (il faut que Fc<Fe/2 )
        QMessageBox::information(NULL, QObject::tr("Attention"),
                                 QString::fromUtf8(QObject::tr("Mauvais paramètres dans bandFilter").toStdString().c_str()));
    }
    else
    {
        unsigned long size;
        // Conversion de baData en complexes
        Complex * cpxData;
        cpxData = fromBaToComplex(baData, wBps, size);
        // Calculer la fft du signal
        Complex * fc_sortie_fft = FFT(cpxData, size);
        delete [] cpxData;
        // Convoluer par le filtre Butterworth d'ordre 4, applique dans le sens direct et retrograde
        // pour supprimer la phase (Hr4 * H4 = Gr4 * G4 = (G4)^2)
        double d_gain_ph, d_gain_pb;
        if (fHaut <= 0)
        {
            double pos;
            // Filtre passe bas uniquement
            if (ordre == -1)
            {
                // "Mur de brique"
                for (unsigned long i = 0; i < (size+1)/2; i++)
                {
                    pos = i / ((double)size-1);
                    fc_sortie_fft[i] *= (pos * dwSmplRate) < fBas;
                    fc_sortie_fft[size-1-i] *= (pos * dwSmplRate) < fBas;
                }
            }
            else
            {
                for (unsigned long i = 0; i < (size+1)/2; i++)
                {
                    pos = i / ((double)size-1);
                    d_gain_pb = 1.0/(1.0 + pow(pos * dwSmplRate / fBas, 2 * ordre));
                    fc_sortie_fft[i] *= d_gain_pb;
                    fc_sortie_fft[size-1-i] *= d_gain_pb;
                }
            }
        }
        else if (fBas <= 0)
        {
            double pos;
            // Filtre passe haut uniquement
            if (ordre == -1)
            {
                // "Mur de brique"
                for (unsigned long i = 0; i < (size+1)/2; i++)
                {
                    pos = i / ((double)size-1);
                    fc_sortie_fft[i] *= (pos * dwSmplRate) > fHaut;
                    fc_sortie_fft[size-1-i] *= (pos * dwSmplRate) > fHaut;
                }
            }
            else
            {
                for (unsigned long i = 0; i < (size+1)/2; i++)
                {
                    pos = i / ((double)size-1);
                    d_gain_ph = 1 - (1.0 / (1.0 + pow((pos * dwSmplRate) / fHaut, 2 * ordre)));
                    fc_sortie_fft[i] *= d_gain_ph;
                    fc_sortie_fft[size-1-i] *= d_gain_ph;
                }
            }
        }
        else
        {
            double pos;
            // Filtre passe bande
            for (unsigned long i = 0; i < (size+1)/2; i++)
            {
                pos = i / ((double)size-1);
                d_gain_ph = 1 - (1.0 / (1.0 + pow((pos * dwSmplRate) / fHaut, 2 * ordre)));
                d_gain_pb = 1.0/(1.0 + pow(pos * dwSmplRate / fBas, 2 * ordre));
                fc_sortie_fft[i] *= d_gain_ph * d_gain_pb;
                fc_sortie_fft[size-1-i] *= d_gain_ph * d_gain_pb;
            }
        }
        // Calculer l'ifft du signal
        cpxData = IFFT(fc_sortie_fft, size);
        delete [] fc_sortie_fft;
        // Prise en compte du facteur d'echelle
        for (unsigned long i = 0; i < size; i++)
            cpxData[i].real() /= size;
        // Retour en QByteArray
        QByteArray baRet;
        baRet = fromComplexToBa(cpxData, (long int)baData.size() * 8 / wBps, wBps);
        delete [] cpxData;
        return baRet;
    }
    return baData;
}

QByteArray Sound::EQ(QByteArray baData, DWORD dwSmplRate, WORD wBps, int i1, int i2, int i3, int i4, int i5,
                         int i6, int i7, int i8, int i9, int i10)
{
    unsigned long size;
    // Conversion de baData en complexes
    Complex * cpxData;
    cpxData = fromBaToComplex(baData, wBps, size);
    // Calculer la fft du signal
    Complex * fc_sortie_fft = FFT(cpxData, size);
    delete [] cpxData;
    // Filtrage
    double freq;
    double gain;
    for (unsigned long i = 0; i < (size+1)/2; i++)
    {
        freq = (double)(i * dwSmplRate) / (size-1);
        gain = gainEQ(freq, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10);
        fc_sortie_fft[i] *= gain;
        fc_sortie_fft[size-1-i] *= gain;
    }
    // Calculer l'ifft du signal
    cpxData = IFFT(fc_sortie_fft, size);
    delete [] fc_sortie_fft;
    // Prise en compte du facteur d'echelle
    for (unsigned long i = 0; i < size; i++)
        cpxData[i].real() /= size;
    // Retour en QByteArray
    QByteArray baRet;
    baRet = fromComplexToBa(cpxData, baData.size() * 8 / wBps, wBps);
    delete [] cpxData;
    return baRet;
}

Complex * Sound::FFT(Complex * x, int N)
{
    Complex* out = new Complex[N];
    Complex* scratch = new Complex[N];
    Complex* twiddles = new Complex [N];
    int k;
    for (k = 0; k != N; ++k)
    {
        twiddles[k].real() = cos(-2.0*PI*k/N);
        twiddles[k].imag() = sin(-2.0*PI*k/N);
    }
    FFT_calculate(x, N, out, scratch, twiddles);
    delete [] twiddles;
    delete [] scratch;
    return out;
}
Complex * Sound::IFFT(Complex * x, int N)
{
    Complex * out = new Complex[N];
    Complex * scratch = new Complex[N];
    Complex * twiddles = new Complex [N];
    int k;
    for (k = 0; k != N; ++k)
    {
        twiddles[k].real() = cos(2.0*PI*k/N);
        twiddles[k].imag() = sin(2.0*PI*k/N);
    }
    FFT_calculate(x, N, out, scratch, twiddles);
    delete [] twiddles;
    delete [] scratch;
    return out;
}
void Sound::bpsConversion(char *cDest, const char *cFrom, qint32 size, WORD wBpsInit, WORD wBpsFinal, bool bigEndian)
{
    // Conversion entre formats 32, 24 et 16 bits
    // Particularité : demander format 824 bits renvoie les 8 bits de poids faible
    //                 dans les 24 bits de poids fort

    // Remplissage
    switch (wBpsInit)
    {
    case 8:
        switch (wBpsFinal)
        {
        case 824:
            // Remplissage de 0
            for (int i = 0; i < size; i++)
                cDest[i] = 0;
            break;
        case 8:
            for (int i = 0; i < size; i++)
                cDest[i] = cFrom[i];
            break;
        case 16:
            if (bigEndian)
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[2*i+1] = 0;
                    cDest[2*i] = cFrom[i];
                }
            }
            else
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[2*i] = 0;
                    cDest[2*i+1] = cFrom[i];
                }
            }
            break;
        case 24:
            if (bigEndian)
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[3*i+2] = 0;
                    cDest[3*i+1] = 0;
                    cDest[3*i] = cFrom[i];
                }
            }
            else
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[3*i] = 0;
                    cDest[3*i+1] = 0;
                    cDest[3*i+2] = cFrom[i];
                }
            }
            break;
        case 32:
            if (bigEndian)
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[4*i+3] = 0;
                    cDest[4*i+2] = 0;
                    cDest[4*i+1] = 0;
                    cDest[4*i] = cFrom[i];
                }
            }
            else
            {
                for (int i = 0; i < size; i++)
                {
                    cDest[4*i] = 0;
                    cDest[4*i+1] = 0;
                    cDest[4*i+2] = 0;
                    cDest[4*i+3] = cFrom[i];
                }
            }
            break;
        }
        break;
    case 16:
        switch (wBpsFinal)
        {
        case 824:
            // Remplissage de 0
            for (int i = 0; i < size/2; i++)
                cDest[i] = 0;
            break;
        case 8:
            for (int i = 0; i < size/2; i++)
                cDest[i] = cFrom[2*i+1];
            break;
        case 16:
            if (bigEndian)
            {
                for (int i = 0; i < size/2; i++)
                {
                    cDest[2*i+1] = cFrom[2*i];
                    cDest[2*i] = cFrom[2*i+1];
                }
            }
            else
                for (int i = 0; i < size; i++)
                    cDest[i] = cFrom[i];
            break;
        case 24:
            if (bigEndian)
            {
                for (int i = 0; i < size/2; i++)
                {
                    cDest[3*i+2] = 0;
                    cDest[3*i+1] = cFrom[2*i];
                    cDest[3*i] = cFrom[2*i+1];
                }
            }
            else
            {
                for (int i = 0; i < size/2; i++)
                {
                    cDest[3*i] = 0;
                    cDest[3*i+1] = cFrom[2*i];
                    cDest[3*i+2] = cFrom[2*i+1];
                }
            }
            break;
        case 32:
            if (bigEndian)
            {
                for (int i = 0; i < size/2; i++)
                {
                    cDest[4*i+3] = 0;
                    cDest[4*i+2] = 0;
                    cDest[4*i+1] = cFrom[2*i];
                    cDest[4*i] = cFrom[2*i+1];
                }
            }
            else
            {
                for (int i = 0; i < size/2; i++)
                {
                    cDest[4*i] = 0;
                    cDest[4*i+1] = 0;
                    cDest[4*i+2] = cFrom[2*i];
                    cDest[4*i+3] = cFrom[2*i+1];
                }
            }
            break;
        }
        break;
    case 24:
        switch (wBpsFinal)
        {
        case 824:
            // 8 bits de poids faible
            for (int i = 0; i < size/3; i++)
                cDest[i] = cFrom[3*i];
            break;
        case 8:
            for (int i = 0; i < size/3; i++)
                cDest[i] = cFrom[3*i+2];
            break;
        case 16:
            if (bigEndian)
            {
                for (int i = 0; i < size/3; i++)
                {
                    cDest[2*i+1] = cFrom[3*i+1];
                    cDest[2*i] = cFrom[3*i+2];
                }
            }
            else
            {
                for (int i = 0; i < size/3; i++)
                {
                    cDest[2*i] = cFrom[3*i+1];
                    cDest[2*i+1] = cFrom[3*i+2];
                }
            }
            break;
        case 24:
            if (bigEndian)
            {
                for (int i = 0; i < size/3; i++)
                {
                    cDest[3*i+2] = cFrom[3*i];
                    cDest[3*i+1] = cFrom[3*i+1];
                    cDest[3*i] = cFrom[3*i+2];
                }
            }
            else
                for (int i = 0; i < size; i++)
                    cDest[i] = cFrom[i];
            break;
        case 32:
            if (bigEndian)
            {
                for (int i = 0; i < size/3; i++)
                {
                    cDest[4*i+3] = 0;
                    cDest[4*i+2] = cFrom[3*i];
                    cDest[4*i+1] = cFrom[3*i+1];
                    cDest[4*i] = cFrom[3*i+2];
                }
            }
            else
            {
                for (int i = 0; i < size/3; i++)
                {
                    cDest[4*i] = 0;
                    cDest[4*i+1] = cFrom[3*i];
                    cDest[4*i+2] = cFrom[3*i+1];
                    cDest[4*i+3] = cFrom[3*i+2];
                }
            }
            break;
        }
        break;
    case 32:
        switch (wBpsFinal)
        {
        case 824:
            // 8 bits poids faible après 16
            for (int i = 0; i < size/4; i++)
                cDest[i] = cFrom[4*i+1];
            break;
        case 8:
            for (int i = 0; i < size/4; i++)
                cDest[i] = cFrom[4*i+3];
            break;
        case 16:
            if (bigEndian)
            {
                for (int i = 0; i < size/4; i++)
                {
                    cDest[2*i+1] = cFrom[4*i+2];
                    cDest[2*i] = cFrom[4*i+3];
                }
            }
            else
            {
                for (int i = 0; i < size/4; i++)
                {
                    cDest[2*i] = cFrom[4*i+2];
                    cDest[2*i+1] = cFrom[4*i+3];
                }
            }
            break;
        case 24:
            if (bigEndian)
            {
                for (int i = 0; i < size/4; i++)
                {
                    cDest[3*i+2] = cFrom[4*i+1];
                    cDest[3*i+1] = cFrom[4*i+2];
                    cDest[3*i] = cFrom[4*i+3];
                }
            }
            else
            {
                for (int i = 0; i < size/4; i++)
                {
                    cDest[3*i] = cFrom[4*i+1];
                    cDest[3*i+1] = cFrom[4*i+2];
                    cDest[3*i+2] = cFrom[4*i+3];
                }
            }
            break;
        case 32:
            if (bigEndian)
            {
                for (int i = 0; i < size/4; i++)
                {
                    cDest[4*i+3] = cFrom[4*i];
                    cDest[4*i+2] = cFrom[4*i+1];
                    cDest[4*i+1] = cFrom[4*i+2];
                    cDest[4*i] = cFrom[4*i+3];
                }
            }
            else
                for (int i = 0; i < size; i++)
                    cDest[i] = cFrom[i];
            break;
        }
        break;
    }
}
QByteArray Sound::bpsConversion(QByteArray baData, WORD wBpsInit, WORD wBpsFinal, bool bigEndian)
{
    // Conversion entre formats 32, 24 et 16 bits
    // Particularité : demander format 824 bits renvoie les 8 bits de poids faible
    //                 dans les 24 bits de poids fort
    int size = baData.size();
    // Données de retour
    QByteArray baRet;
    // Redimensionnement
    int i = 1;
    int j = 1;
    switch (wBpsInit)
    {
    case 16: i = 2; break;
    case 24: i = 3; break;
    case 32: i = 4; break;
    default: i = 1;
    }
    switch (wBpsFinal)
    {
    case 16: j = 2; break;
    case 24: j = 3; break;
    case 32: j = 4; break;
    default: j = 1;
    }
    baRet.resize((size * j) / i);
    // Remplissage
    char * cDest = baRet.data();
    const char * cFrom = baData.constData();
    bpsConversion(cDest, cFrom, size, wBpsInit, wBpsFinal, bigEndian);
    return baRet;
}
QByteArray Sound::from2MonoTo1Stereo(QByteArray baData1, QByteArray baData2, WORD wBps, bool bigEndian)
{
    int size;
    // Si tailles différentes, on complète le petit avec des 0
    if (baData1.size() < baData2.size())
    {
        QByteArray baTmp;
        baTmp.fill(0, baData2.size() - baData1.size());
        baData1.append(baTmp);
    }
    else if (baData2.size() < baData1.size())
    {
        QByteArray baTmp;
        baTmp.fill(0, baData1.size() - baData2.size());
        baData2.append(baTmp);
    }
    size = baData1.size();
    // Assemblage
    QByteArray baRet;
    baRet.resize(2 * size);
    char * cDest = baRet.data();
    const char * cFrom1 = baData1.constData();
    const char * cFrom2 = baData2.constData();
    if (wBps == 32)
    {
        if (bigEndian)
        {
            for (int i = 0; i < size/4; i++)
            {
                cDest[8*i] = cFrom1[4*i+3];
                cDest[8*i+1] = cFrom1[4*i+2];
                cDest[8*i+2] = cFrom1[4*i+1];
                cDest[8*i+3] = cFrom1[4*i];
                cDest[8*i+4] = cFrom2[4*i+3];
                cDest[8*i+5] = cFrom2[4*i+2];
                cDest[8*i+6] = cFrom2[4*i+1];
                cDest[8*i+7] = cFrom2[4*i];
            }
        }
        else
        {
            for (int i = 0; i < size/4; i++)
            {
                cDest[8*i] = cFrom1[4*i];
                cDest[8*i+1] = cFrom1[4*i+1];
                cDest[8*i+2] = cFrom1[4*i+2];
                cDest[8*i+3] = cFrom1[4*i+3];
                cDest[8*i+4] = cFrom2[4*i];
                cDest[8*i+5] = cFrom2[4*i+1];
                cDest[8*i+6] = cFrom2[4*i+2];
                cDest[8*i+7] = cFrom2[4*i+3];
            }
        }
    }
    else if (wBps == 24)
    {
        if (bigEndian)
        {
            for (int i = 0; i < size/3; i++)
            {
                cDest[6*i] = cFrom1[3*i+2];
                cDest[6*i+1] = cFrom1[3*i+1];
                cDest[6*i+2] = cFrom1[3*i];
                cDest[6*i+3] = cFrom2[3*i+2];
                cDest[6*i+4] = cFrom2[3*i+1];
                cDest[6*i+5] = cFrom2[3*i];
            }
        }
        else
        {
            for (int i = 0; i < size/3; i++)
            {
                cDest[6*i] = cFrom1[3*i];
                cDest[6*i+1] = cFrom1[3*i+1];
                cDest[6*i+2] = cFrom1[3*i+2];
                cDest[6*i+3] = cFrom2[3*i];
                cDest[6*i+4] = cFrom2[3*i+1];
                cDest[6*i+5] = cFrom2[3*i+2];
            }
        }
    }
    else
    {
        if (bigEndian)
        {
            for (int i = 0; i < size/2; i++)
            {
                cDest[4*i] = cFrom1[2*i+1];
                cDest[4*i+1] = cFrom1[2*i];
                cDest[4*i+2] = cFrom2[2*i+1];
                cDest[4*i+3] = cFrom2[2*i];
            }
        }
        else
        {
            for (int i = 0; i < size/2; i++)
            {
                cDest[4*i] = cFrom1[2*i];
                cDest[4*i+1] = cFrom1[2*i+1];
                cDest[4*i+2] = cFrom2[2*i];
                cDest[4*i+3] = cFrom2[2*i+1];
            }
        }
    }
    return baRet;
}
Complex * Sound::fromBaToComplex(QByteArray baData, WORD wBps, long unsigned int &size)
{
    Complex * cpxData;
    // Création et remplissage d'un tableau de complexes
    if (wBps == 16)
    {
        qint16 * data = (qint16 *)baData.data();
        // Nombre de données (puissance de 2 la plus proche)
        int nb = ceil(log2(baData.size()/2));
        size = 1;
        for (int i = 0; i < nb; i++) size *= 2;
        // Remplissage
        cpxData = new Complex[size];
        for (int i = 0; i < baData.size()/2; i++)
        {
            cpxData[i].real() = data[i];
            cpxData[i].imag() = 0;
        }
        // On complète avec des 0
        for (unsigned int i = baData.size()/2; i < size; i++)
        {
            cpxData[i].real() = 0;
            cpxData[i].imag() = 0;
        }
    }
    else
    {
        // Passage 32 bits si nécessaire
        if (wBps == 24)
            baData = bpsConversion(baData, 24, 32);
        qint32 * data = (qint32 *)baData.data();
        // Nombre de données (puissance de 2 la plus proche)
        int nb = ceil(log2(baData.size()/4));
        size = 1;
        for (int i = 0; i < nb; i++) size *= 2;
        // Remplissage
        cpxData = new Complex[size];
        for (int i = 0; i < baData.size()/4; i++)
        {
            cpxData[i].real() = data[i];
            cpxData[i].imag() = 0;
        }
        // On complète avec des 0
        for (unsigned int i = baData.size()/4; i < size; i++)
        {
            cpxData[i].real() = 0;
            cpxData[i].imag() = 0;
        }
    }
    return cpxData;
}
QByteArray Sound::fromComplexToBa(Complex * cpxData, int size, WORD wBps)
{
    QByteArray baData;
    if (wBps == 16)
    {
        // Calcul du maximum
        quint64 valMax = 0;
        for (int i = 0; i < size; i++)
            valMax = qMax(valMax, (quint64)qAbs(cpxData[i].real()));
        // Atténuation si dépassement de la valeur max
        double att = 1;
        if (valMax > 32700)
            att = 32700. / valMax;
        // Conversion qint16
        baData.resize(size*2);
        qint16 * data = (qint16 *)baData.data();
        for (int i = 0; i < size; i++)
            data[i] = (qint16)(cpxData[i].real() * att);
    }
    else
    {
        // Calcul du maximum
        quint64 valMax = 0;
        for (int i = 0; i < size; i++)
            valMax = qMax(valMax, (quint64)qAbs(cpxData[i].real()));
        // Atténuation si dépassement de la valeur max
        double att = 1;
        if (valMax > 2147483000)
            att = 2147483000. / valMax;
        // Conversion qint32
        baData.resize(size*4);
        qint32 * data = (qint32 *)baData.data();
        for (int i = 0; i < size; i++)
            data[i] = (qint32)(cpxData[i].real() * att);
        // Conversion 24 bits
        if (wBps == 24)
            baData = bpsConversion(baData, 32, 24);
    }
    return baData;
}

QByteArray Sound::normaliser(QByteArray baData, double dVal, WORD wBps, double &db)
{
    // Conversion 32 bits si nécessaire
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    // Calcul valeur max
    qint32 * data = (qint32 *)baData.data();
    qint32 valMax = 0;
    for (int i = 0; i < baData.size()/4; i++) valMax = qMax(valMax, qAbs(data[i]));
    // Calcul amplification
    double mult = dVal * (double)2147483648LL / valMax;
    db = 20.0 * log10(mult);
    // Amplification
    for (int i = 0; i < baData.size()/4; i++) data[i] *= mult;
    // Conversion format d'origine si nécessaie
    if (wBps != 32)
        baData = bpsConversion(baData, 32, wBps);
    return baData;
}
QByteArray Sound::multiplier(QByteArray baData, double dMult, WORD wBps, double &db)
{
    // Conversion 32 bits si nécessaire
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    // Calcul amplification
    db = 20.0 * log10(dMult);
    // Amplification
    qint32 * data = (qint32 *)baData.data();
    for (int i = 0; i < baData.size()/4; i++) data[i] *= dMult;
    // Conversion format d'origine si nécessaie
    if (wBps != 32)
        baData = bpsConversion(baData, 32, wBps);
    return baData;
}
QByteArray Sound::enleveBlanc(QByteArray baData, double seuil, WORD wBps, quint32 &pos)
{
    // Conversion 32 bits si nécessaire
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    // Calcul de la moyenne des valeurs absolues
    qint32 * data = (qint32 *)baData.data();
    QByteArray baData2; baData2.resize(baData.size());
    qint32 * data2 = (qint32 *)baData2.data();
    for (int i = 0; i < baData.size()/4; i++) data2[i] = qAbs(data[i]);
    qint32 median = mediane(baData2, 32);
    // Calcul du nombre d'éléments   sauter
    while ((signed)pos < baData.size()/4 - 1 && (data2[pos] < seuil * median)) pos++;
    // Saut
    if ((signed)pos < baData.size()/4 - 1)
        baData = baData.mid(pos * 4, baData.size() - 4 * pos);
    // Conversion format d'origine si nécessaie
    if (wBps != 32)
        baData = bpsConversion(baData, 32, wBps);
    return baData;
}
void Sound::regimePermanent(QByteArray baData, DWORD dwSmplRate, WORD wBps, qint32 &posStart, qint32 &posEnd)
{
    // Recherche d'un régiment permanent (sans attaque ni release)
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 size = baData.size() / 4;
    // Recherche fine
    regimePermanent(baData, dwSmplRate, 32, posStart, posEnd, 10, 1.05);
    if (posEnd - posStart < size/2)
    {
        // Recherche grossière
        regimePermanent(baData, dwSmplRate, 32, posStart, posEnd, 7, 1.2);
        if (posEnd - posStart < size / 2)
        {
            // Recherche très grossière
            regimePermanent(baData, dwSmplRate, 32, posStart, posEnd, 4, 1.35);
            if (posEnd - posStart < size / 2)
            {
                // moitié du milieu
                posStart = size / 4;
                posEnd = size * 3 / 4;
            }
        }
    }
}
QByteArray Sound::correlation(QByteArray baData, DWORD dwSmplRate, WORD wBps, qint32 fMin, qint32 fMax, qint32 &dMin)
{
    if (wBps != 16)
        baData = bpsConversion(baData, wBps, 16);
    // Décalage max (fréquence basse)
    qint32 dMax = dwSmplRate / fMin;
    // Décalage min (fréquence haute)
    dMin = dwSmplRate / fMax;
    // Calcul de la corrélation
    QByteArray baCorrel;
    baCorrel.resize(2*(dMax - dMin + 1));
    qint16 * dataCorrel = (qint16 *)baCorrel.data();
    const qint16 * data = (const qint16 *)baData.constData();
    qint64 qTmp;
    if (dMax >= baData.size() / 2)
        baCorrel.fill(0);
    else
    {
        for (int i = dMin; i <= dMax; i++)
        {
            // Ressemblance
            qTmp = 0;
            for (int j = 0; j < baData.size() / 2 - dMax; j++)
                qTmp += (qint64)qAbs(data[j] - data[j+i]);
            dataCorrel[i - dMin] = qTmp / (baData.size() / 2 - dMax);
        }
    }
    return baCorrel;
}
qint32 Sound::correlation(QByteArray baData1, QByteArray baData2, WORD wBps)
{
    if (baData1.size() != baData2.size() || baData1.size() == 0)
        return 0;
    if (wBps != 32)
    {
        baData1 = bpsConversion(baData1, wBps, 32);
        baData2 = bpsConversion(baData2, wBps, 32);
    }
    qint32 * data1 = (qint32 *)baData1.data();
    qint32 * data2 = (qint32 *)baData2.data();
    quint64 somme = 0;
    // Mesure ressemblance
    for (int i = 0; i <= baData1.size()/4; i++)
        somme += qAbs(data1[i] - data2[i]);
    // Normalisation et retour
    somme /= baData1.size() / 4;
    return 2147483647 - somme;
}
QByteArray Sound::bouclage(QByteArray baData, DWORD dwSmplRate, qint32 &loopStart, qint32 &loopEnd, WORD wBps)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 * data = (qint32 *)baData.data();
    // Recherche du régime permament
    qint32 posStart = loopStart;
    if (posStart == loopEnd || loopEnd - posStart < (signed)dwSmplRate)
        regimePermanent(baData, dwSmplRate, 32, posStart, loopEnd);
    if (loopEnd - posStart < (signed)dwSmplRate)
        return QByteArray();
    // Extraction du segment B de 0.05s   la fin du régime permanent
    qint32 longueurSegmentB = 0.05 * dwSmplRate;
    QByteArray segmentB = baData.mid(4 * (loopEnd - longueurSegmentB), 4 * longueurSegmentB);
    // Calcul des corrélations
    qint32 nbCor = (loopEnd - posStart) / 2 - 2 * longueurSegmentB;
    QByteArray baCorrelations;
    baCorrelations.resize(nbCor * 4);
    qint32 * correlations = (qint32 *)baCorrelations.data();
    for (int i = 0; i < nbCor; i++)
        correlations[i] = correlation(segmentB, baData.mid(4 * (i + longueurSegmentB + posStart), 4 * longueurSegmentB), 32);
    // Recherche des meilleures corrélations (pour l'instant : utilisation de la 1ère uniquement)
    quint32 * meilleuresCorrel = findMax(baCorrelations, 32, 1, 0.9);
    // calcul de posStartLoop
    qint32 posStartLoop = 2 * longueurSegmentB + meilleuresCorrel[0] + posStart;
    // Longueur du crossfade pour bouclage (augmente avec l'incohérence)
    qint32 longueurBouclage = qMin(meilleuresCorrel[0] + 2 * longueurSegmentB,
                                   (quint32)floor(dwSmplRate*4*(double)(2147483647-baCorrelations[meilleuresCorrel[0]])/2147483647));
    // Détermination des coefficients pour crossfade
    double cr1[longueurBouclage];
    double cr2[longueurBouclage];
    for (int i = 0; i < longueurBouclage;i++)
    {
        cr1[i] = (double)i/(longueurBouclage-1);
        cr2[i] = 1.0 - cr1[i];
    }
    // Bouclage
    for (int i = 0; i < longueurBouclage; i++)
    {
        data[loopEnd - longueurBouclage + i] =
                cr2[i] * data[loopEnd - longueurBouclage + i] +
                cr1[i] * data[posStartLoop - longueurBouclage + i];
    }
    baData = baData.left(loopEnd * 4);
    data = (qint32*)baData.data();
    // Ajout de 8 valeurs
    QByteArray baTmp;
    baTmp.resize(4*8);
    qint32 * dataTmp = (qint32 *)baTmp.data();
    for (int i = 0; i < 8; i++)
        dataTmp[i] = data[posStartLoop+i];
    baData.append(baTmp);
    // Modification de loopStart et renvoi des données
    loopStart = posStartLoop;
    if (wBps != 32)
        baData = bpsConversion(baData, 32, wBps);
    return baData;
}
QByteArray Sound::sifflements(QByteArray baData, DWORD dwSmplRate, WORD wBps, double fCut, double fMax, double raideur)
{
    // Conversion 32 bits si nécessaire
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    // Elimination des fréquences hautes du son
    unsigned long size;
    // Conversion de baData en complexes
    Complex * cpxData;
    cpxData = fromBaToComplex(baData, 32, size);
    // Calculer la fft du signal
    Complex * fc_sortie_fft = FFT(cpxData, size);
    delete [] cpxData;
    double pos;
    // Ajustement raideur
    raideur += 1;
    // Module carré maxi de la FFT
    double moduleMax = 0;
    for (unsigned long i = 0; i < (size+1)/2; i++)
    {
        pos = i / ((double)size-1.0);
        if (pos * dwSmplRate < fCut)
        {
            double module = sqrt(fc_sortie_fft[i].imag() * fc_sortie_fft[i].imag() +
                            fc_sortie_fft[i].real() * fc_sortie_fft[i].real());
            moduleMax = qMax(moduleMax, module);
            module = sqrt(fc_sortie_fft[size-1-i].imag() * fc_sortie_fft[size-1-i].imag() +
                          fc_sortie_fft[size-1-i].real() * fc_sortie_fft[size-1-i].real());
            moduleMax = qMax(moduleMax, module);
        }
    }
    for (unsigned long i = 0; i < (size+1)/2; i++)
    {
        pos = i / ((double)size-1);
        if (pos * dwSmplRate > fMax)
        {
            fc_sortie_fft[i] *= 0;
            fc_sortie_fft[size-1-i] *= 0;
        }
        else if (pos * dwSmplRate > fCut)
        {
            double module = sqrt(fc_sortie_fft[i].imag() * fc_sortie_fft[i].imag() +
                                 fc_sortie_fft[i].real() * fc_sortie_fft[i].real());
            double moduleMaxPos = (1.0 - (double)(pos * dwSmplRate - fCut) / (fMax - fCut));
            moduleMaxPos = moduleMax * pow(moduleMaxPos, raideur);
            if (module > moduleMaxPos && module != 0)
            {
                fc_sortie_fft[i] *= moduleMaxPos / module;
                fc_sortie_fft[size-1-i] *= moduleMaxPos / module;
            }
        }
    }

    // Calculer l'ifft du signal
    cpxData = IFFT(fc_sortie_fft, size);
    delete [] fc_sortie_fft;
    // Prise en compte du facteur d'echelle
    for (unsigned long i = 0; i < size; i++)
        cpxData[i].real() /= size;
    // Retour en QByteArray
    QByteArray baRet = fromComplexToBa(cpxData, (long int)baData.size() * 8 / 32, 32);
    delete [] cpxData;
    // retour wBps si nécessaire
    if (wBps != 32)
        baRet = bpsConversion(baRet, 32, wBps);
    return baRet;
}

quint32 * Sound::findMins(QByteArray baCorrel, WORD wBps, int nb, double minFrac)
{
    // recherche des pics
    if (wBps != 32)
        baCorrel = bpsConversion(baCorrel, wBps, 32);
    // Calcul mini maxi
    qint32 * data = (qint32 *)baCorrel.data();
    qint32 mini, maxi;
    mini = data[0];
    maxi = data[0];
    for (qint32 i = 1; i < baCorrel.size() / 4; i++)
    {
        if (data[i] < mini) mini = data[i];
        if (data[i] > maxi) maxi = data[i];
    }
    // Valeur à ne pas dépasser
    qint32 valMax = maxi - minFrac * (maxi - mini);
    // Recherche de l'indice des premiers grands pics
    quint32 * indices = new quint32[nb];
    qint32 i = 1;
    int pos = 0;
    while ((i < baCorrel.size() / 4 - 1) && pos < nb)
    {
        if (data[i-1] > data[i] && data[i+1] > data[i] && data[i] < valMax)
        {
            // Présence d'un pic
            indices[pos] = i;
            pos++;
        }
        i++;
    }
    // On complète par des 0
    for (int i = pos; i < nb; i++)
        indices[i] = 0;
    return indices;
}
quint32 * Sound::findMax(QByteArray baData, WORD wBps, int nb, double minFrac)
{
    // recherche des pics
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    // Calcul mini maxi
    qint32 * data = (qint32 *)baData.data();
    qint32 mini, maxi;
    mini = data[0];
    maxi = data[0];
    for (qint32 i = 1; i < baData.size() / 4; i++)
    {
        if (data[i] < mini) mini = data[i];
        if (data[i] > maxi) maxi = data[i];
    }
    // Valeur à dépasser
    qint32 valMin = mini + minFrac * (maxi - mini);
    // Recherche de l'indice des premiers grands pics
    quint32 * indices = new quint32[nb];
    qint32 * valeurs = new qint32[nb];
    for (int i = 0; i < nb; i++)
    {
        indices[i] = 0;
        valeurs[i] = 0;
    }
    int pos;
    for (int i = 1; i < baData.size()/4 - 1; i++)
    {
        if (data[i-1] < data[i] && data[i+1] < data[i] && data[i] > valMin)
        {
            // Présence d'un pic
            pos = 0;
            while (valeurs[pos] > data[i] && pos < nb - 1) pos++;
            if (valeurs[pos] > data[i]) pos++;
            if (pos < nb)
            {
                // Insertion du pic
                for (int j = nb - 2; j >= pos; j--)
                {
                    indices[j+1] = indices[j];
                    valeurs[j+1] = valeurs[j];
                }
                indices[pos] = i;
                valeurs[pos] = data[i];
            }
            i++;
        }
    }
    delete [] valeurs;
    return indices;
}
qint32 Sound::max(QByteArray baData, WORD wBps)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 * data = (qint32 *)baData.data();
    qint32 maxi = data[0];
    for (int i = 1; i < baData.size()/4; i++)
    {
        if (data[i] > maxi)
            maxi = data[i];
    }
    return maxi;
}

// UTILITAIRES, PARTIE PRIVEE

void Sound::FFT_calculate(Complex * x, long N /* must be a power of 2 */,
        Complex * X, Complex * scratch, Complex * twiddles)
{
    int k, m, n;
    int skip;
    bool evenIteration = N & 0x55555555;
    Complex* E;
    Complex* Xp, * Xp2, * Xstart;
    if (N == 1)
    {
        X[0] = x[0];
        return;
    }
    E = x;
    for (n = 1; n < N; n *= 2)
    {
        Xstart = evenIteration? scratch : X;
        skip = N/(2 * n);
        /* each of D and E is of length n, and each element of each D and E is
        separated by 2*skip. The Es begin at E[0] to E[skip - 1] and the Ds
        begin at E[skip] to E[2*skip - 1] */
        Xp = Xstart;
        Xp2 = Xstart + N/2;
        for(k = 0; k != n; k++)
        {
            double tim = twiddles[k * skip].imag();
            double tre = twiddles[k * skip].real();
            for (m = 0; m != skip; ++m)
            {
                Complex* D = E + skip;
                /* twiddle *D to get dre and dim */
                double dre = D->real() * tre - D->imag() * tim;
                double dim = D->real() * tim + D->imag() * tre;
                Xp->real() = E->real() + dre;
                Xp->imag() = E->imag() + dim;
                Xp2->real() = E->real() - dre;
                Xp2->imag() = E->imag() - dim;
                ++Xp;
                ++Xp2;
                ++E;
            }
            E += skip;
        }
        E = Xstart;
        evenIteration = !evenIteration;
    }
}
double Sound::moyenne(QByteArray baData, WORD wBps)
{
    if (baData.size())
        return somme(baData, wBps) / (baData.size() / (wBps/8));
    else
        return 0;
}
double Sound::moyenneCarre(QByteArray baData, WORD wBps)
{
    //return sommeAbs(baData, wBps) / (baData.size() / (wBps/8));
    return (double)qSqrt(sommeCarre(baData, wBps)) / (baData.size() / (wBps/8));
}
qint32 Sound::mediane(QByteArray baData, WORD wBps)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 n = baData.size() / 4;
    qint32 * arr = (qint32 *)baData.data();
    qint32 low, high ;
    qint32 median;
    qint32 middle, ll, hh;
    qint32 qTmp;
    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;)
    {
        if (high <= low) // One element only
            return arr[median] ;

        if (high == low + 1)
        {  // Two elements only
            if (arr[low] > arr[high])
            {
                qTmp = arr[low];
                arr[low] = arr[high];
                arr[high] = qTmp;
            }
            return arr[median] ;
        }
        // Find median of low, middle and high items; swap into position low
        middle = (low + high) / 2;
        if (arr[middle] > arr[high])
        {
            qTmp = arr[middle];
            arr[middle] = arr[high];
            arr[high] = qTmp;
        }
        if (arr[low] > arr[high])
        {
            qTmp = arr[low];
            arr[low] = arr[high];
            arr[high] = qTmp;
        }
        if (arr[middle] > arr[low])
        {
            qTmp = arr[middle];
            arr[middle] = arr[low];
            arr[low] = qTmp;
        }
        // Swap low item (now in position middle) into position (low+1)
        qTmp = arr[middle];
        arr[middle] = arr[low+1];
        arr[low+1] = qTmp;
        // Nibble from each end towards middle, swapping items when stuck
        ll = low + 1;
        hh = high;
        for (;;)
        {
            do ll++; while (arr[low] > arr[ll]) ;
            do hh--; while (arr[hh]  > arr[low]) ;
            if (hh < ll)
                break;
            qTmp = arr[ll];
            arr[ll] = arr[hh];
            arr[hh] = qTmp;
        }
        // Swap middle item (in position low) back into correct position
        qTmp = arr[low];
        arr[low] = arr[hh];
        arr[hh] = qTmp;
        // Re-set active partition
        if (hh <= median)
            low = ll;
        if (hh >= median)
            high = hh - 1;
    }
}
qint64 Sound::somme(QByteArray baData, WORD wBps)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 n = baData.size() / 4;
    qint32 * arr = (qint32 *)baData.data();
    qint64 valeur = 0;
    for (int i = 0; i < n; i++)
        valeur += arr[i];
    return valeur;
}
qint64 Sound::sommeCarre(QByteArray baData, WORD wBps)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);
    qint32 n = baData.size() / 4;
    qint32 * arr = (qint32 *)baData.data();
    qint64 valeur = 0;
    for (int i = 0; i < n; i++)
        valeur += (arr[i]/47000) * (arr[i]/47000);
    return valeur;
}

double Sound::gainEQ(double freq, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10)
{
    int x1 = 0;
    int x2 = 1;
    int y1 = 0;
    int y2 = 1;
    if (freq < 32)
    {
        x1 = 32; x2 = 64;
        y1 = qMin(i1, i2); y2 = i2;
    }
    else if (freq < 64)
    {
        x1 = 32; x2 = 64;
        y1 = i1; y2 = i2;
    }
    else if (freq < 125)
    {
        x1 = 64; x2 = 125;
        y1 = i2; y2 = i3;
    }
    else if (freq < 250)
    {
        x1 = 125; x2 = 250;
        y1 = i3; y2 = i4;
    }
    else if (freq < 500)
    {
        x1 = 250; x2 = 500;
        y1 = i4; y2 = i5;
    }
    else if (freq < 1000)
    {
        x1 = 500; x2 = 1000;
        y1 = i5; y2 = i6;
    }
    else if (freq < 2000)
    {
        x1 = 1000; x2 = 2000;
        y1 = i6; y2 = i7;
    }
    else if (freq < 4000)
    {
        x1 = 2000; x2 = 4000;
        y1 = i7; y2 = i8;
    }
    else if (freq < 8000)
    {
        x1 = 4000; x2 = 8000;
        y1 = i8; y2 = i9;
    }
    else if (freq < 16000)
    {
        x1 = 8000; x2 = 16000;
        y1 = i9; y2 = i10;
    }
    else
    {
        x1 = 8000; x2 = 16000;
        y1 = i9; y2 = qMin(i9, i10);
    }
    double a = (double)(y1 - y2) / (x1 - x2);
    double b = (double)y2 - a * x2;
    // Gain en dB
    double val = a * freq + b;
    // Conversion
    return pow(10.0, val / 20);
}
void Sound::regimePermanent(QByteArray baData, DWORD dwSmplRate, WORD wBps, qint32 &posStart, qint32 &posEnd, int nbOK, double coef)
{
    if (wBps != 32)
        baData = bpsConversion(baData, wBps, 32);

    // Valeur absolue
    qint32 *data = (qint32 *)baData.data();
    for (int i = 0; i < baData.size() / 4; i++)
        data[i] = qAbs(data[i]);
    // Calcul de la moyenne des valeurs absolues sur une période de 0.05 s à chaque 10ième de seconde
    qint32 sizePeriode = dwSmplRate / 10;
    if (baData.size() < 4 * sizePeriode)
    {
        posStart = 0;
        posEnd = baData.size() / 4;
        return;
    }
    qint32 nbValeurs = (baData.size() / 4 - sizePeriode) / (dwSmplRate/20);
    QByteArray tableauMoyennes;
    tableauMoyennes.resize(nbValeurs * 4);
    data = (qint32 *)tableauMoyennes.data();
    for (int i = 0; i < nbValeurs; i++)
        data[i] = moyenne(baData.mid((dwSmplRate/20)*4*i, sizePeriode * 4), 32);
    // Calcul de la médiane des valeurs
    qint32 median = mediane(tableauMoyennes, 32);
    posStart = 0;
    posEnd = nbValeurs - 1;
    int count = 0;
    while (count < nbOK && posStart <= posEnd)
    {
        if (data[posStart] < coef * median && data[posStart] > (double)median / coef)
            count++;
        else
            count = 0;
        posStart++;
    }
    posStart -= count-2;
    count = 0;
    while (count < nbOK && posEnd > 0)
    {
        if (data[posEnd] < coef * median && data[posEnd] > (double)median / coef)
            count++;
        else
            count = 0;
        posEnd--;
    }
    posEnd += count-2;
    // Conversion position
    posStart *= dwSmplRate / 20;
    posEnd *= dwSmplRate / 20;
    posEnd += sizePeriode;
}

double Sound::sinc(double x)
{
    double epsilon0 = 0.32927225399135962333569506281281311031656150598474e-9L;
    double epsilon2 = qSqrt(epsilon0);
    double epsilon4 = qSqrt(epsilon2);

    if (qAbs(x) >= epsilon4)
        return(qSin(x)/x);
    else
    {
        // x très proche de 0, développement limité ordre 0
        double result = 1;
        if (qAbs(x) >= epsilon0)
        {
            double x2 = x*x;

            // x un peu plus loin de 0, développement limité ordre 2
            result -= x2 / 6.;

            if (qAbs(x) >= epsilon2)
            {
                // x encore plus loin de 0, développement limité ordre 4
                result += (x2 * x2) / 120.;
            }
        }
        return(result);
    }
}

// Keser-Bessel window
void Sound::KBDWindow(double* window, int size, double alpha)
{
    double sumvalue = 0.;
    int i;

    for (i = 0; i < size/2; i++)
    {
        sumvalue += BesselI0(M_PI * alpha * sqrt(1. - pow(4.0*i/size - 1., 2)));
        window[i] = sumvalue;
    }

    // need to add one more value to the nomalization factor at size/2:
    sumvalue += BesselI0(PI * alpha * sqrt(1. - pow(4.0*(size/2) / size-1., 2)));

    // normalize the window and fill in the righthand side of the window:
    for (i = 0; i < size/2; i++)
    {
        window[i] = sqrt(window[i]/sumvalue);
        window[size-1-i] = window[i];
    }
}

double Sound::BesselI0(double x)
{
    double denominator;
    double numerator;
    double z;

    if (x == 0.0)
        return 1.0;
    else
    {
        z = x * x;
        numerator = (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z*
                                                                     (z* 0.210580722890567e-22  + 0.380715242345326e-19 ) +
                                                                     0.479440257548300e-16) + 0.435125971262668e-13 ) +
                                                             0.300931127112960e-10) + 0.160224679395361e-7  ) +
                                                     0.654858370096785e-5)  + 0.202591084143397e-2  ) +
                                             0.463076284721000e0)   + 0.754337328948189e2   ) +
                                     0.830792541809429e4)   + 0.571661130563785e6   ) +
                             0.216415572361227e8)   + 0.356644482244025e9   ) +
                     0.144048298227235e10);
        denominator = (z*(z*(z-0.307646912682801e4)+
                          0.347626332405882e7)-0.144048298227235e10);
    }

    return -numerator/denominator;
}
