/*******************************************************************************
  Copyright(c) 2010 Gerry Rozema. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#ifndef CCDSIM_H
#define CCDSIM_H

#include "indibase/indiccd.h"
#include "indibase/indifilterinterface.h"

/*  Some headers we need */
#include <math.h>
#include <sys/time.h>


class CCDSim : public INDI::CCD, public INDI::FilterInterface
{
    protected:

    virtual bool saveConfigItems(FILE *fp);

    private:

        bool InExposure;
        float ExposureRequest;
        struct timeval ExpStart;

        bool InGuideExposure;
        float GuideExposureRequest;
        struct timeval GuideExpStart;

        float CalcTimeLeft(timeval,float);

        int testvalue;
        int ShowStarField;
        int bias;
        int maxnoise;
        int maxval;
        int maxpix;
        int minpix;
        float skyglow;
        float limitingmag;
        float saturationmag;
        float seeing;
        float ImageScalex;
        float ImageScaley;
        float focallength;
        float guider_focallength;
        float OAGoffset;
        float TimeFactor;
        //  our zero point calcs used for drawing stars
        float k;
        float z;

        bool AbortGuideFrame;
        bool AbortPrimaryFrame;


        float GuideRate;

        float PEPeriod;
        float PEMax;

        double raPEC,decPEC;
        time_t RunStart;

        //  And this lives in our simulator settings page

        INumberVectorProperty *SimulatorSettingsNV;
        INumber SimulatorSettingsN[11];

        ISwitch TimeFactorS[3];
        ISwitchVectorProperty *TimeFactorSV;

        bool SetupParms();

        //  We are going to snoop these from focuser
        INumberVectorProperty FWHMNP;
        INumber FWHMN[1];

        // We are going to snoop these from telescope
        INumber ScopeParametersN[4];
        INumberVectorProperty ScopeParametersNP;

        INumberVectorProperty EqPECNP;
        INumber EqPECN[2];

        // Filter
        bool SelectFilter(int);
        bool SetFilterNames() { return false; }
        bool GetFilterNames(const char* groupName);
        int QueryFilter();

    public:
        CCDSim();
        virtual ~CCDSim();

        const char *getDefaultName();

        bool initProperties();
        bool updateProperties();

        void ISGetProperties (const char *dev);


        bool Connect();
        bool Disconnect();

        int StartExposure(float duration);
        int StartGuideExposure(float);

        bool AbortExposure();
        bool AbortGuideExposure();


        void TimerHit();

        int DrawCcdFrame(CCDChip *targetChip);

        int DrawImageStar(CCDChip *targetChip, float,float,float);
        int AddToPixel(CCDChip *targetChip, int,int,int);

        bool GuideNorth(float);
        bool GuideSouth(float);
        bool GuideEast(float);
        bool GuideWest(float);

        virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
        virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
        virtual bool ISSnoopDevice (XMLEle *root);

};

#endif // CCDSim_H
