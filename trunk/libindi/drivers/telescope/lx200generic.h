/*
    LX200 Generic
    Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef LX200GENERIC_H
#define LX200GENERIC_H

#include "indibase/indiguiderinterface.h"
#include "indibase/inditelescope.h"

#include "indidevapi.h"
#include "indicom.h"

#define	POLLMS		1000		/* poll period, ms */

class LX200Generic: public INDI::Telescope, public INDI::GuiderInterface
{
 public:
    LX200Generic();
    virtual ~LX200Generic();

    virtual const char *getDefaultName();
    virtual bool Connect();
    virtual bool Connect(char *);
    virtual bool Disconnect();
    virtual bool ReadScopeStatus();
    virtual void ISGetProperties(const char *dev);
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

    int getPort() { return PortFD; }

  protected:

    virtual bool MoveNS(TelescopeMotionNS dir);
    virtual bool MoveWE(TelescopeMotionWE dir);
    virtual bool Abort();

    virtual bool updateTime(ln_date utc, double utc_offset);
    virtual bool updateLocation(double latitude, double longitude, double elevation);

    virtual bool GuideNorth(float ms);
    virtual bool GuideSouth(float ms);
    virtual bool GuideEast(float ms);
    virtual bool GuideWest(float ms);

    bool Goto(double,double);
    bool Park();
    bool Sync(double ra, double dec);
    virtual bool canSync();
    virtual bool canPark();


    virtual void getBasicData();
    void slewError(int slewCode);
    void getAlignment();
    void sendScopeTime();
    void sendScopeLocation();
    void mountSim();

    static void updateFocusTimer(void *p);
    static void guideTimeout(void *p);

    int    GuideNSTID;
    int    GuideWETID;


    int timeFormat;
    int currentSiteNum;
    int trackingMode;

    double JD;
    double targetRA, targetDEC;
    double currentRA, currentDEC;

  /* Telescope Alignment Mode */
  ISwitchVectorProperty AlignmentSP;
  ISwitch AlignmentS[3];

  /* Slew Speed */
  ISwitchVectorProperty SlewModeSP;
  ISwitch SlewModeS[4];

  /* Tracking Mode */
  ISwitchVectorProperty TrackModeSP;
  ISwitch TrackModeS[3];

  /* Tracking Frequency */
  INumberVectorProperty TrackingFreqNP;
  INumber TrackFreqN[1];

  /* Use pulse-guide commands */
  ISwitchVectorProperty UsePulseCmdSP;
  ISwitch UsePulseCmdS[1];

  /* Site Management */
  ISwitchVectorProperty SiteSP;
  ISwitch SiteS[4];

  /* Site Name */
  ITextVectorProperty SiteNameTP;
  IText   SiteNameT[1];

  /* Focus motion */
  ISwitchVectorProperty	FocusMotionSP;
  ISwitch  FocusMotionS[2];

  /* Focus Timer */
  INumberVectorProperty FocusTimerNP;
  INumber  FocusTimerN[1];

  /* Focus Mode */
  ISwitchVectorProperty FocusModeSP;
  ISwitch  FocusModeS[3];


};

#endif
