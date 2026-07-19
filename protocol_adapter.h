#ifndef PROTOCOL_ADAPTER_H
#define PROTOCOL_ADAPTER_H
//----------------------------------------------------------------------------------------------------------------------
#include "chanlib_export.h"
#include "modbus_client.h"
#include <memory>
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
class ModbusGpioGateway;
//----------------------------------------------------------------------------------------------------------------------
class ProtocolAdapter
{
public:
    weak_ptr<ModbusGpioGateway> MBGW;
    ProtocolAdapter(shared_ptr<ModbusGpioGateway> SGW) : MBGW(SGW) {}
    virtual ~ProtocolAdapter() {}
    virtual void process_packet(std::unique_ptr<MessageBuffer> packet) = 0;
    enum modbus_pdu_type_e pdu_type;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*PROTOCOL_ADAPTER_H*/
