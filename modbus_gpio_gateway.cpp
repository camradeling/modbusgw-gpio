#include "mxmlconf.h"
#include "modbus_gpio_gateway.h"
#include "modbus_slave_adapter.h"
//----------------------------------------------------------------------------------------------------------------------
void ModbusGpioGateway::init_module()
{
    CHPL = shared_ptr<ChanPool>(new ChanPool());
    CHPL->chp = CHPL;
    mxml_node_t* loggernode = mxmlFindElement(config, config, "Logger", NULL, NULL, MXML_DESCEND);
    if (loggernode)
    {
        const char* ext = mxmlGetText(loggernode, NULL);
        if (ext && string(ext) == "true") CHPL->logger = new ChannelLib::Logger;
    }
    shared_ptr<ChanPoolConfig> conf = shared_ptr<ChanPoolConfig>(mxml_parse_config(config));
    CHPL->init(conf.get());
    for (int i = 0; i < CHPL->allChan.size(); i++)
    {
        weak_ptr<BasicChannel> chan = CHPL->allChan.at(i);
        shared_ptr<BasicChannel> schan = chan.lock();
        CHPLWRITELOG("channel alias = %s\n", schan->alias.c_str());
        shared_ptr<ProtocolAdapter> adapter;
        if (schan->funcName == "ModbusTCPSlave")
        {
            CHPLWRITELOG("function = ModbusTCPSlave\n");
            adapter = uplink_adapter = shared_ptr<ProtocolAdapter>(new ModbusSlaveAdapter(shared_from_this(), MODBUS_TCP_PDU_TYPE));
            uplinkChannel = schan;
        }
        if (!adapter)
            continue;
        add_pollable_handler(schan->inCmdQueue.fd(), EPOLLIN, &ModbusGpioGateway::dispatch_event, this, chan, adapter);
        add_pollable_handler(schan->inQueue.fd(), EPOLLIN, &ModbusGpioGateway::dispatch_event, this, chan, adapter);
    }
    ProgramThread::init_module();
}
//----------------------------------------------------------------------------------------------------------------------
void ModbusGpioGateway::dispatch_event(weak_ptr<BasicChannel> chan, shared_ptr<ProtocolAdapter> adapter)
{
    shared_ptr<BasicChannel> schan = chan.lock();
    if (!schan)
        return;
    std::unique_ptr<MessageBuffer> packet;
    while ((packet = schan->inCmdQueue.pop()) && !stop)
    {
        enum MessageType packetType = packet->Type();
        if (packetType == CHAN_OPEN_PACKET)
        {
            Session ses;
            ses.fd = packet->getfd();
            CHPLWRITELOG("[%d] connection established\n", ses.fd);
            ses.ch = schan;
            sessionsActive.push_back(ses);
            currentSession = &sessionsActive.at(sessionsActive.size() - 1);
        }
        else if (packetType == CHAN_CLOSE_PACKET)
        {
            for (int j = 0; j < sessionsActive.size(); j++)
            {
                if (sessionsActive.at(j).fd == packet->getfd())
                {
                    sessionsActive.erase(sessionsActive.begin() + j);
                    break;
                }
            }
            currentSession = nullptr;
            CHPLWRITELOG("[%d] channel closed\n", packet->getfd());
        }
    }
    while ((packet = schan->inQueue.pop()) && !stop)
    {
        enum MessageType packetType = packet->Type();
        currentSession = nullptr;
        if (packetType == CHAN_DATA_PACKET)
        {
            for (int j = 0; j < sessionsActive.size(); j++)
            {
                if (sessionsActive.at(j).fd == packet->getfd())
                {
                    currentSession = &sessionsActive[j];
                    currentSession->InSeq = packet->seqnum;
                    adapter->process_packet(std::move(packet));
                    break;
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
void ModbusGpioGateway::handle_request(ModbusPDU& req)
{
    if (req.SlaveAddress != slave_address && req.SlaveAddress != 0)
    {
        fprintf(stderr, "request for slave %d, ignoring (our address: %d)\n", req.SlaveAddress, slave_address);
        return;
    }

    ModbusPDU reply;
    reply.transactionID = req.transactionID;
    reply.SlaveAddress = slave_address;
    reply.FunctionCode = req.FunctionCode;
    reply.reg = req.reg;

    switch (req.FunctionCode)
    {
    case MODBUS_READ_HOLDING_REGISTERS:
    case MODBUS_READ_INPUT_REGISTERS:
    {
        reply.cnt = req.cnt;
        for (int i = 0; i < req.cnt; i++)
        {
            uint16_t reg_addr = req.reg + i;
            if (reg_addr == gpio.do_register_addr())
                reply.values.push_back(gpio.get_register());
            else
                reply.values.push_back(0);
        }
        break;
    }
    case MODBUS_WRITE_SINGLE_REGISTER:
    {
        reply.cnt = 1;
        uint16_t val = req.values[0];
        if (req.reg == gpio.do_register_addr())
            gpio.set_register(val);
        reply.values.push_back(val);
        fprintf(stderr, "FC06: register 0x%04X = 0x%04X\n", req.reg, val);
        break;
    }
    case MODBUS_READ_COIL_STATUS:
    {
        int num_coils = req.cnt;
        int bytes_needed = (num_coils + 7) / 8;
        reply.cnt = bytes_needed;
        for (int byte_idx = 0; byte_idx < bytes_needed; byte_idx++)
        {
            uint8_t byte_val = 0;
            for (int bit = 0; bit < 8; bit++)
            {
                int coil_idx = byte_idx * 8 + bit;
                if (coil_idx >= num_coils)
                    break;
                if (gpio.get_coil(req.reg + coil_idx))
                    byte_val |= (1 << bit);
            }
            reply.values.push_back(byte_val);
        }
        break;
    }
    case MODBUS_FORCE_SINGLE_COIL:
    {
        reply.cnt = 1;
        bool on = (req.values[0] == 0xFF00 || req.values[0] == 0xFF);
        gpio.set_coil(req.reg, on);
        reply.values.push_back(on ? 0xFF00 : 0x0000);
        fprintf(stderr, "FC05: coil %d = %s\n", req.reg, on ? "ON" : "OFF");
        break;
    }
    case MODBUS_LOOPBACK:
    {
        reply.cnt = 0;
        break;
    }
    case MODBUS_WRITE_MULTIPLE_REGISTERS:
    {
        reply.cnt = req.cnt;
        for (int i = 0; i < req.cnt; i++)
        {
            uint16_t reg_addr = req.reg + i;
            if (reg_addr == gpio.do_register_addr())
                gpio.set_register(req.values[i]);
        }
        fprintf(stderr, "FC16: %d registers from 0x%04X\n", req.cnt, req.reg);
        break;
    }
    default:
        fprintf(stderr, "unsupported function code %d\n", req.FunctionCode);
        return;
    }

    shared_ptr<BasicChannel> schan = uplinkChannel.lock();
    if (!schan)
    {
        fprintf(stderr, "no uplink channel\n");
        return;
    }
    int fd = 0;
    for (auto& session : sessionsActive)
    {
        if (session.ch.lock() == schan)
        {
            fd = session.fd;
            break;
        }
    }
    std::unique_ptr<MessageBuffer> reply_packet(new MessageBuffer(fd, ModbusPacketConstructor::serialize_reply(reply, MODBUS_TCP_PDU_TYPE), CHAN_DATA_PACKET));
    schan->send_message_buffer(&schan->outQueue, std::move(reply_packet), true);
}
//----------------------------------------------------------------------------------------------------------------------
