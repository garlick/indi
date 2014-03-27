/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \brief Namespace the support decoding camera model names from raw input data 
* 
*/ 


#ifndef CAMERAMODEL_INCLUDE_H__ 
#define CAMERAMODEL_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>
#include "DefDllExport.h"

namespace CamInfo
{
    struct DLL_EXPORT StrDb {
        std::string FactorySn;
        std::string CustomerSn;
        std::string Id;
        std::string Platform;
        std::string PartNum;
        std::string Ccd;
        std::string CcdSn;
        std::string CcdGrade;
        std::string ProcBoardRev;
        std::string DriveBoardRev;
        std::string Shutter;
        std::string WindowType;
        std::string MechCfg;
        std::string MechRev;
        std::string CoolingType;
        std::string FinishFront;
        std::string FinishBack;
        std::string MpiRev;
        std::string TestDate;
        std::string TestedBy;
        std::string TestedDllRev;
        std::string TestedFwRev;
        std::string Gain;
        std::string Noise;
        std::string Bias;
        std::string TestTemp;
        std::string DarkCount;
        std::string DarkDuration;
        std::string DarkTemp;
        std::string CoolingDelta;
        std::string Ad1Offset;
        std::string Ad1Gain;
        std::string Ad2Offset;
        std::string Ad2Gain;
        std::string Rma1;
        std::string Rma2;
        std::string Comment1;
        std::string Comment2;
        std::string Comment3;
    };

    std::vector< std::string > DLL_EXPORT MkStrVectFromStrDb( const CamInfo::StrDb & DbStruct );
    CamInfo::StrDb DLL_EXPORT MkStrDbFromStrVect( const std::vector< std::string > & strVect );
    CamInfo::StrDb DLL_EXPORT GetNoOpDb();

    const uint32_t NET_MAGIC_VALID = 0x63626160;	
	struct DLL_EXPORT NetDb {
		uint32_t Magic;          // 0x63626160 when struct is valid
		uint8_t  IP[4];          // IP address
		uint8_t  Gateway[4];     // Gateway
		uint8_t  Netmask[4];     // mask
		uint8_t  Port[2];        // Webserver port
		uint8_t  Flags[4];       // flag[0].0 DHCP enable
		uint8_t  MAC[6];         // MAC
		uint32_t Timeout;        // session timeout (minutes)
	};
    std::vector< uint8_t > DLL_EXPORT MkU8VectFromNetDb( const CamInfo::NetDb & DbStruct );
    CamInfo::NetDb DLL_EXPORT MkNetDbFromU8Vect( const std::vector< uint8_t > & u8Vect );

}

/*! 
* \namespace CamModel
* Namespace that supports different camera models
*/
namespace CamModel
{
    /*! */
    enum DLL_EXPORT InterfaceType {	
        /*! */
        UNKNOWN_INTERFACE,
        /*! */
        USB,
        /*! */
        ETHERNET
        };

    /*! */
    enum DLL_EXPORT PlatformType {	
        /*! */
        UNKNOWN_PLATFORM,
        /*! */
        ALTAU,
        /*! */
        ALTAE,
        /*! */
        ASCENT,
        /*! */
        ASPEN,
        /*! */
        HIC,
        /*! */
        ALTAF,
        /*! */
        QUAD
    };

    bool DLL_EXPORT IsAlta(uint16_t FirmwareRev);
    bool DLL_EXPORT IsGen2Platform(uint16_t FirmwareRev);
    bool DLL_EXPORT IsFirmwareRevGood( uint16_t FirmwareRev );

    uint16_t DLL_EXPORT MaskRawId( uint16_t FirmwareRev,
        uint16_t CamId);

    
    CamModel::PlatformType DLL_EXPORT GetPlatformType( uint16_t FixedId, bool IsEthernet=false );
    CamModel::PlatformType DLL_EXPORT GetPlatformType( const std::string & cameraLine );
    std::string DLL_EXPORT GetPlatformStr(uint16_t FixedId, bool IsEthernet=false);
    std::string DLL_EXPORT GetModelStr( uint16_t CamId );
    std::string DLL_EXPORT GetNoOpFirmwareRev();

     // Used to derive camera platform type from camera ID
    const uint16_t FIRMWARE_PLATFORM_MASK              = 0xFF00;	
    const uint16_t MAX_ALTA_FIRMWARE_REV                  = 0x0064;
    const uint16_t MIN_GEN2_FIRMWARE                            = 0x0064; 
    const uint16_t MAX_GEN2_FIRMWARE                           = 0x0400; 


    const uint16_t ALTA_CAMERA_ID_MASK		         = 0x007F;
    const uint16_t GEN2_CAMERA_ID_MASK               = 0x01FF;

    
    const uint16_t NO_OP_FRMWR_REV                    = 0x0;
}

/*! 
* \namespace Apg
* Defines the different camera's statuses, modes, and type enumerations
*/
namespace Apg
{
    /*! The current imaging state of the camera. */
    enum DLL_EXPORT Status
    {	
        /*!  An internal error was generated while attempting to communicate 
        with the camera. This error may occur when a connection to the camera is 
        attempted and failed, or if the driver detects a failure while communicating 
        with the camera system (for example, if a USB connector is suddenly unplugged).*/
        Status_ConnectionError	= -3, 
        /*! An internal error was generated by the camera during image readout and the 
        internal FIFO was hung. Using the ApogeeCam::Reset() or ApogeeCam::Init() methods may return the 
        camera to a known, good state. */
        Status_DataError = -2,  
        /*! An internalerror was generated by the camera during pixel processing. 
        Using the ApogeeCam::Reset() or ApogeeCam::Init() methods may return the camera to a known, good state.*/
        Status_PatternError = -1,
        /*! The camera system is completely idle. Flushing operations have not been started. 
        Applications should typically never see this state after the ApogeeCam::Init() method has been called. */
        Status_Idle	= 0,
        /*! An exposure is in progress. */
        Status_Exposing	= 1,
        /*! The camera is reading out an image, or waiting for an image to begin. While an image 
        is actually being exposed, the status returned will be Status_Exposing*/
        Status_ImagingActive	= 2,
        /*! The camera has completed an exposure and digitized the image data. Applications 
        should poll this flag before retrieving the image data. Once the image data has been 
        read, the camera will return the Status_Flushing state.*/
        Status_ImageReady = 3,
        /*! The camera system is flushing the sensor. No other operations are in effect.*/
        Status_Flushing = 4,
        /*! The camera is waiting for a trigger event to start an exposure. */
        Status_WaitingOnTrigger = 5
    };

    /*! Camera operational mode.*/
    enum DLL_EXPORT CameraMode
    {	
        /*! Specifies nominal camera operation for exposure control. 
        Single exposures, or sequences of exposures, are may be initiated 
        by software or hardware control. Applications should note that the 
        ContinuousImaging property is only available in this mode.*/
        CameraMode_Normal = 0,
        /*! Specifies camera operation using time delayed integration 
        (drift scan) mode. Used in conjunction with TDIRows, TDIRate 
        and TDIBinningRows. The actual TDI exposure is started with the 
        Expose method, but the "Duration" parameter of Expose is not 
        used. This mode cannot be used with interline sensors. */
        CameraMode_TDI	= 1,
        /*! Specifies that the camera operation should be defined using 
        simulated data for image parameters.*/
        CameraMode_Test	= 2,
        /*! While maintained for backward compatibility, this mode is 
        deprecated and will throw an error if set. Applications should use 
        the new Apg::TriggerMode_Normal and 
        Apg::TriggerMode_TdiKinetics properties to enable and use 
        external hardware triggering.*/
        CameraMode_ExternalTrigger	= 3,
        /*! While maintained for backward compatibility, this mode is 
        deprecated and will throw an error if set. Applications should 
        use the ExternalShutter property instead. */
        CameraMode_ExternalShutter	= 4,
        /*! In this mode, the user will optically mask all but a portion of the 
        CCD. This remaining section is exposed, shifted by some number 
        of rows, and then exposed again. The process continues until the 
        entire CCD surface is exposed. This mode cannot be used with 
        interline sensors. */
        CameraMode_Kinetics	= 5,
        /*! Error mode. Application should set the camera mode to CameraMode_Normal
        and call the Reset() or Init() methods to return the camera to a 
        known, good state.*/
        CameraMode_Unknown	= 6
    };

     /*! */
    enum DLL_EXPORT Resolution
    {	
        /*! */
        Resolution_SixteenBit	= 0,
        /*! */
        Resolution_TwelveBit	= 1
    };


     /*! */
    enum DLL_EXPORT AdcSpeed
    {
        /*! */
        AdcSpeed_Unknown,
        /*! */
        AdcSpeed_Normal,
        /*! */
        AdcSpeed_Fast,
        /*! */
        AdcSpeed_Video
    };

     /*! */
    enum DLL_EXPORT CoolerStatus
    {	/*! */
        CoolerStatus_Off	= 0,
        /*! */
        CoolerStatus_RampingToSetPoint	= 1,
        /*! */
        CoolerStatus_AtSetPoint	= 2,
        /*! */
        CoolerStatus_Revision	= 3,
        /*! */
        CoolerStatus_Suspended	= 4
    };

    /*! Camera Fan Mode */
    enum DLL_EXPORT FanMode
    {	
        /*! Turns off the fan*/
        FanMode_Off	= 0,
        /*! Low Speed */
        FanMode_Low = 1,
        /*! Medium Speed*/
        FanMode_Medium	= 2,
        /*! High Speed */
        FanMode_High = 3,
        /*! Error state.*/
        FanMode_Unknown = 4
    };

     /*! */
    enum DLL_EXPORT LedState
    {	/*! */
        LedState_Expose	= 0,
        /*! */
        LedState_ImageActive	= 1,
        /*! */
        LedState_Flushing	= 2,
        /*! */
        LedState_ExtTriggerWaiting	= 3,
        /*! */
        LedState_ExtTriggerReceived	= 4,
        /*! */
        LedState_ExtShutterInput	= 5,
        /*! */
        LedState_ExtStartReadout	= 6,
        /*! */
        LedState_AtTemp = 7,
        /*! */
        LedState_Unknown = 8
    };

     /*! */
    enum DLL_EXPORT LedMode
    {	
        /*! */
        LedMode_DisableAll	= 0,
        /*! */
        LedMode_DisableWhileExpose	= 1,
        /*! */
        LedMode_EnableAll = 2,
        /*! */
        LedMode_Unknown = 3
    };

    /*! */
    enum DLL_EXPORT TriggerMode
    {
        /*! */
        TriggerMode_Unknown,
        /*! */
        TriggerMode_Normal,
        /*! */
        TriggerMode_TdiKinetics,
        /*! */
        TriggerMode_ExternalShutter,
        /*! */
        TriggerMode_ExternalReadoutIo
    };

    /*! */
    enum DLL_EXPORT TriggerType
    {
        /*! */
        TriggerType_Unkown,
        /*! */
        TriggerType_Each,
        /*! */
        TriggerType_Group
    };

    /*! Camera's shutter state*/
    enum DLL_EXPORT ShutterState
    {
        /*! Error condition*/
        ShutterState_Unkown,
        /*! Camera performs normal shutter operations*/
        ShutterState_Normal,
        /*! Shutter is forced open */
        ShutterState_ForceOpen,
        /*! Shutter is forced closed and disabled during an exposure*/
        ShutterState_ForceClosed
    };

    /*! Exception Error Catagories.  
    * See the \ref exceptionHandling "Exception Handling" 
    * page for more information */
    enum DLL_EXPORT ErrorType
    {
        /*! Error trying to establish connection with camera */
        ErrorType_Connection = 0,
        /*! Critical error communicating with camera call ApogeeCam::CloseConnection() and
        * search again for the device.  If found call ApogeeCam::OpenConnection and reinitalized
        * the camera */
        ErrorType_Critical = 1,
        /*! Internal error call ApogeeCam::Reset() and ApogeeCam::Init() to reset the camera*/
        ErrorType_Serious = 2,
        /*! Error in the configuraiton data */
        ErrorType_Configuration,
        /*! Camera is not the proper mode to support the operation */
        ErrorType_InvalidMode,
        /*! Operation is not support on the camera model */
        ErrorType_InvalidOperation,
        /*! Invalid information was given to function */
        ErrorType_InvalidUsage
        
    };

     /*! AltaU serial port parity type*/
    enum DLL_EXPORT SerialParity
    {
        /*! The parity state of the port cannot be determined. */
        SerialParity_Unknown	= -1,
        /*! The port does not/should not use parity. */
        SerialParity_None	= 0,
        /*! The port does/should use odd parity. */
        SerialParity_Odd	= 1,
        /*! The port does/should use even parity */
        SerialParity_Even = 2
    };

    /*! AltaU serial port flow control type*/
    enum DLL_EXPORT SerialFC
    {
        /*! The flow control state of the port cannot be determined. */
        SerialFC_Unknown	= -1,
        /*! The port does not/should not use flow control. */
        SerialFC_Off	= 0,
        /*! The port does/should use flow control. */
        SerialFC_On	= 1
    };

}

#endif
