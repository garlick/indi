/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class AltaEthernetIo 
* \brief First generation implemenation (alta) of the ethernet interface 
* 
*/ 


#ifndef ALTAETHERNETIO_INCLUDE_H__ 
#define ALTAEETHERNETIO_INCLUDE_H__ 

#include <string>
#include <vector>
#include <map>

#include "ICamIo.h" 
#include "IAltaSerialPortIo.h" 

class AltaEthernetIo : public ICamIo, public IAltaSerialPortIo
{ 
    public: 
        AltaEthernetIo( const std::string url );
        virtual ~AltaEthernetIo(); 

        std::map<uint16_t,uint16_t> ReadRegs(const std::vector<uint16_t> & Regs );

        uint16_t ReadReg( uint16_t reg ) const;
	    void WriteReg( uint16_t reg, uint16_t val ) ;

        void WriteSRMD( uint16_t reg, const std::vector<uint16_t> & data );

        void WriteMRMD( uint16_t reg, const std::vector<uint16_t> & data );

        void SetupImgXfer(uint16_t Rows, 
            uint16_t Cols, 
            uint16_t NumOfImages, bool IsBulkSeq);

        void CancelImgXfer();

        void GetImageData( std::vector<uint16_t> & data );

        void GetStatus(CameraStatusRegs::AdvStatus & status);
        void GetStatus(CameraStatusRegs::BasicStatus & status);

        void GetMacAddress( std::string & Mac );

        void Reboot();

        std::string GetNetworkSettings();

        std::string GetDriverVersion();

        uint16_t GetFirmwareRev();
        std::string GetInfo();

        void SetSerialBaudRate( uint16_t PortId , uint32_t BaudRate );

        uint32_t GetSerialBaudRate(  uint16_t PortId  );

        Apg::SerialFC GetSerialFlowControl( uint16_t PortId );

        void SetSerialFlowControl( uint16_t PortId, 
            Apg::SerialFC FlowControl );

         Apg::SerialParity GetSerialParity( uint16_t PortId );
        
        void SetSerialParity( uint16_t PortId, Apg::SerialParity Parity );
        
        void ReadSerial( uint16_t PortId, std::string & buffer );
        
        void WriteSerial( uint16_t PortId, const std::string & buffer );

        // TODO - see if we can do same error
        // trapping we do in USB
        bool IsError() { return false; }

    private:
        void OpenSession();
        void CloseSession();
        const std::string m_url;
        const std::string m_fileName;
        std::vector<uint16_t> m_StatusRegs;

        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        AltaEthernetIo(const AltaEthernetIo&);
        AltaEthernetIo& operator=(AltaEthernetIo&);
}; 

#endif
