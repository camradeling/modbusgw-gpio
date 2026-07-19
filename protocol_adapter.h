#ifndef PROTOCOL_ADAPTER_H
#define PROTOCOL_ADAPTER_H
//----------------------------------------------------------------------------------------------------------------------
#include "modbus_gpio_gateway.h"
//----------------------------------------------------------------------------------------------------------------------
class ProtocolAdapter
{
public:
    weak_ptr<ModbusGpioGateway> MBGW;
    ProtocolAdapter(shared_ptr<ModbusGpioGateway> SGW) : MBGW(SGW) {}
    virtual ~ProtocolAdapter() {}
    virtual void process_packet(std::unique_ptr<MessageBuffer> packet) = 0;
    modbus_pdu_type_e pdu_type;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*PROTOCOL_ADAPTER_H*/
