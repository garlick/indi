/*!
 * \file AlignmentSubsystemForClients.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "AlignmentSubsystemForClients.h"

#include <cstring>

namespace INDI {
namespace AlignmentSubsystem {

void AlignmentSubsystemForClients::Initialise(const char * DeviceName, INDI::BaseClient * BaseClient)
{
    AlignmentSubsystemForClients::DeviceName = DeviceName;
    ClientAPIForAlignmentDatabase::Initialise(BaseClient);
    ClientAPIForMathPluginManagement::Initialise(BaseClient);

}

void AlignmentSubsystemForClients::ProcessNewDevice(INDI::BaseDevice *DevicePointer)
{
    if (!strcmp(DevicePointer->getDeviceName(), DeviceName.c_str()))
    {
        IDLog("Receiving %s Device...\n", DevicePointer->getDeviceName());
        ClientAPIForAlignmentDatabase::ProcessNewDevice(DevicePointer);
        ClientAPIForMathPluginManagement::ProcessNewDevice(DevicePointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewProperty(INDI::Property *PropertyPointer)
{
    if (!strcmp(PropertyPointer->getDeviceName(), DeviceName.c_str()))
    {
        IDLog("newProperty %s\n", PropertyPointer->getName());
        ClientAPIForAlignmentDatabase::ProcessNewProperty(PropertyPointer);
        ClientAPIForMathPluginManagement::ProcessNewProperty(PropertyPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewSwitch(ISwitchVectorProperty *SwitchVectorPropertyPointer)
{
    if (!strcmp(SwitchVectorPropertyPointer->device, DeviceName.c_str()))
    {
        IDLog("newSwitch %s\n", SwitchVectorPropertyPointer->name);
        ClientAPIForAlignmentDatabase::ProcessNewSwitch(SwitchVectorPropertyPointer);
        ClientAPIForMathPluginManagement::ProcessNewSwitch(SwitchVectorPropertyPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewNumber(INumberVectorProperty *NumberVectorPropertyPointer)
{
    if (!strcmp(NumberVectorPropertyPointer->device, DeviceName.c_str()))
    {
        IDLog("newNumber %s\n", NumberVectorPropertyPointer->name);
        ClientAPIForAlignmentDatabase::ProcessNewNumber(NumberVectorPropertyPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewBLOB(IBLOB *BLOBPointer)
{
    if (!strcmp(BLOBPointer->bvp->device, DeviceName.c_str()))
    {
        IDLog("newBLOB %s\n", BLOBPointer->bvp->name);
        ClientAPIForAlignmentDatabase::ProcessNewBLOB(BLOBPointer);
    }
}

} // namespace AlignmentSubsystem
} // namespace INDI