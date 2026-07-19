#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string>
//----------------------------------------------------------------------------------------------------------------------
#include "modbus_gpio_gateway.h"
#include "chanlib_export.h"
#include "ini_parser.h"
//----------------------------------------------------------------------------------------------------------------------
static const char* defaultConf = "/etc/modbusgw-gpio/config.ini";
static shared_ptr<ModbusGpioGateway> mgw;
//----------------------------------------------------------------------------------------------------------------------
static void signal_handler(int sig)
{
    fprintf(stderr, "signal %d received, shutting down\n", sig);
    if (mgw)
    {
        mgw->gpio.shutdown();
        mgw->stop = true;
    }
}
//----------------------------------------------------------------------------------------------------------------------
static mxml_node_t* build_channel_xml(int port)
{
    char xml_str[512];
    snprintf(xml_str, sizeof(xml_str),
        "<?xml version=\"1.0\"?>"
        "<Configuration Version=\"2.0\">"
        "<Logger>true</Logger>"
        "<Channels>"
        "<Channel Type=\"TCP\" Alias=\"TCP1\" Function=\"ModbusTCPSlave\">"
        "<ListenPort>%d</ListenPort>"
        "</Channel>"
        "</Channels>"
        "</Configuration>",
        port);
    return mxmlLoadString(NULL, xml_str, MXML_NO_CALLBACK);
}
//----------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::string configfile;
    int option_index = 0;
    int c;
    static struct option long_options[] = {
        {"config", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}
    };
    while ((c = getopt_long(argc, argv, "c:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 0:
            if (option_index == 0)
                configfile = std::string(optarg);
            break;
        case 'c':
            configfile = std::string(optarg);
            break;
        case '?':
            fprintf(stderr, "unknown option: %c\n", option_index);
            break;
        }
    }
    if (configfile.empty())
        configfile = std::string(defaultConf);
    fprintf(stderr, "configuration file = %s\n", configfile.c_str());

    IniParser ini;
    if (!ini.load(configfile))
    {
        fprintf(stderr, "cant open config file %s\n", configfile.c_str());
        return -1;
    }

    int port = ini.get_int("modbus", "port", 502);
    int slave_addr = ini.get_int("modbus", "slave_address", 1);

    GpioConfig gpio_cfg;
    memset(&gpio_cfg, 0, sizeof(gpio_cfg));
    gpio_cfg.do_register = (uint16_t)ini.get_int("gpio", "do_register", 1);
    gpio_cfg.num_outputs = ini.get_int("gpio", "num_outputs", 8);
    if (gpio_cfg.num_outputs > GPIO_MAX_OUTPUTS)
        gpio_cfg.num_outputs = GPIO_MAX_OUTPUTS;
    std::string polarity = ini.get("gpio", "polarity", "active_high");
    gpio_cfg.active_high = (polarity != "active_low");

    for (int i = 0; i < gpio_cfg.num_outputs; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "do%d", i);
        gpio_cfg.gpio_numbers[i] = ini.get_int("gpio", key, 0);
        if (gpio_cfg.gpio_numbers[i] == 0)
        {
            fprintf(stderr, "gpio number not configured for %s\n", key);
            return -1;
        }
    }

    fprintf(stderr, "modbus port=%d slave_address=%d\n", port, slave_addr);
    fprintf(stderr, "gpio: %d outputs, register=0x%04X, polarity=%s\n",
            gpio_cfg.num_outputs, gpio_cfg.do_register, gpio_cfg.active_high ? "active_high" : "active_low");

    mxml_node_t* tree = build_channel_xml(port);
    if (!tree)
    {
        fprintf(stderr, "failed to build channel config\n");
        return -1;
    }

    mgw = shared_ptr<ModbusGpioGateway>(new ModbusGpioGateway(tree));
    mgw->slave_address = (uint8_t)slave_addr;

    if (!mgw->gpio.init(gpio_cfg))
    {
        fprintf(stderr, "GPIO initialization failed\n");
        return -1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    mgw->init_module();
    fprintf(stderr, "running...\n");
    while (!mgw->stop)
        sleep(1);

    fprintf(stderr, "exiting\n");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
