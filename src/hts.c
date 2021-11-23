/** @file
 *  @brief HTS Service sample
 */

/*
 * Copyright (c) 2020 SixOctets Systems
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <drivers/sensor.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <drivers/adc.h>
// #ifdef CONFIG_TEMP_NRF5
// static const struct device *temp_dev = DEVICE_DT_GET_ANY(nordic_nrf_temp);
// #else
// static const struct device *temp_dev;
// #endif

//static uint8_t simulate_htm;

/**************** ADC start ******************************/

const struct device *ADC_DEVICE_NAME = DEVICE_DT_GET(DT_NODELABEL(adc));
//#define ADC_DEVICE_NAME		DEVICE_DT_GET(DT_NODELABEL(adc))//DT_ALIAS(adcctrl)
#define ADC_RESOLUTION		10
#define ADC_GAIN			ADC_GAIN_1_6
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define BUFFER_SIZE			6
#define BAD_ANALOG_READ -123


static bool _IsInitialized = false;
static uint8_t _LastChannel = 250;
static int16_t m_sample_buffer[BUFFER_SIZE];

// the channel configuration with channel not yet filled in
static struct adc_channel_cfg m_1st_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = 0, // gets set during init
	.differential	  = 0,
#if CONFIG_ADC_CONFIGURABLE_INPUTS
	.input_positive   = 0, // gets set during init
#endif
};

// return device* for the adc
static const struct device* getAdcDevice(void)
{
	return device_get_binding(ADC_DEVICE_NAME->name);
}

// initialize the adc channel
static const struct device* init_adc(int channel)
{
	int ret;
	const struct device *adc_dev = getAdcDevice();
	if(_LastChannel != channel)
	{
		_IsInitialized = false;
		_LastChannel = channel;
	}

	if ( adc_dev != NULL && !_IsInitialized)
	{
		// strangely channel_id gets the channel id and input_positive gets id+1
		m_1st_channel_cfg.channel_id = channel;
#if CONFIG_ADC_CONFIGURABLE_INPUTS
        m_1st_channel_cfg.input_positive = channel+1,
#endif
		ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
		if(ret != 0)
		{
			//LOG_INF("Setting up of the first channel failed with code %d", ret);
			adc_dev = NULL;
		}
		else
		{
			_IsInitialized = true;	// we don't have any other analog users
		}
	}
	
	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));
	return adc_dev;
}

// ------------------------------------------------
// read one channel of adc
// ------------------------------------------------
static int16_t readOneChannel(int channel)
{
	const struct adc_sequence sequence = {
		.options     = NULL,				// extra samples and callback
		.channels    = BIT(channel),		// bit mask of channels to read
		.buffer      = m_sample_buffer,		// where to put samples read
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,		// desired resolution
		.oversampling = 0,					// don't oversample
		.calibrate = 0						// don't calibrate
	};

	int ret;
	int16_t sample_value = BAD_ANALOG_READ;
	const struct device *adc_dev = init_adc(channel);
	if (adc_dev)
	{
		ret = adc_read(adc_dev, &sequence);
		if(ret == 0)
		{
			sample_value = m_sample_buffer[0];
		}
	}

	return sample_value;
}

// ------------------------------------------------
// high level read adc channel and convert to float voltage
// ------------------------------------------------
float AnalogRead(int channel)
{

	int16_t sv = readOneChannel(channel);
	if(sv == BAD_ANALOG_READ)
	{
		return sv;
	}

	// Convert the result to voltage
	// Result = [V(p) - V(n)] * GAIN/REFERENCE / 2^(RESOLUTION)
																				  
	int multip = 256;
	// find 2**adc_resolution
	switch(ADC_RESOLUTION)
				
	{
		default :
		case 8 :
			multip = 256;
			break;
		case 10 :
			multip = 1024;
			break;
		case 12 :
			multip = 4096;
			break;
		case 14 :
			multip = 16384;
			break;
	}
	
	// the 3.6 relates to the voltage divider being used in my circuit
	float fout = (sv * 3.6 / multip);
	return fout;
}
/**************** ADC end ******************************/

static uint8_t indicating;
static struct bt_gatt_indicate_params ind_params;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	//simulate_htm = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	//printk("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	indicating = 0U;
}

/* Health Thermometer Service Declaration */
BT_GATT_SERVICE_DEFINE(hts_svc,  //
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	/* more optional Characteristics */
);

void hts_init(void)
{
	//temp_dev = NULL;
}

void hts_indicate(void)
{
	/* Temperature measurements simulation */
	//struct sensor_value temp_value;

	
	static uint8_t htm[5];
	double temperature = AnalogRead(7);
	// if(temperature < 30) {
	// 	temperature+=1;
	// } else {
	// 	temperature = 20U;
	// }
	uint32_t mantissa;
	uint8_t exponent;

	//r = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP,
	//		       &temp_value);
	//temperature = sensor_value_to_double(&temp_value);

	mantissa = (uint32_t)(temperature * 100);
	exponent = (uint8_t)-2;

	htm[0] = 0; /* temperature in celcius */
	sys_put_le24(mantissa, (uint8_t *)&htm[1]);
	htm[4] = exponent;

	ind_params.attr = &hts_svc.attrs[2];
	ind_params.func = indicate_cb;
	ind_params.destroy = indicate_destroy;
	ind_params.data = &htm;
	ind_params.len = sizeof(htm);

	if (bt_gatt_indicate(NULL, &ind_params) == 0) {
		indicating = 1U;
	}
	
}
