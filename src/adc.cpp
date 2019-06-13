#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <signal.h>
extern "C" {
  #include "iio_utils.h"
}
#include "adc.h"

ADC::ADC(int dev_num) {
	this->dev_num = dev_num;
  char *dev_dir_name = NULL;
  //find_type_by_name(device_name, "iio:device");
  if (dev_num < 0) {
    fprintf(stderr, "Failed to find the device%d\n", dev_num);
    ret = dev_num;
  }
  ret = asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
  EnableDisableChannels(dev_dir_name, 1);
}

/**
 * size_from_channelarray() - calculate the storage size of a scan
 * @channels:		the channel info array
 * @num_channels:	number of channels
 *
 * Has the side effect of filling the channels[i].location values used
 * in processing the buffer output.
 **/
int ADC::size_from_channelarray(struct iio_channel_info *channels, int num_channels)
{
	int bytes = 0;
	int i = 0;

	while (i < num_channels) {
		if (bytes % channels[i].bytes == 0)
			channels[i].location = bytes;
		else
			channels[i].location = bytes - bytes % channels[i].bytes
					       + channels[i].bytes;

		bytes = channels[i].location + channels[i].bytes;
		i++;
	}

	return bytes;
}

int ADC::EnableDisableChannels(char *dev_dir_name, int enable)
{
	const struct dirent *ent;
	char scanelemdir[256];
	DIR *dp;
	int ret;
  char *buf_dir_name = NULL;
  ret = build_channel_array(dev_dir_name, &channels, &num_channels);
	snprintf(scanelemdir, sizeof(scanelemdir),
		 FORMAT_SCAN_ELEMENTS_DIR, dev_dir_name);
	scanelemdir[sizeof(scanelemdir)-1] = '\0';

	dp = opendir(scanelemdir);
	if (!dp) {
		fprintf(stderr, "Enabling/disabling channels: can't open %s\n",
			scanelemdir);
		return -EIO;
	}

	ret = -ENOENT;
	while (ent = readdir(dp), ent) {
		if (iioutils_check_suffix(ent->d_name, "_en")) {
			printf("%sabling: %s\n",
			       enable ? "En" : "Dis",
			       ent->d_name);
			ret = write_sysfs_int(ent->d_name, scanelemdir,
					      enable);
			if (ret < 0)
				fprintf(stderr, "Failed to enable/disable %s\n",
					ent->d_name);
		}
	}


  ret = asprintf(&buf_dir_name,
		       "%siio:device%d/buffer", iio_dir, dev_num);

	/* Setup ring buffer parameters */
	ret = write_sysfs_int("length", buf_dir_name, buf_len);

	/* Enable the buffer */
	ret = write_sysfs_int("enable", buf_dir_name, 1);
	if (ret < 0) {
		fprintf(stderr,
			"Failed to enable buffer: %s\n", strerror(-ret));
	}

	scan_size = size_from_channelarray(channels, num_channels);
  printf("scan siez is %d", scan_size);
	data = (char*)malloc(scan_size * buf_len);
	if (!data) {
		ret = -ENOMEM;
	}

	ret = asprintf(&buffer_access, "/dev/iio:device%d", dev_num);
	if (ret < 0) {
		ret = -ENOMEM;
    printf("no buffer");
	}

	/* Attempt to open non blocking the access dev */
	fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
	if (fp == -1) { /* TODO: If it isn't there make the node */
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", buffer_access);
	}
}

void ADC::read_adc(float* values)
{
	int k;
		if (!noevents) {
			struct pollfd pfd = {
				.fd = fp,
				.events = POLLIN,
			};

			ret = poll(&pfd, 1, -1);
			if (ret < 0) {
				ret = -errno;
			} else if (ret == 0) {
				exit;
			}

			toread = buf_len;
		} else {
			usleep(timedelay);
			toread = 64;
		}

		read_size = read(fp, data, toread * scan_size);
		if (read_size < 0) {
			if (errno == EAGAIN) {
				fprintf(stderr, "nothing available\n");
				exit;
			} else {
        printf("exit000000000000000");
				exit;
			}
		}
		for (i = 0; i < read_size / scan_size; i++)
		  for (k = 0; k < num_channels; k++) {
		  	 float adc_val = print2byte(*(uint16_t *)((data + scan_size * i) + channels[k].location), &channels[k]);
				 values[k] = adc_val * v_ref / max_adc_val;
			}
}

/**
 * process_scan() - print out the values in SI units
 * @data:		pointer to the start of the scan
 * @channels:		information about the channels.
 *			Note: size_from_channelarray must have been called first
 *			      to fill the location offsets.
 * @num_channels:	number of channels
 **/
float* ADC::process_scan(char *data,
		  struct iio_channel_info *channels,
		  int num_channels)
{
	int k;
	float *val_read;
	for (k = 0; k < num_channels; k++)
		switch (channels[k].bytes) {
			/* only a few cases implemented so far */
		case 1:
      printf("1 byte");
			//print1byte(*(uint8_t *)(data + channels[k].location),
			//	   &channels[k]);
			break;
		case 2:
			val_read[k] = print2byte(*(uint16_t *)(data + channels[k].location),
				   &channels[k]);
			printf("%05f ", val_read[k]);
			break;
		case 4:
      printf("4 byte");
			//print4byte(*(uint32_t *)(data + channels[k].location),
			//	   &channels[k]);
			break;
		case 8:
      printf("8 byte");
			//print8byte(*(uint64_t *)(data + channels[k].location),
			//	   &channels[k]);
			break;
		default:
			break;
		}
	return val_read;
}

float ADC::print2byte(uint16_t input, struct iio_channel_info *info)
{
	/* First swap if incorrect endian */
	if (info->be)
		input = be16toh(input);
	else
		input = le16toh(input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input >>= info->shift;
	input &= info->mask;
	float ret;
	if (info->is_signed) {
		int16_t val = (int16_t)(input << (16 - info->bits_used)) >>
			      (16 - info->bits_used);
		ret = ((float)val + info->offset) * info->scale;
	} else {
		ret = ((float)input + info->offset) * info->scale;
	}
	return ret;
}
