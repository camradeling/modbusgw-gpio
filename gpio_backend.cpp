#include "gpio_backend.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
//----------------------------------------------------------------------------------------------------------------------
#pragma GCC diagnostic ignored "-Wunused-result"
//----------------------------------------------------------------------------------------------------------------------
GpioBackend::GpioBackend()
    : current_state(0), initialized(false)
{
    memset(value_fds, -1, sizeof(value_fds));
    memset(&config, 0, sizeof(config));
}
//----------------------------------------------------------------------------------------------------------------------
GpioBackend::~GpioBackend()
{
    if (initialized)
        shutdown();
}
//----------------------------------------------------------------------------------------------------------------------
bool GpioBackend::init(const GpioConfig& cfg)
{
    config = cfg;
    for (int i = 0; i < config.num_outputs; i++)
    {
        export_gpio(config.gpio_numbers[i]);
        set_direction(config.gpio_numbers[i], "out");

        char path[64];
        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", config.gpio_numbers[i]);
        value_fds[i] = open(path, O_WRONLY);
        if (value_fds[i] < 0)
        {
            fprintf(stderr, "failed to open %s for writing\n", path);
            return false;
        }
    }
    current_state = 0;
    set_register(0);
    initialized = true;
    fprintf(stderr, "GPIO backend initialized: %d outputs, register 0x%04X, polarity %s\n",
            config.num_outputs, config.do_register, config.active_high ? "active_high" : "active_low");
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::shutdown()
{
    set_register(0);
    for (int i = 0; i < config.num_outputs; i++)
    {
        if (value_fds[i] >= 0)
        {
            close(value_fds[i]);
            value_fds[i] = -1;
        }
        unexport_gpio(config.gpio_numbers[i]);
    }
    initialized = false;
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::set_register(uint16_t value)
{
    current_state = value & ((1 << config.num_outputs) - 1);
    for (int i = 0; i < config.num_outputs; i++)
    {
        bool on = (current_state >> i) & 1;
        write_gpio(i, on);
    }
}
//----------------------------------------------------------------------------------------------------------------------
uint16_t GpioBackend::get_register() const
{
    return current_state;
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::set_coil(int index, bool on)
{
    if (index < 0 || index >= config.num_outputs)
        return;
    if (on)
        current_state |= (1 << index);
    else
        current_state &= ~(1 << index);
    write_gpio(index, on);
}
//----------------------------------------------------------------------------------------------------------------------
bool GpioBackend::get_coil(int index) const
{
    if (index < 0 || index >= config.num_outputs)
        return false;
    return (current_state >> index) & 1;
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::write_gpio(int index, bool on)
{
    if (value_fds[index] < 0)
        return;
    int hw_val = config.active_high ? (on ? 1 : 0) : (on ? 0 : 1);
    const char* str = hw_val ? "1" : "0";
    lseek(value_fds[index], 0, SEEK_SET);
    write(value_fds[index], str, 1);
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::export_gpio(int gpio_num)
{
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "failed to open /sys/class/gpio/export\n");
        return;
    }
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", gpio_num);
    write(fd, buf, len);
    close(fd);
    usleep(100000);
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::unexport_gpio(int gpio_num)
{
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0)
        return;
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", gpio_num);
    write(fd, buf, len);
    close(fd);
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::set_direction(int gpio_num, const char* dir)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "failed to open %s\n", path);
        return;
    }
    write(fd, dir, strlen(dir));
    close(fd);
}
//----------------------------------------------------------------------------------------------------------------------
void GpioBackend::set_value(int gpio_num, int val)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
    int fd = open(path, O_WRONLY);
    if (fd < 0)
        return;
    const char* str = val ? "1" : "0";
    write(fd, str, 1);
    close(fd);
}
//----------------------------------------------------------------------------------------------------------------------
