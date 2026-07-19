#ifndef MODBUS_GPIO_GATEWAY_H
#define MODBUS_GPIO_GATEWAY_H
//----------------------------------------------------------------------------------------------------------------------
#include "chanlib_export.h"
#include "mxml.h"
#include "programthread.h"
#include "modbus_client.h"
#include "gpio_backend.h"
//----------------------------------------------------------------------------------------------------------------------
#define CHPLWRITELOG(format,...) {if(CHPL->logger) CHPL->logger->write(CHPL->DebugValue,format,##__VA_ARGS__);}
//----------------------------------------------------------------------------------------------------------------------
typedef struct _Session
{
    weak_ptr<BasicChannel> ch;
    uint32_t fd = 0;
    uint32_t InSeq = 0;
    uint32_t OutSeq = 0;
} Session;
//----------------------------------------------------------------------------------------------------------------------
class ProtocolAdapter;
//----------------------------------------------------------------------------------------------------------------------
class ModbusGpioGateway : public ProgramThread, public std::enable_shared_from_this<ModbusGpioGateway>
{
public:
    ModbusGpioGateway(mxml_node_t* cnf = nullptr) : config(cnf) {}
    virtual ~ModbusGpioGateway() {}
    virtual void init_module();
    virtual void thread_job() {}
    virtual void dispatch_event(weak_ptr<BasicChannel> chan, shared_ptr<ProtocolAdapter> adapter);
    void handle_request(ModbusPDU& req);
    shared_ptr<ChanPool> CHPL;
    std::vector<Session> sessionsActive;
    Session* currentSession = nullptr;
    mxml_node_t* config = nullptr;
    weak_ptr<BasicChannel> uplinkChannel;
    shared_ptr<ProtocolAdapter> uplink_adapter;
    GpioBackend gpio;
    uint8_t slave_address = 1;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*MODBUS_GPIO_GATEWAY_H*/
