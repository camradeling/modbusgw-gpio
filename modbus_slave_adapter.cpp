#include <stdint.h>
#include "modbus_slave_adapter.h"
//----------------------------------------------------------------------------------------------------------------------
void ModbusSlaveAdapter::process_packet(std::unique_ptr<MessageBuffer> packet)
{
    shared_ptr<ModbusGpioGateway> mbgw = MBGW.lock();
    if (!mbgw)
        return;
    if (!mbgw->currentSession)
        return;
    int offset = (MODBUS_TCP_PDU_TYPE == pdu_type) ? MODBUS_TCP_HEADER_OFFSET : 0;
    if (packet->Length() < offset + 2)
        return;
    mbap_header_s* header = (mbap_header_s*)packet->Data();
    if (header->length < packet->Length() - offset)
        return;
    ModbusPDU req;
    if (MODBUS_TCP_PDU_TYPE == pdu_type)
        req.transactionID = SWAP16(header->transaction_id);
    req.SlaveAddress = packet->Data()[MODBUS_REQUEST_SLAVE_ADDRESS_POSITION + offset];
    req.FunctionCode = packet->Data()[MODBUS_REQUEST_FUNCTION_CODE_POSITION + offset];
    req.reg = SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_ADDRESS_POSITION + offset]);
    switch (req.FunctionCode)
    {
    case MODBUS_READ_COIL_STATUS:
        req.cnt = SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_NUMBER_POSITION + offset]);
        break;
    case MODBUS_READ_HOLDING_REGISTERS:
    case MODBUS_READ_INPUT_REGISTERS:
        req.cnt = SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_NUMBER_POSITION + offset]);
        break;
    case MODBUS_FORCE_SINGLE_COIL:
        req.cnt = 1;
        req.values.push_back(SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_NUMBER_POSITION + offset]));
        break;
    case MODBUS_WRITE_SINGLE_REGISTER:
        req.cnt = 1;
        req.values.push_back(SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_NUMBER_POSITION + offset]));
        break;
    case MODBUS_LOOPBACK:
        req.cnt = 0;
        break;
    case MODBUS_WRITE_MULTIPLE_REGISTERS:
    {
        req.cnt = SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_NUMBER_POSITION + offset]);
        if (req.cnt > MAX_REGS_TO_WRITE) {
            fprintf(stderr, "too many registers to write %d\n", req.cnt);
            return;
        }
        int numbytes = packet->Data()[MODBUS_REQUEST_BYTES_NUMBER_POSITION + offset];
        if (numbytes > req.cnt * 2) {
            fprintf(stderr, "number of bytes to write %d mismatch number of registers %d\n", numbytes, req.cnt);
            return;
        }
        for (int i = 0; i < req.cnt; i++)
            req.values.push_back(SWAP16(*(uint16_t*)&packet->Data()[MODBUS_REQUEST_REGISTER_DATA_START + offset + i * 2]));
        break;
    }
    default:
        fprintf(stderr, "unknown modbus function code %d\n", req.FunctionCode);
        return;
    }
    fprintf(stderr, "request: ID: %04X, addr %d, func %d, register %04X, val/num %04X\n",
            req.transactionID, req.SlaveAddress, req.FunctionCode, req.reg, req.cnt);
    mbgw->handle_request(req);
}
//----------------------------------------------------------------------------------------------------------------------
