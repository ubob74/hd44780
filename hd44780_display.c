#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

enum hd44780_pins {
	RS,
	RW,
	E,
	BL,
	D4,
	D5,
	D6,
	D7,
};

struct hd44780 {
	struct i2c_adapter *adap;
	struct i2c_msg msg[1];
	unsigned char data;
};

static struct hd44780 hd44780;

static void hd44780_init(struct i2c_client *client)
{
	hd44780.adap = client->adapter;
	hd44780.msg->addr = client->addr;
	hd44780.msg->flags = 0;
	hd44780.msg->len = 1;
	hd44780.msg->buf = &hd44780.data;
}

static int hd44780_transfer(void)
{
	return i2c_transfer(hd44780.adap, hd44780.msg, 1);
}

static int hd44780_bl(int bl)
{
	if (bl)
		hd44780.data |= 1 << BL;
	else
		hd44780.data &= ~(1 << BL);
	udelay(20);
	return hd44780_transfer();
}

static int hd44780_strobe(void)
{
	int ret;

	hd44780.data |= 1 << E;
	udelay(20);
	ret = hd44780_transfer();

	hd44780.data &= ~(1 << E);
	udelay(40);
	ret = hd44780_transfer();

	return ret;
}

static int hd44780_write(unsigned char val, int rs, int timeout)
{
	int ret;
	int i;

	if (rs)
		hd44780.data |= rs << RS;
	else
		hd44780.data &= ~(rs << RS);

	hd44780.data &= ~(0xF << 4);
	for (i = 4; i < 8; i++)
		hd44780.data |= !!(val & BIT(i)) << i;

	pr_debug("high nibble: data=%X\n", hd44780.data);

	ret = hd44780_transfer();
	pr_err("transfer ret=%d\n", ret);

	hd44780_strobe();

	hd44780.data &= ~(0xF << 4);
	for (i = 0; i < 4; i++)
		hd44780.data |= !!(val & BIT(i)) << (i + D4);

	pr_debug("low nibble: data=%X\n", hd44780.data);

	ret = hd44780_transfer();

	hd44780_strobe();

	if (timeout)
		udelay(timeout);

	return ret;
}

static int hd44780_cmd(unsigned char cmd)
{
	return hd44780_write(cmd, 0, 120);
}

static int hd44780_data(unsigned char data)
{
	return hd44780_write(data, 1, 45);
}

static void hd44780_blink_test(void)
{
	hd44780_bl(1);
	mdelay(500);
	hd44780_bl(0);
	mdelay(500);
	hd44780_bl(1);
}

static int hd44780_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	unsigned char cmd[] = {
		0x02, /* return home */
		0x20, /* 4-bit mode */
		0x0C, /* display on, cursor off, blinking off */
		0x06, /* cursor increment */
		0x01, /* clear display */
	};
	unsigned char d[] = {0x54, 0x65, 0x73, 0x74, 0x21, 0x2A};

	hd44780_init(client);

	hd44780_blink_test();

	for (i = 0; i < ARRAY_SIZE(cmd); i++)
		hd44780_cmd(cmd[i]);

	/* Data */
	for (i = 0; i < ARRAY_SIZE(d); i++)
		hd44780_data(d[i]);

	return 0;
}

static int hd44780_remove(struct i2c_client *client)
{
	return 0;
}

static const struct of_device_id hd44780_match[] = {
	{ .compatible = "display,hd44780", },
	{}
};
MODULE_DEVICE_TABLE(of, hd44780_match);

static struct i2c_driver hd44780_driver = {
	.probe  = hd44780_probe,
	.remove = hd44780_remove,
	.driver = {
		.name       = "hd44780",
		.of_match_table = of_match_ptr(hd44780_match),
	},
};

module_i2c_driver(hd44780_driver);

MODULE_LICENSE("GPL");
