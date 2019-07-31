#ifndef VOSION_ADC_H_
#define VOSION_ADC_H_

#include <unistd.h>
#include <inttypes.h>

class ADC
{
public:
    ADC(){};
    int init();
    void print_usage(void);
    void cleanup(void);
    void sig_handler(int signum);
    //void register_cleanup(void);
    void read_adc(float *values);
    void set_device_num(int dev_num);

private:
    int size_from_channelarray(struct iio_channel_info *channels, int num_channels);
    void print1byte(uint8_t input, struct iio_channel_info *info);
    void print2byte(uint16_t input, struct iio_channel_info *info);
    void print4byte(uint32_t input, struct iio_channel_info *info);
    void print8byte(uint64_t input, struct iio_channel_info *info);
    void process_scan(char *data,
                      struct iio_channel_info *channels,
                      int num_channels);
    int enable_disable_all_channels(char *dev_dir_name, int enable);
    int enable_disable_one_channel(char *dev_dir_name, int enable, int channel_num);
    float read2byte(uint16_t input, struct iio_channel_info *info);
    long long num_loops = 2;
    unsigned long timedelay = 1000000;
    unsigned long buf_len = 128;

    ssize_t i;
    unsigned long long j;
    unsigned long toread;
    int ret, c;
    int fp = -1;

    int num_channels = 0;
    char *trigger_name = NULL, *device_name = NULL;

    char *data = NULL;
    ssize_t read_size;
    int dev_num = -1, trig_num = -1;
    char *buffer_access = NULL;
    int scan_size;
    int noevents = 0;
    int notrigger = 0;
    char *dummy;
    bool force_autochannels = false;

    struct iio_channel_info *channels = NULL;

    float v_ref = 1.8;
    float max_adc_val = 4095;
};

#endif //VOSION_ADC_H_