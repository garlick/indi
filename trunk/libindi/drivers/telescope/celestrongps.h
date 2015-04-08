/*
    Celestron GPS
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

#ifndef CELESTRONGPS_H
#define CELESTRONGPS_H

#include <inditelescope.h>
#include "indidevapi.h"
#include "indicom.h"
#include "indicontroller.h"

#include "celestrondriver.h"

#define	POLLMS		1000		/* poll period, ms */

class CelestronGPS : public INDI::Telescope
{
 public:
 CelestronGPS();
 virtual ~CelestronGPS() {}

 virtual const char *getDefaultName();
 virtual bool Connect();
 virtual bool Connect(char *);
 virtual bool Disconnect();
 virtual bool ReadScopeStatus();
 virtual void ISGetProperties(const char *dev);
 virtual bool initProperties();
 virtual bool updateProperties();
 virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
 virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
 virtual bool ISSnoopDevice(XMLEle *root);

 static void joystickHelper(const char * joystick_n, double mag, double angle, void *context);
 static void buttonHelper(const char * button_n, ISState state, void *context);

protected:

 // Goto, Sync, and Motion
 bool Goto(double ra,double dec);
 bool Sync(double ra, double dec);
 virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command);
 virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command);
 virtual bool Abort();

 // Time and Location
 virtual bool updateLocation(double latitude, double longitude, double elevation);
 virtual bool updateTime(ln_date *utc, double utc_offset);

 // Parking
 virtual bool Park();
 virtual bool UnPark();
 virtual void SetCurrentPark();
 virtual void SetDefaultPark();

 virtual void simulationTriggered(bool enable);
 virtual bool saveConfigItems(FILE *fp);

 void mountSim();

 // Joystick
 void processNSWE(double mag, double angle);
 void processJoystick(const char * joystick_n, double mag, double angle);
 void processSlewPresets(double mag, double angle);
 void processButton(const char * button_n, ISState state);


 /* Slew Speed */
 ISwitchVectorProperty SlewRateSP;
 ISwitch SlewRateS[9];

 /* Firmware */
 IText   FirmwareT[5];
 ITextVectorProperty FirmwareTP;


private:
  INDI::Controller *controller;

  int PortFD;
  double currentRA, currentDEC;
  double targetRA, targetDEC;

  FirmwareInfo fwInfo;

};

#endif

