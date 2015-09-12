/*******************************************************************************
 INDI Dome Base Class
 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

 The code used calculate dome target AZ and ZD is written by Ferran Casarramona, and adapted from code from Markus Wildi.
 The transformations are based on the paper Matrix Method for Coodinates Transformation written by Toshimi Taki (http://www.asahi-net.or.jp/~zs3t-tk).

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#ifndef INDIDOME_H
#define INDIDOME_H

#include <libnova.h>

#include "defaultdevice.h"
#include "indicontroller.h"

// Defines a point in a 3 dimension space
typedef struct
{
    double x,y,z;
} point3D;

/**
 * \class INDI::Dome
   \brief Class to provide general functionality of a Dome device.

   Both relative and absolute position domes are supported. Furthermore, if no position feedback is available from the dome, an open-loop control is possible with simple direction commands (Clockwise and counter clockwise).

   Before using any of the dome functions, you must define the capabilities of the dome by calling SetDomeCapability() function. All positions are represented as degrees of azimuth.

   Relative motion is specified in degrees as either positive (clock wise direction), or negative (counter clock-wise direction).

   Slaving is used to synchronizes the dome's azimuth position with that of the mount. The user is required to fill in all required parameters before slaving can be used. The mount's azimuth position is snooped
   from the ACTIVE_TELESCOPE property in ACTIVE_DEVICES vector. The AutoSync threshold is the difference in degrees between the dome's azimuth angle and the mount's azimuth angle that should trigger a dome motion.
   By default, it is set to 0.5 degrees which would trigger dome motion due to any difference between the dome and mount azimuth angles that exceeds 0.5 degrees.
   For example, if the threshold is set to 5 degrees, the dome will only start moving to sync with the mount's azimuth angle once the difference in azimuth angles is equal or exceeds 5 degrees.
   The dome will only commence movement once the mount completed slewing.

   Custom parking position is available for absolute/relative position domes.

   For roll-off observatories, parking state reflects whether the roof is closed or open.

   Developers need to subclass INDI::Dome to implement any driver for Domes within INDI.

  \note The code used calculate dome target AZ and ZD is written by Ferran Casarramona, and adapted from code from Markus Wildi. The transformations are based on the paper Matrix Method for Coodinates
 Transformation written by Toshimi Taki (http://www.asahi-net.or.jp/~zs3t-tk).

\author Jasem Mutlaq
*/
class INDI::Dome : public INDI::DefaultDevice
{
    public:

    /** \typedef DomeMeasurements
        \brief Measurements necessary for dome-slit synchronization. All values are in meters. The displacements are measured from the true dome centre, and the dome is assumed spherical.
        \note: The mount centre is the point where RA and Dec. axis crosses, no matter the kind of mount. For example, for a fork mount this displacement is typically 0 if it's perfectly centred with RA axis.
    */
    typedef enum
    {
        DM_DOME_RADIUS,           /*!< Dome RADIUS */
        DM_SHUTTER_WIDTH,         /*!< Shutter width */
        DM_NORTH_DISPLACEMENT,    /*!< Displacement to north of the mount center */
        DM_EAST_DISPLACEMENT,     /*!< Displacement to east of the mount center */
        DM_UP_DISPLACEMENT,       /*!< Up Displacement of the mount center */
        DM_OTA_OFFSET             /*!< Distance from the optical axis to the mount center*/
    } DomeMeasurements;

    enum DomeDirection { DOME_CW, DOME_CCW };
    enum DomeMotionCommand { MOTION_START, MOTION_STOP };

    /*! Dome Parking data type enum */
    enum DomeParkData  { PARK_NONE,         /*!< Open loop Parking  */
                         PARK_AZ,           /*!< Parking via azimuth angle control */
                         PARK_AZ_ENCODER,   /*!< Parking via azimuth encoder control */
                       };

    /** \typedef ShutterOperation
        \brief Shutter operation command.
    */
    typedef enum
    {
        SHUTTER_OPEN,            /*!< Open Shutter */
        SHUTTER_CLOSE            /*!< Close Shutter */
    } ShutterOperation;

    /** \typedef DomeState
        \brief Dome status
    */
    typedef enum
    {
        DOME_IDLE,               /*!< Dome is idle */
        DOME_MOVING,             /*!< Dome is in motion */
        DOME_PARKING,            /*!< Dome is parking */
        DOME_UNPARKING,          /*!< Dome is unparking */
        DOME_PARKED,             /*!< Dome is parked */
    } DomeState;

    /** \typedef ShutterStatus
        \brief Shutter Status
    */
    typedef enum
    {
        SHUTTER_OPENED,           /*!< Shutter is open */
        SHUTTER_CLOSED,           /*!< Shutter is closed */
        SHUTTER_MOVING,           /*!< Shutter is in motion */
        SHUTTER_UNKNOWN           /*!< Shutter status is unknown */
    } ShutterStatus;


    /** \struct DomeCapability
        \brief Holds the capabilities of the dome.
    */
    typedef struct
    {
        /** Can the dome motion be aborted? */
        bool canAbort;
        /** Can the dome move to an absolute azimuth position? */
        bool canAbsMove;
        /** Can the dome move to a relative position a number of degrees away from current position? Positive degress is Clockwise direction. Negative Degrees is counter clock wise direction */
        bool canRelMove;
        /** Can the dome park and unpark itself? */
        bool canPark;
        /** Does the dome has a shutter than can be opened and closed electronically? */
        bool hasShutter;
        /** Can the dome move in different configurable speeds? */
        bool hasVariableSpeed;
    } DomeCapability;



    Dome();
    virtual ~Dome();

    virtual bool initProperties();
    virtual void ISGetProperties (const char *dev);
    virtual bool updateProperties();
    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISSnoopDevice (XMLEle *root);

    static void buttonHelper(const char * button_n, ISState state, void *context);

    /**
     * @brief GetDomeCapability returns the capability of the dome
     */
    DomeCapability GetDomeCapability() const { return capability;}

    /**
     * @brief SetDomeCapability set the dome capabilities. All capabilities must be initialized.
     * @param cap pointer to dome capability
     */
    void SetDomeCapability(DomeCapability * cap);

    DomeState getDomeState() const;
    void setDomeState(const DomeState &value);

    IPState getWeatherState() const;
    IPState getMountState() const;

protected:

    /**
     * @brief SetSpeed Set Dome speed. This does not initiate motion, it sets the speed for the next motion command. If motion is in progress, then change speed accordingly.
     * @param rpm Dome speed (RPM)
     * @return true if successful, false otherwise
     */
    virtual bool SetSpeed(double rpm);

    /** \brief Move the Dome in a particular direction.
        \param dir Direction of Dome, either DOME_CW or DOME_CCW.
        \return Return true if dome started motion in the desired direction. False if there is an error.
    */
    virtual bool Move(DomeDirection dir, DomeMotionCommand operation);

    /** \brief Move the Dome to an absolute azimuth.
        \param az The new position of the Dome.
        \return Return IPS_OK if motion is completed and Dome reached requested position. Return IPS_BUSY if Dome started motion to requested position and is in progress.
                Return IPS_ALERT if there is an error.
    */
    virtual IPState MoveAbs(double az);

    /** \brief Move the Dome to an relative position.
        \param azDiff The relative azimuth angle to move. Positive degree is clock-wise direction. Negative degrees is counter clock-wise direction.
        \return Return IPS_OK if motion is completed and Dome reached requested position. Return IPS_BUSY if Dome started motion to requested position and is in progress.
                Return IPS_ALERT if there is an error.
    */
    virtual IPState MoveRel(double azDiff);

    /**
     * \brief Abort all dome motion
     * \return True if abort is successful, false otherwise.
     */
    virtual bool Abort();

    /**
     * \brief Goto Park Position. The park position is an absolute azimuth value.
     * \return Return IPS_OK if motion is completed and Dome reached park position. Return IPS_BUSY if Dome started motion to park requested position and is in progress.
                Return -IPS_ALERT if there is an error.
     */
    virtual IPState Park();

    /**
     * \brief UnPark dome. The action of the Unpark command is dome specific, but it may include opening the shutter and moving to home position. When UnPark() is successful
     * The observatory should be in a ready state to utilize the mount to perform observations.
     * \return Return IPS_OK if motion is completed and Dome is unparked. Return IPS_BUSY if Dome unparking is in progress.
                Return -IPS_ALERT if there is an error.
     */
    virtual IPState UnPark();

    /**
     * \brief Open or Close shutter
     * \param operation Either open or close the shutter.
     * \return Return IPS_OK if shutter operation is complete. Return IPS_BUSY if shutter operation is in progress.
                Return IPS_ALERT if there is an error.
     */
    virtual IPState ControlShutter(ShutterOperation operation);

    /**
     * @brief getShutterStatusString
     * @param status Status of shutter
     * @return Returns string representation of the shutter status
     */
    const char * GetShutterStatusString(ShutterStatus status);

    /**
     * \brief setParkDataType Sets the type of parking data stored in the park data file and presented to the user.
     * \param type parking data type. If PARK_NONE then no properties will be presented to the user for custom parking position.
     */
    void SetParkDataType(DomeParkData type);

    /**
     * @brief InitPark Loads parking data (stored in ~/.indi/ParkData.xml) that contains parking status
     * and parking position. InitPark() should be called after successful connection to the dome on startup.
     * @return True if loading is successful and data is read, false otherwise. On success, you must call
     * SetAzParkDefault() to set the default parking values. On failure, you must call
     * SetAzParkDefault() to set the default parking values in addition to SetAzPark()
     * to set the current parking position.
     */
    bool InitPark();

    /**
     * @brief isParked is dome currently parked?
     * @return True if parked, false otherwise.
     */
    bool isParked();

    /**
     * @brief SetParked Change the mount parking status. The data park file (stored in ~/.indi/ParkData.xml) is updated in the process.
     * @param isparked set to true if parked, false otherwise.
     */
    void SetParked(bool isparked);

    /**
     * @return Get current AZ parking position.
     */
    double GetAxis1Park();

    /**
     * @return Get default AZ parking position.
     */
    double GetAxis1ParkDefault();

    /**
     * @brief SetRAPark Set current AZ parking position. The data park file (stored in ~/.indi/ParkData.xml) is updated in the process.
     * @param value current Axis 1 value (AZ either in angles or encoder values as specificed by the DomeParkData type).
     */
    void SetAxis1Park(double value);

    /**
     * @brief SetAxis1Park Set default AZ parking position.
     * @param value Default Axis 1 value (AZ either in angles or encoder values as specificed by the DomeParkData type).
     */
    void SetAxis1ParkDefault(double steps);

    /**
     * @brief SetCurrentPark Set current coordinates/encoders value as the desired parking position
     * \note This function performs no action unless subclassed by the child class if required.
     */
    virtual void SetCurrentPark();

    /**
     * @brief SetDefaultPark Set default coordinates/encoders value as the desired parking position
     * \note This function performs no action unless subclassed by the child class if required.
     */
    virtual void SetDefaultPark();

    //Park
    char *LoadParkData();
    bool WriteParkData();

    /**
     * @brief GetTargetAz
     * @param Az Returns Azimuth required to the dome in order to center the shutter aperture with telescope
     * @param Alt
     * @param minAz Returns Minimum azimuth in order to avoid any dome interference to the full aperture of the telescope
     * @param maxAz Returns Maximum azimuth in order to avoid any dome interference to the full aperture of the telescope
     * @return Returns false if it can't solve it due bad geometry of the observatory
     */
    bool GetTargetAz(double & Az, double & Alt, double & minAz, double & maxAz);

    /**
     * @brief Intersection Calculate the intersection of a ray and a sphere. The line segment is defined from p1 to p2.  The sphere is of radius r and centered at sc.
     * From http://local.wasp.uwa.edu.au/~pbourke/geometry/sphereline/
     * There are potentially two points of intersection given by
     * p := p1 + mu1 (p2 - p1)
     * p := p1 + mu2 (p2 - p1)
     * @param p1 First ray
     * @param p2 Second ray
     * @param sc Center of sphere
     * @param r RADIUS of sphere
     * @param mu1 First point of potentional intersection.
     * @param mu2 Second point of potentional intersection.
     * @return Returns FALSE if the ray doesn't intersect the sphere.
     */
    bool Intersection(point3D p1, point3D p2, point3D sc, double r, double & mu1, double & mu2);

    /**
     * @brief OpticalCenter This function calculates the distance from the optical axis to the Dome center
     * @param MountCenter Distance from the Dome center to the point where mount axis crosses
     * @param dOpticalAxis Distance from the mount center to the optical axis.
     * @param Lat Latitude
     * @param Ah Hour Angle (in hours)
     * @param OP a 3D point from the optical center to the Dome center.
     * @return false in case of error.
     */
    bool OpticalCenter(point3D MountCenter, double dOpticalAxis, double Lat, double Ah, point3D &OP);

    /**
     * @brief OpticalVector This function calculates a second point for determining the optical axis
     * @param OP Optical center
     * @param Az Azimuth
     * @param Alt Altitude
     * @param OV a 3D point that determines the optical line.
     * @return false in case of error.
     */
    bool OpticalVector(point3D OP, double Az, double Alt, point3D & OV);

    /**
     * @brief CheckHorizon Returns true if telescope points above horizon.
     * @param HA Hour angle
     * @param dec Declination
     * @param lat observer's latitude
     * @return True if telescope points above horizon, false otherwise.
     */
    bool CheckHorizon(double HA, double dec, double lat);

    /**
     * @brief saveConfigItems Saves the Device Port and Dome Presets in the configuration file
     * @param fp pointer to configuration file
     * @return true if successful, false otherwise.
     */
    virtual bool saveConfigItems(FILE *fp);

    /**
     * @brief updateCoords updates the horizontal coordinates (Az & Alt) of the mount from the snooped RA, DEC and observer's location.
     */
    void UpdateMountCoords();

    /**
     * @brief UpdateAutoSync This function calculates the target dome azimuth from the mount's target coordinates given the dome parameters.
     *  If the difference between the dome's and mount's azimuth angles exceeds the AutoSync threshold, the dome will be commanded to sync to the mount azimuth position.
     */
    virtual void UpdateAutoSync();

    double Csc(double x);
    double Sec(double x);

    INumberVectorProperty DomeSpeedNP;
    INumber DomeSpeedN[1];

    ISwitchVectorProperty DomeMotionSP;
    ISwitch DomeMotionS[2];

    INumberVectorProperty DomeAbsPosNP;
    INumber DomeAbsPosN[1];

    INumberVectorProperty DomeRelPosNP;
    INumber DomeRelPosN[1];

    ISwitchVectorProperty AbortSP;
    ISwitch AbortS[1];

    INumberVectorProperty DomeParamNP;
    INumber DomeParamN[1];

    ISwitchVectorProperty DomeShutterSP;
    ISwitch DomeShutterS[2];

    ISwitchVectorProperty ParkSP;
    ISwitch ParkS[2];

    INumber ParkPositionN[1];
    INumberVectorProperty ParkPositionNP;

    ISwitch ParkOptionS[3];
    ISwitchVectorProperty ParkOptionSP;

    ISwitch AutoParkS[2];
    ISwitchVectorProperty AutoParkSP;

    DomeCapability capability;    
    ShutterStatus shutterState;
    DomeParkData parkDataType;

    ITextVectorProperty PortTP;
    IText PortT[1];

    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[2];

    INumber PresetN[3];
    INumberVectorProperty PresetNP;
    ISwitch PresetGotoS[3];
    ISwitchVectorProperty PresetGotoSP;
    INumber DomeMeasurementsN[6];
    INumberVectorProperty DomeMeasurementsNP;
    ISwitchVectorProperty DomeAutoSyncSP;
    ISwitch DomeAutoSyncS[2];

    double prev_az, prev_alt, prev_ra, prev_dec;

private:

        void processButton(const char * button_n, ISState state);

        INDI::Controller *controller;

        DomeState domeState;

        struct ln_lnlat_posn observer;
        struct ln_hrz_posn mountHoriztonalCoords;
        struct ln_equ_posn mountEquatorialCoords;

        IPState mountState;
        IPState weatherState;

        bool IsParked;
        const char *ParkDeviceName;
        const char * Parkdatafile;
        XMLEle *ParkdataXmlRoot, *ParkdeviceXml, *ParkstatusXml, *ParkpositionXml, *ParkpositionAxis1Xml;

        double Axis1ParkPosition;
        double Axis1DefaultParkPosition;


};

#endif // INDIDOME_H
