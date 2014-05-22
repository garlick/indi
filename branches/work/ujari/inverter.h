#if 0
    INDI Ujari Driver - Hitachi Inverter
    Copyright (C) 2014 Jasem Mutlaq
    
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

#endif

#ifndef INVERTER_H
#define INVERTER_H

#include <modbus.h>

#include <indidevapi.h>
#include <indicom.h>
#include <string>
#include <bitset>
#include <iostream>

#include <modbus.h>

using namespace std;

class Ujari;

class Inverter
{

    public:

        enum inverterType { DOME_INVERTER, SHUTTER_INVERTER };
        enum inverterMotion { INVERTER_STOP, INVERTER_FORWARD, INVERTER_REVERSE};
        enum inverterCommand { INVERTER_OPERATION, INVERTER_DIRECTION };


        Inverter(inverterType new_type, Ujari *scope);
        ~Inverter();

          bool move_forward();
          bool move_reverse();
          bool stop();

          bool set_speed(float newHz);
          float get_speed() { return InverterSpeedN[0].value; }

          void set_type(inverterType new_type);
          inverterType get_type() { return type; }

          bool connect();
          void disconnect();

        bool is_in_motion();
        // Simulation
        void setSimulation(bool enable);
        bool isSimulation() { return simulation;}

        inline void enable_debug() { debug = true;     modbus_set_debug(mb_param, true);}
        inline void disable_debug() { debug = false;   modbus_set_debug(mb_param, false);}

        // Standard INDI interface fucntions
        virtual bool initProperties();
        virtual void ISGetProperties();
        virtual bool updateProperties(bool connected);
        virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
        virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
        virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);

        void reset_all_properties();

        void set_verbose(bool enable) { verbose = enable; }

    private:

        // Modbus functions
        bool enable_drive();
        bool disable_drive();

        // INDI Properties

        // Inverter Speed (Hz)
        INumber InverterSpeedN[1];
        INumberVectorProperty InverterSpeedNP;

        // Motion Control
        ISwitch MotionControlS[3];
        ISwitchVectorProperty MotionControlSP;

        // Inverter Port
        ITextVectorProperty PortTP;
        IText PortT[1];

        // Functions
        bool check_drive_connection();

        // Variable
        int connection_status;
        inverterType type;
        bool simulation;
        bool debug;
        bool verbose;

        string type_name;
        string forward_motion;
        string reverse_motion;
        string default_port;        

        // Stop, Forward, Reverse
        inverterMotion motionStatus;

        Ujari *telescope;

        modbus_t * mb_param;

        uint SLAVE_ADDRESS;

        const uint OPERATION_COMMAND_ADDRESS;
        const uint DIRECTION_COMMAND_ADDRESS;
        const uint HZ_HOLD_ADDRESS;

        // 1st coil: Run (1) & Stop (0). 2nd coil: Reverse (1), Forward (0)
        uint8_t Motion_Control_Coils[2];
        // Frequency in Hz
        uint16_t Hz_Speed_Register[2];

};

#endif

