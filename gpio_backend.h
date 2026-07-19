#ifndef GPIO_BACKEND_H
#define GPIO_BACKEND_H
//----------------------------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <string>
#include <vector>
//----------------------------------------------------------------------------------------------------------------------
#define GPIO_MAX_OUTPUTS 8
//----------------------------------------------------------------------------------------------------------------------
struct GpioConfig
{
    int gpio_numbers[GPIO_MAX_OUTPUTS];
    int num_outputs;
    uint16_t do_register;
    bool active_high;
};
//----------------------------------------------------------------------------------------------------------------------
class GpioBackend
{
public:
    GpioBackend();
    ~GpioBackend();
    bool init(const GpioConfig& cfg);
    void shutdown();
    void set_register(uint16_t value);
    uint16_t get_register() const;
    void set_coil(int index, bool on);
    bool get_coil(int index) const;
    uint16_t do_register_addr() const { return config.do_register; }
    int num_outputs() const { return config.num_outputs; }
private:
    void write_gpio(int index, bool on);
    void export_gpio(int gpio_num);
    void unexport_gpio(int gpio_num);
    void set_direction(int gpio_num, const char* dir);
    void set_value(int gpio_num, int val);
    GpioConfig config;
    uint16_t current_state;
    bool initialized;
    int value_fds[GPIO_MAX_OUTPUTS];
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*GPIO_BACKEND_H*/
