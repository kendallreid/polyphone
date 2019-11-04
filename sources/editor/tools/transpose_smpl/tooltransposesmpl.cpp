/***************************************************************************
**                                                                        **
**  Polyphone, a soundfont editor                                         **
**  Copyright (C) 2013-2019 Davy Triponney                                **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program. If not, see http://www.gnu.org/licenses/.    **
**                                                                        **
****************************************************************************
**           Author: Davy Triponney                                       **
**  Website/Contact: https://www.polyphone-soundfonts.com                 **
**             Date: 01.01.2013                                           **
***************************************************************************/

#include "tooltransposesmpl.h"
#include "tooltransposesmpl_gui.h"
#include "tooltransposesmpl_parameters.h"
#include "soundfontmanager.h"
#include "sampleutils.h"
#include "qmath.h"

ToolTransposeSmpl::ToolTransposeSmpl() : AbstractToolIterating(elementSmpl, new ToolTransposeSmpl_parameters(), new ToolTransposeSmpl_gui())
{

}

void ToolTransposeSmpl::process(SoundfontManager * sm, EltID id, AbstractToolParameters *parameters)
{
    ToolTransposeSmpl_parameters * params = dynamic_cast<ToolTransposeSmpl_parameters *>(parameters);

    // Get sample data
    QByteArray baData = sm->getData(id, champ_sampleDataFull24);
    quint32 echFinal = sm->get(id, champ_dwSampleRate).dwValue;

    // Compute the new initial sample rate
    double echInit = static_cast<double>(echFinal) * qPow(2, params->getSemiTones() / 12);

    // Resampling
    baData = SampleUtils::resampleMono(baData, echInit, echFinal, 24);
    sm->set(id, champ_sampleDataFull24, baData);

    // Update the length
    AttributeValue val;
    val.dwValue = static_cast<quint32>(baData.size() / 3);
    sm->set(id, champ_dwLength, val);

    // Update loop
    quint32 dwTmp = sm->get(id, champ_dwStartLoop).dwValue;
    dwTmp = ((qint64)dwTmp * (qint64)echFinal) / echInit;
    val.dwValue = dwTmp;
    sm->set(id, champ_dwStartLoop, val);
    dwTmp = sm->get(id, champ_dwEndLoop).dwValue;
    dwTmp = ((qint64)dwTmp * (qint64)echFinal) / echInit;
    val.dwValue = dwTmp;
    sm->set(id, champ_dwEndLoop, val);

    // Update rootkey / correction
    int deltaPitch = qRound(params->getSemiTones());
    int deltaCorrection = qRound(100. * (params->getSemiTones() - deltaPitch));
    int newPitch = sm->get(id, champ_byOriginalPitch).bValue + deltaPitch;
    int newCorrection = sm->get(id, champ_chPitchCorrection).cValue + deltaCorrection;
    while (newCorrection < -50)
    {
        newCorrection += 100;
        newPitch -= 1;
    }
    if (newCorrection > 50)
    {
        newCorrection -= 100;
        newPitch += 1;
    }
    if (newPitch < 0)
    {
        newPitch = 0;
        newCorrection = 0;
    }
    if (newPitch > 127)
    {
        newPitch = 127;
        newCorrection = 0;
    }

    val.bValue = static_cast<quint8>(newPitch);
    sm->set(id, champ_byOriginalPitch, val);
    val.shValue = static_cast<qint8>(newCorrection);
    sm->set(id, champ_chPitchCorrection, val);
}
