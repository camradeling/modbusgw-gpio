#ifndef MODBUS_SLAVE_ADAPTER_H
#define MODBUS_SLAVE_ADAPTER_H
//----------------------------------------------------------------------------------------------------------------------
#include "modbus_gpio_gateway.h"
#include "protocol_adapter.h"
//----------------------------------------------------------------------------------------------------------------------
class ModbusSlaveAdapter : public ProtocolAdapter
{
public:
    ModbusSlaveAdapter(shared_ptr<ModbusGpioGateway> SGW, modbus_pdu_type_e tp) : ProtocolAdapter(SGW) { pdu_type = tp; }
    virtual ~ModbusSlaveAdapter() {}
    virtual void process_packet(std::unique_ptr<MessageBuffer> packet);
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*MODBUS_SLAVE_ADAPTER_H*/
