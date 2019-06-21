#ifndef ADC_H_
#define ADC_H_

/**
 * enum autochan - state for the automatic channel enabling mechanism
 */
enum autochan
{
	AUTOCHANNELS_DISABLED,
	AUTOCHANNELS_ENABLED,
	AUTOCHANNELS_ACTIVE,
};

class ADC
{
public:
	ADC(int dev_num);
	int EnableDisableChannels(int enable);
	int size_from_channelarray(struct iio_channel_info *channels, int num_channels);
	float *process_scan(char *data,
						struct iio_channel_info *channels,
						int num_channels);
	float print2byte(uint16_t input, struct iio_channel_info *info);
	void read_adc(float *values);

private:
	long long num_loops = -1;
	unsigned long timedelay = 1000000;
	unsigned long buf_len = 1;

	ssize_t i;
	unsigned long long j;
	unsigned long toread;
	int ret, c;
	int fp = -1;

	int num_channels = 8;
	char *trigger_name = NULL;

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

	char *dev_dir_name;
};
#endif // ADC_H_
