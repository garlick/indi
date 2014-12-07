/*
    Baader Steeldrive Focuser
    Copyright (C) 2014 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "steeldrive.h"
#include "indicom.h"

#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <memory>

#define STEELDRIVE_TIMEOUT   3
#define STEELDRIVE_MAXBUF   16
#define STEELDRIVE_CMD      9
#define STEELDRIVE_CMD_LONG 11

#define FOCUS_SETTINGS_TAB  "Settings"

#define POLLMS  250

std::auto_ptr<SteelDrive> steelDrive(0);

void ISInit()
{
    static int isInit =0;

    if (isInit == 1)
        return;

     isInit = 1;
     if(steelDrive.get() == 0) steelDrive.reset(new SteelDrive());
}

void ISGetProperties(const char *dev)
{
         ISInit();
         steelDrive->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
         ISInit();
         steelDrive->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
         ISInit();
         steelDrive->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
         ISInit();
         steelDrive->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
   INDI_UNUSED(dev);
   INDI_UNUSED(name);
   INDI_UNUSED(sizes);
   INDI_UNUSED(blobsizes);
   INDI_UNUSED(blobs);
   INDI_UNUSED(formats);
   INDI_UNUSED(names);
   INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
     ISInit();
     steelDrive->ISSnoopDevice(root);
}

SteelDrive::SteelDrive()
{

    // Can move in Absolute & Relative motions, can AbortFocuser motion, and has variable speed.
    FocuserCapability cap;
    cap.canAbort=true;
    cap.canAbsMove=true;
    cap.canRelMove=true;
    cap.variableSpeed=true;

    SetFocuserCapability(&cap);

    sim=false;
    lastPos = 0;
    lastTemperature = 0;
}

SteelDrive::~SteelDrive()
{

}

bool SteelDrive::initProperties()
{
    INDI::Focuser::initProperties();

    FocusSpeedN[0].min = 350;
    FocusSpeedN[0].max = 1000;
    FocusSpeedN[0].value = 500;

    // Focuser temperature
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50, 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Temperature Settings
    IUFillNumber(&TemperatureSettingN[FOCUS_T_COEFF], "Coefficient", "", "%.3f", 0, 0.999, 0, 0.0);
    IUFillNumber(&TemperatureSettingN[FOCUS_T_SAMPLES], "# of Samples", "", "%.01f", 16, 128, 0, 16);
    IUFillNumberVector(&TemperatureSettingNP, TemperatureSettingN, 2, getDeviceName(), "Temperature Settings", "", FOCUS_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Compensate for temperature
    IUFillSwitch(&TemperatureCompensateS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&TemperatureCompensateSP, TemperatureCompensateS, 2, getDeviceName(), "Temperature Compensate", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Focuser Models
    IUFillSwitch(&ModelS[0], "NT2", "", ISS_OFF);
    fSettings[0].maxTrip=30;
    fSettings[0].gearRatio=0.25040;
    IUFillSwitch(&ModelS[1], "SC2", "", ISS_OFF);
    fSettings[1].maxTrip=30;
    fSettings[1].gearRatio=0.25040;
    IUFillSwitch(&ModelS[2], "RT2", "", ISS_OFF);
    fSettings[2].maxTrip=80;
    fSettings[2].gearRatio=0.25040;
    IUFillSwitch(&ModelS[3], "RT3", "", ISS_OFF);
    fSettings[3].maxTrip=115;
    fSettings[3].gearRatio=0.25040;
    IUFillSwitch(&ModelS[4], "Custom", "", ISS_ON);
    fSettings[4].maxTrip=30;
    fSettings[4].gearRatio=0.25040;
    IUFillSwitchVector(&ModelSP, ModelS, 5, getDeviceName(), "Model", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Custom Settings
    IUFillNumber(&CustomSettingN[FOCUS_MAX_TRIP], "Max Trip (mm)", "", "%6.2f", 20, 150, 0, 30);
    IUFillNumber(&CustomSettingN[FOCUS_GEAR_RATIO], "Gear Ratio", "", "%.5f", 0.1, 1, 0, .25040);
    IUFillNumberVector(&CustomSettingNP, CustomSettingN, 2, getDeviceName(), "Custom Settings", "", FOCUS_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Focuser Accleration
    IUFillNumber(&AccelerationN[0], "Ramp", "", "%.01f", 1500., 3000., 100., 2000.);
    IUFillNumberVector(&AccelerationNP, AccelerationN, 1, getDeviceName(), "FOCUS_ACCELERATION", "Acceleration", FOCUS_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Focus Sync
    IUFillNumber(&SyncN[0], "Position", "", "%.01f", 0., 200000., 100., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    // Version
    IUFillText(&VersionT[0], "HW Rev.", "", NULL);
    IUFillText(&VersionT[1], "FW Rev.", "", NULL);
    IUFillTextVector(&VersionTP, VersionT, 2, getDeviceName(), "FOCUS_VERSION", "Version", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    double maxSteps = floor(fSettings[4].maxTrip / fSettings[4].gearRatio * 100.0);

    /* Relative and absolute movement */
    FocusRelPosN[0].min = 0;
    FocusRelPosN[0].max = floor(maxSteps/2.0);
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step = 100;

    FocusAbsPosN[0].min = 0;
    FocusAbsPosN[0].max = maxSteps;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step = 1000;

    simPosition = FocusAbsPosN[0].value;
    addAuxControls();

    return true;

}

bool SteelDrive::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);
        defineNumber(&TemperatureSettingNP);
        defineSwitch(&TemperatureCompensateSP);

        defineSwitch(&ModelSP);
        defineNumber(&CustomSettingNP);
        defineNumber(&AccelerationNP);
        defineNumber(&SyncNP);

        defineText(&VersionTP);

        GetFocusParams();

        loadConfig(true);

        DEBUG(INDI::Logger::DBG_SESSION, "SteelDrive paramaters updated, focuser ready for use.");
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(TemperatureSettingNP.name);
        deleteProperty(TemperatureCompensateSP.name);

        deleteProperty(ModelSP.name);
        deleteProperty(CustomSettingNP.name);
        deleteProperty(AccelerationNP.name);
        deleteProperty(SyncNP.name);

        deleteProperty(VersionTP.name);
    }

    return true;

}

bool SteelDrive::Connect()
{
    int connectrc=0;
    char errorMsg[MAXRBUF];

    sim = isSimulation();

    if (!sim && (connectrc = tty_connect(PortT[0].text, 9600, 8, 0, 1, &PortFD)) != TTY_OK)
    {
        tty_error_msg(connectrc, errorMsg, MAXRBUF);

        DEBUGF(INDI::Logger::DBG_SESSION, "Failed to connect to port %s. Error: %s", PortT[0].text, errorMsg);

        return false;

    }

    if (Ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "SteelDrive is online. Getting focus parameters...");
        SetTimer(POLLMS);
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from SteelDrive, please ensure SteelDrive controller is powered and the port is correct.");
    return false;
}

bool SteelDrive::Disconnect()
{
    if (!sim)
        tty_disconnect(PortFD);
    DEBUG(INDI::Logger::DBG_SESSION, "SteelDrive is offline.");
    return true;
}

const char * SteelDrive::getDefaultName()
{
    return "Baader SteelDrive";
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::Ack()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    char hwVer[STEELDRIVE_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":FVERSIO#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FVERSIO# getHWVersion error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FVERSIO#)");

    if (sim)
    {
        strncpy(resp, ":FV2.00812#", STEELDRIVE_CMD_LONG);
        nbytes_read=STEELDRIVE_CMD;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "getHWVersion error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FV%s#", hwVer);

    if (rc > 0)
        return true;
    else
        return false;

}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateVersion()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    char hardware_string[MAXRBUF];
    char firmware_string[MAXRBUF];

    char hwdate[STEELDRIVE_MAXBUF/2];
    char hwrev[STEELDRIVE_MAXBUF/2];
    char fwdate[STEELDRIVE_MAXBUF/2];
    char fwrev[STEELDRIVE_MAXBUF/2];

    memset(hwdate, 0, sizeof(hwdate));
    memset(hwrev, 0, sizeof(hwrev));
    memset(fwdate, 0, sizeof(fwdate));
    memset(fwrev, 0, sizeof(fwrev));

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":FVERSIO#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FVERSIO# getHWVersion write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FVERSIO#)");

    if (sim)
    {
        strncpy(resp, ":FV2.00812#", STEELDRIVE_CMD_LONG);
        nbytes_read=STEELDRIVE_CMD_LONG;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "FVERSIO# getHWVersion read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FV%s#", hardware_string);

    if (rc > 0)
    {
        strncpy(hwrev, hardware_string, 3);
        strncpy(hwdate, hardware_string+3, 4);
        snprintf(hardware_string, MAXRBUF, "Version: %s Date: %s", hwrev, hwdate);
        IUSaveText(&VersionT[0], hardware_string);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: getHWVersion value (%s)", resp);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":FNFIRMW#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FNFIRMW# getSWVersion write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FNFIRMW#)");

    if (sim)
    {
        strncpy(resp, ":FN2.21012#", STEELDRIVE_CMD_LONG);
        nbytes_read=STEELDRIVE_CMD_LONG;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "FNFIRMW# getSWVersion read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FN%s#", firmware_string);

    if (rc > 0)
    {
        strncpy(fwrev, firmware_string, 3);
        strncpy(fwdate, firmware_string+3, 4);
        snprintf(firmware_string, MAXRBUF, "Version: %s Date: %s", fwrev, fwdate);
        IUSaveText(&VersionT[1], firmware_string);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: getSWVersion value (%s)", resp);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateTemperature()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    int temperature;

    tcflush(PortFD, TCIOFLUSH);


    if (!sim && (rc = tty_write(PortFD, ":F5ASKT0#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F5ASKT0# updateTemperature write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:F5ASKT0#)");

    if (sim)
    {
        strncpy(resp, ":F5+1810#", 9);
        nbytes_read=9;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F5ASKT0# updateTemperature read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":F5%d#", &temperature);

    if (rc > 0)
    {
        TemperatureN[0].value = ( (double) temperature)/100.0;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser temperature value (%s)", resp);
        return false;
    }

  return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updatePosition()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    unsigned short pos=0;

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":F8ASKS0#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F8ASKS0# updatePostion write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:F8ASKS0#)");

    if (sim)
    {
        snprintf(resp, STEELDRIVE_CMD_LONG, ":F8%07u#", (int) simPosition);
        nbytes_read=STEELDRIVE_CMD_LONG;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F8ASKS0# updatePostion read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":F8%hu#", &pos);

    if (rc > 0)
    {
        FocusAbsPosN[0].value = pos;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser position value (%s)", resp);
        return false;
    }

  return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateSpeed()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    unsigned short speed;    

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":FGSPMAX#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FGSPMAX# updateSpeed write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FGSPMAX#)");

    if (sim)
    {
        strncpy(resp, ":FG00350#", STEELDRIVE_CMD);
        nbytes_read = STEELDRIVE_CMD;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FGSPMAX# updateSpeed read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FG%hu#", &speed);

    if (rc > 0)
    {
        FocusSpeedN[0].value = speed;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser speed value (%s)", resp);
        return false;
    }

  return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateAcceleration()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];
    unsigned short accel;

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":FHSPMIN#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FHSPMIN# updateAcceleration write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FHSPMIN#)");

    if (sim)
    {
        strncpy(resp, ":FH01800#", STEELDRIVE_CMD);
        nbytes_read = STEELDRIVE_CMD;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FHSPMIN# updateAcceleration read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FH%hu#", &accel);

    if (rc > 0)
    {
        AccelerationN[0].value = accel;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: updateAcceleration value (%s)", resp);
        return false;
    }
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateTemperatureSettings()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];

    char selectedFocuser[1], coeff[3], enabled[1], tResp[STEELDRIVE_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, ":F7ASKC0#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F7ASKC0# updateTemperatureSettings write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:F7ASKC0#)");

    if (sim)
    {
        strncpy(resp, ":F710004#", STEELDRIVE_CMD);
        nbytes_read = STEELDRIVE_CMD;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F7ASKC0# updateTemperatureSettings read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":F7%s#", tResp);

    if (rc > 0)
    {
        strncpy(coeff, tResp, 3);
        strncpy(enabled, tResp+3, 1);
        strncpy(selectedFocuser, tResp+4, 1);

        TemperatureSettingN[FOCUS_T_COEFF].value = atof(coeff) / 1000.0;

        IUResetSwitch(&TemperatureCompensateSP);
        if (enabled[0] == '0')
            TemperatureCompensateS[1].s = ISS_ON;
        else
            TemperatureCompensateS[0].s = ISS_ON;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: updateTemperatureSettings value (%s)", resp);
        return false;
    }

  return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::updateCustomSettings()
{
    int nbytes_written=0, nbytes_read=0, rc=-1;
    char errstr[MAXRBUF];
    char resp[STEELDRIVE_MAXBUF];

    char selectedFocuser[2], maxTrip[8], tResp[STEELDRIVE_MAXBUF];
    int gearR;
    double gearRatio;

    tcflush(PortFD, TCIOFLUSH);

    // Get Gear Ratio
    if (!sim && (rc = tty_write(PortFD, ":FEASKGR#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FEASKGR# updateCustomSettings write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:FEASKGR#)");

    if (sim)
    {
        strncpy(resp, ":FE25040#", STEELDRIVE_CMD);
        nbytes_read = STEELDRIVE_CMD;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":FEASKGR# updateCustomSettings read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":FE%d#", &gearR);

    if (rc > 0)
    {
        gearRatio = ((double) gearR) / 100000.0;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: updateCustomSettings value (%s)", resp);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    // Get Max Trip
    if (!sim && (rc = tty_write(PortFD, ":F8ASKS1#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F8ASKS1# updateCustomSettings write error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (:F8ASKS1#)");

    if (sim)
    {
        strncpy(resp, ":F40011577#", STEELDRIVE_CMD_LONG);
        nbytes_read = STEELDRIVE_CMD_LONG;
    }
    else if ( (rc = tty_read_section(PortFD, resp, '#', STEELDRIVE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F8ASKS1# updateCustomSettings read error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, ":F%s#", tResp);

    if (rc > 0)
    {
        strncpy(selectedFocuser, tResp, 1);
        strncpy(maxTrip, tResp+1, 7);

        int sFocuser = atoi(selectedFocuser);

        IUResetSwitch(&ModelSP);
        ModelS[sFocuser].s = ISS_ON;

        fSettings[sFocuser].maxTrip   = (atof(maxTrip) * gearRatio) / 100.0;
        fSettings[sFocuser].gearRatio = gearRatio;

        CustomSettingN[FOCUS_MAX_TRIP].value   = fSettings[sFocuser].maxTrip;
        CustomSettingN[FOCUS_GEAR_RATIO].value = fSettings[sFocuser].gearRatio;

        DEBUGF(INDI::Logger::DBG_DEBUG, "Updated max trip: %g gear ratio: %g", fSettings[sFocuser].maxTrip, fSettings[sFocuser].gearRatio);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: updateCustomSettings value (%s)", resp);
        return false;
    }

  return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::setTemperatureSamples(unsigned int targetSamples, unsigned int *finalSample)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD];

    int maxSample = TemperatureSettingN[FOCUS_T_SAMPLES].max;
    int sample=0;

    for (int i=maxSample; i > 0;)
    {
        if (targetSamples & maxSample)
        {
            sample = maxSample;
            break;
        }

        maxSample >>= 1;
    }

    int value=0;
    if (sample == 16)
        value = 5000;
    else if (sample == 32)
        value = 15000;
    else if (sample == 64)
        value = 25000;
    else
        value = 35000;

    snprintf(cmd, STEELDRIVE_CMD, ":FI%05d#", value);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s setTemperatureSamples write error: %s.", cmd, errstr);
        return false;
    }

    TemperatureSettingN[FOCUS_T_SAMPLES].value = sample;
    *finalSample = sample;

    return true;

}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::setTemperatureCompensation()
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD];

    double coeff = TemperatureSettingN[FOCUS_T_COEFF].value;
    bool enable  = TemperatureCompensateS[0].s == ISS_ON;
    int selectedFocus = IUFindOnSwitchIndex(&ModelSP);

    snprintf(cmd, STEELDRIVE_CMD, ":F%02d%03d%d#", selectedFocus, (int) (coeff * 1000), enable ? 2 : 0);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, 6, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCoefficient error: %s.", errstr);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::setCustomSettings(double maxTrip, double gearRatio)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD_LONG];

    unsigned short mmTrip = (unsigned short int) (maxTrip/gearRatio * 100.0);

    snprintf(cmd, STEELDRIVE_CMD_LONG, ":FC%07d#", mmTrip);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD_LONG, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setCustomSettings error: %s.", errstr);
        return false;
    }

    snprintf(cmd, STEELDRIVE_CMD, ":FE%05d#", (int) (gearRatio * 100000));

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setCustomSettings error: %s.", errstr);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::Sync(unsigned int position)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD_LONG];

    snprintf(cmd, STEELDRIVE_CMD_LONG, ":FB%07d#", position);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD_LONG, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Sync error: %s.", errstr);
        return false;
    }

    simPosition = position;

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::moveFocuser(unsigned int position)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD_LONG];

    if (position < FocusAbsPosN[0].min || position > FocusAbsPosN[0].max)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Requested position value out of bound: %d", position);
        return false;
    }

    snprintf(cmd, STEELDRIVE_CMD_LONG, ":F9%07d#", position);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    // Goto absolute step
    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD_LONG, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition error: %s.", errstr);
        return false;
    }

    targetPos = position;

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::startMotion(FocusDirection dir)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD];

    // inward  --> decreasing value --> DOWN
    // outward --> increasing value --> UP
    strncpy(cmd, (dir == FOCUS_INWARD) ? ":F2MDOW0#" : ":F1MUP00#", STEELDRIVE_CMD);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "StartMotion error: %s.", errstr);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::setSpeed(unsigned short speed)
{    
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD];

    snprintf(cmd, STEELDRIVE_CMD, ":Fg%05d#", speed);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setSpeed error: %s.", errstr);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::setAcceleration(unsigned short accel)
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];
    char cmd[STEELDRIVE_CMD];

    snprintf(cmd, STEELDRIVE_CMD, ":Fh%05d#", accel);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!sim && (rc = tty_write(PortFD, cmd, STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setAcceleration error: %s.", errstr);
        return false;
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        if (!strcmp(TemperatureCompensateSP.name, name))
        {
            int last_index = IUFindOnSwitchIndex(&TemperatureCompensateSP);
            IUUpdateSwitch(&TemperatureCompensateSP, states, names, n);

            bool rc = setTemperatureCompensation();

            if (rc == false)
            {
                TemperatureCompensateSP.s = IPS_ALERT;
                IUResetSwitch(&TemperatureCompensateSP);
                TemperatureCompensateS[last_index].s = ISS_ON;
                IDSetSwitch(&TemperatureCompensateSP, NULL);
                return false;
            }

            TemperatureCompensateSP.s = IPS_OK;
            IDSetSwitch(&TemperatureCompensateSP, NULL);
            return true;
        }

        if (!strcmp(ModelSP.name, name))
        {
            IUUpdateSwitch(&ModelSP, states, names, n);

            int i = IUFindOnSwitchIndex(&ModelSP);

            double focusMaxPos = floor(fSettings[i].maxTrip / fSettings[i].gearRatio) * 100;
            FocusAbsPosN[0].max = focusMaxPos;
            IUUpdateMinMax(&FocusAbsPosNP);

            CustomSettingN[FOCUS_MAX_TRIP].value = fSettings[i].maxTrip;
            CustomSettingN[FOCUS_GEAR_RATIO].value = fSettings[i].gearRatio;

            IDSetNumber(&CustomSettingNP, NULL);

            ModelSP.s = IPS_OK;
            IDSetSwitch(&ModelSP, NULL);

            return true;
        }

    }


    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        // Set Accelration
        if (!strcmp(name, AccelerationNP.name))
        {
            if (setAcceleration((int) values[0]))
            {
                IUUpdateNumber(&AccelerationNP, values, names, n);
                AccelerationNP.s = IPS_OK;
                IDSetNumber(&AccelerationNP, NULL);
                return true;
            }
            else
            {

                AccelerationNP.s = IPS_ALERT;
                IDSetNumber(&AccelerationNP, NULL);
                return false;
            }
        }

        // Set Temperature Settings
        if (!strcmp(name, TemperatureSettingNP.name))
        {
            // Coeff is only needed when we enable or disable the temperature compensation. Here we only set the # of samples
            unsigned int targetSamples;

            if (!strcmp(names[0], TemperatureSettingN[FOCUS_T_SAMPLES].name))
                targetSamples = (int) values[0];
            else
                targetSamples = (int) values[1];

            unsigned int finalSample = targetSamples;

            if (setTemperatureSamples(targetSamples, &finalSample))
            {
                IUUpdateNumber(&TemperatureSettingNP, values, names, n);
                TemperatureSettingN[FOCUS_T_SAMPLES].value = finalSample;

                if (TemperatureSettingN[FOCUS_T_COEFF].value > TemperatureSettingN[FOCUS_T_COEFF].max)
                    TemperatureSettingN[FOCUS_T_COEFF].value = TemperatureSettingN[FOCUS_T_COEFF].max;

                TemperatureSettingNP.s = IPS_OK;
                IDSetNumber(&TemperatureSettingNP, NULL);
                return true;
            }
            else
            {
                TemperatureSettingNP.s = IPS_ALERT;
                IDSetNumber(&TemperatureSettingNP, NULL);
                return true;
            }
        }

        // Set Custom Settings
        if (!strcmp(name, CustomSettingNP.name))
        {
            int i = IUFindOnSwitchIndex(&ModelSP);

            // If the model is NOT set to custom, then the values cannot be updated
            if (i != 4)
            {
                CustomSettingNP.s = IPS_IDLE;
                DEBUG(INDI::Logger::DBG_WARNING, "You can not set custom values for a non-custom focuser.");
                IDSetNumber(&CustomSettingNP, NULL);
                return false;
            }


            double maxTrip, gearRatio;
            if (!strcmp(names[0], CustomSettingN[FOCUS_MAX_TRIP].name))
            {
                maxTrip   = values[0];
                gearRatio = values[1];
            }
            else
            {
                maxTrip   = values[1];
                gearRatio = values[0];
            }

            if (setCustomSettings(maxTrip, gearRatio))
            {
                IUUpdateNumber(&CustomSettingNP, values, names, n);
                CustomSettingNP.s = IPS_OK;
                IDSetNumber(&CustomSettingNP, NULL);
            }
            else
            {
                CustomSettingNP.s = IPS_ALERT;
                IDSetNumber(&CustomSettingNP, NULL);
            }
        }

        // Set Sync Position
        if (!strcmp(name, SyncNP.name))
        {
            if (Sync( (unsigned int) values[0]))
            {
                IUUpdateNumber(&SyncNP, values, names, n);
                SyncNP.s = IPS_OK;
                IDSetNumber(&SyncNP, NULL);

                if (updatePosition())
                    IDSetNumber(&FocusAbsPosNP, NULL);

                return true;
            }
            else
            {
                SyncNP.s = IPS_ALERT;
                IDSetNumber(&SyncNP, NULL);

                return false;
            }
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);

}

/************************************************************************************
 *
* ***********************************************************************************/
void SteelDrive::GetFocusParams ()
{
    if (updateVersion())
        IDSetText(&VersionTP, NULL);

    if (updateTemperature())
        IDSetNumber(&TemperatureNP, NULL);

    if (updateTemperatureSettings())
        IDSetNumber(&TemperatureSettingNP, NULL);

    if (updatePosition())
        IDSetNumber(&FocusAbsPosNP, NULL);   

    if (updateSpeed())
        IDSetNumber(&FocusSpeedNP, NULL);

    if (updateAcceleration())
        IDSetNumber(&AccelerationNP, NULL);

    if (updateCustomSettings())
    {
        IDSetNumber(&CustomSettingNP, NULL);
        IDSetSwitch(&ModelSP, NULL);
    }

}

bool SteelDrive::SetFocuserSpeed(int speed)
{
    bool rc = false;

    rc = setSpeed(speed);

    if (rc == false)
        return false;

    currentSpeed = speed;

    FocusSpeedNP.s = IPS_OK;
    IDSetNumber(&FocusSpeedNP, NULL);

    return true;
}

int SteelDrive::MoveFocuser(FocusDirection dir, int speed, int duration)
{
    if (speed != currentSpeed)
    {
        bool rc=false;

        rc = setSpeed(speed);
        if (rc == false)
            return -1;
    }

    gettimeofday(&focusMoveStart,NULL);
    focusMoveRequest = duration/1000.0;

    startMotion(dir);

    if (duration <= POLLMS)
    {
        usleep(POLLMS * 1000);
        AbortFocuser();
    }

    return 1;
}


int SteelDrive::MoveAbsFocuser(int targetTicks)
{
    targetPos = targetTicks;

    bool rc = false;

    rc = moveFocuser(targetPos);

    if (rc == false)
        return -1;

    FocusAbsPosNP.s = IPS_BUSY;

    return 1;
}

int SteelDrive::MoveRelFocuser(FocusDirection dir, unsigned int ticks)
{
    double newPosition=0;
    bool rc=false;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = moveFocuser(newPosition);

    if (rc == false)
        return -1;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s = IPS_BUSY;

    return 1;
}

void SteelDrive::TimerHit()
{
    if (isConnected() == false)
        return;

    bool rc = updatePosition();
    if (rc)
    {
        if (fabs(lastPos - FocusAbsPosN[0].value) > 5)
        {
            IDSetNumber(&FocusAbsPosNP, NULL);
            lastPos = FocusAbsPosN[0].value;
        }
    }

    rc = updateTemperature();
    if (rc)
    {
        if (fabs(lastTemperature - TemperatureN[0].value) >= 0.5)
        {
            IDSetNumber(&TemperatureNP, NULL);
            lastTemperature = TemperatureN[0].value;
        }
    }

    if (FocusTimerNP.s == IPS_BUSY)
    {
        float remaining = CalcTimeLeft(focusMoveStart, focusMoveRequest);

        if (sim)
        {
            if (FocusMotionS[FOCUS_INWARD].s == ISS_ON)
            {
                FocusAbsPosN[0].value -= FocusSpeedN[0].value;
                if (FocusAbsPosN[0].value <= 0)
                    FocusAbsPosN[0].value =0;
                simPosition = FocusAbsPosN[0].value;
            }
            else
            {
                FocusAbsPosN[0].value += FocusSpeedN[0].value;
                if (FocusAbsPosN[0].value >= FocusAbsPosN[0].max)
                    FocusAbsPosN[0].value=FocusAbsPosN[0].max;
                simPosition = FocusAbsPosN[0].value;
            }
        }

        // If we exceed focus abs values, stop timer and motion
        if (FocusAbsPosN[0].value <= 0 || FocusAbsPosN[0].value >= FocusAbsPosN[0].max)
        {
            AbortFocuser();

            if (FocusAbsPosN[0].value <= 0)
                FocusAbsPosN[0].value =0;
            else
                FocusAbsPosN[0].value=FocusAbsPosN[0].max;

            FocusTimerN[0].value=0;
            FocusTimerNP.s = IPS_IDLE;
        }
        else if (remaining <= 0)
        {
            FocusTimerNP.s = IPS_OK;
            FocusTimerN[0].value = 0;
            AbortFocuser();
        }
        else
            FocusTimerN[0].value = remaining*1000.0;

        IDSetNumber(&FocusTimerNP, NULL);
    }

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (sim)
        {
            if (FocusAbsPosN[0].value < targetPos)
                simPosition += 100;
            else
                simPosition -= 100;

            if (fabs(simPosition - targetPos) < 100)
            {
                FocusAbsPosN[0].value = targetPos;
                simPosition = FocusAbsPosN[0].value;
            }
        }

        // Set it OK to within 5 steps
        if (fabs(targetPos - FocusAbsPosN[0].value) < 5)
        {
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusAbsPosNP, NULL);
            IDSetNumber(&FocusRelPosNP, NULL);
            lastPos = FocusAbsPosN[0].value;
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
        }
    }

    SetTimer(POLLMS);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::AbortFocuser()
{
    int nbytes_written=0, rc=-1;
    char errstr[MAXRBUF];

    tcflush(PortFD, TCIOFLUSH);

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD :F3STOP0#");

    if (!sim && (rc = tty_write(PortFD, ":F3STOP0#", STEELDRIVE_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, ":F3STOP0# Stop error: %s.", errstr);
        return false;
    }

    FocusTimerNP.s = FocusAbsPosNP.s = IPS_IDLE;
    IDSetNumber(&FocusTimerNP, NULL);
    IDSetNumber(&FocusAbsPosNP, NULL);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
float SteelDrive::CalcTimeLeft(timeval start,float req)
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now,NULL);

    timesince=(double)(now.tv_sec * 1000.0 + now.tv_usec/1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec/1000);
    timesince=timesince/1000;
    timeleft=req-timesince;
    return timeleft;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool SteelDrive::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &TemperatureSettingNP);
    IUSaveConfigSwitch(fp, &TemperatureCompensateSP);
    IUSaveConfigNumber(fp, &FocusSpeedNP);
    IUSaveConfigNumber(fp, &AccelerationNP);
    IUSaveConfigNumber(fp, &CustomSettingNP);
    IUSaveConfigSwitch(fp, &ModelSP);

    return true;
}


