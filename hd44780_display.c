#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

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
	int pos;
	struct miscdevice mdev;
	struct i2c_adapter *adap;
	struct i2c_msg msg[1];
	unsigned char data;
};

static struct hd44780 hd44780;

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

static int hd44780_send(unsigned char val, int rs, int timeout)
{
	int ret = 0;
	int i;

	if (rs)
		hd44780.data |= 1 << RS;
	else
		hd44780.data &= ~(1 << RS);

	hd44780.data &= ~(0xF << 4);
	for (i = 4; i < 8; i++)
		hd44780.data |= !!(val & BIT(i)) << i;

	pr_debug("high nibble: data=%X\n", hd44780.data);

	hd44780_transfer();
	hd44780_strobe();

	hd44780.data &= ~(0xF << 4);
	for (i = 0; i < 4; i++)
		hd44780.data |= !!(val & BIT(i)) << (i + D4);

	pr_debug("low nibble: data=%X\n", hd44780.data);

	hd44780_transfer();
	hd44780_strobe();

	if (timeout)
		udelay(timeout);

	return ret;
}

static int hd44780_cmd(unsigned char cmd)
{
	return hd44780_send(cmd, 0, 120);
}

static int hd44780_data(unsigned char data)
{
	return hd44780_send(data, 1, 45);
}

static inline void hd44780_clear(void)
{
	hd44780_cmd(0x01);
}

static inline void hd44780_home(void)
{
	hd44780_cmd(0x02);
}

static long hd44780_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOSYS;
}

ssize_t hd44780_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int i;
	size_t _size;
	unsigned char data[16];

	_size = (size > 16) ? sizeof(data) : size;
	if (copy_from_user((void *)&data[0], buf, _size))
		return -EFAULT;

	pr_err("size=%d (%d)\n", _size, size);

	hd44780_clear();
	hd44780_home();

	for (i = 0; i < _size; i++) {
		pr_cont("%c", data[i]);
		if (data[i] == '\n')
			continue;
		hd44780_data(data[i]);
	}
	pr_cont("\n");

	return _size;
}

static const struct file_operations hd44780_fops = {
	.unlocked_ioctl = hd44780_ioctl,
	.write = hd44780_write,
};

static void hd44780_init(struct i2c_client *client)
{
	hd44780.pos = 0;
	hd44780.adap = client->adapter;
	hd44780.msg->addr = client->addr;
	hd44780.msg->flags = 0;
	hd44780.msg->len = 1;
	hd44780.msg->buf = &hd44780.data;

	hd44780.mdev.minor = MISC_DYNAMIC_MINOR;
	hd44780.mdev.name = "hd44780";
	hd44780.mdev.fops = &hd44780_fops;

	misc_register(&hd44780.mdev);
}

static void hd44780_release(void)
{
	misc_deregister(&hd44780.mdev);
}

static int hd44780_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	unsigned char cmd[] = {
		0x02, /* return home */
		0x20, /* 4-bit mode, 1 line */
		0x0C, /* display on, cursor off, blinking off */
		0x06, /* cursor increment */
		0x01, /* clear display */
	};
	unsigned char d[] = {0x54, 0x65, 0x73, 0x74, 0x21, 0x2A};

	hd44780_init(client);

	hd44780_clear();
	hd44780_home();
	hd44780_bl(1);

	for (i = 0; i < ARRAY_SIZE(cmd); i++)
		hd44780_cmd(cmd[i]);

	/* Data */
	for (i = 0; i < ARRAY_SIZE(d); i++)
		hd44780_data(d[i]);

	for (i = 0; i < 5; i++)
		hd44780_data(0x75);

	mdelay(500);

	hd44780_clear();
	hd44780_home();

	return 0;
}

static int hd44780_remove(struct i2c_client *client)
{
	hd44780_clear();
	hd44780_home();
	hd44780_bl(0);
	hd44780_release();
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
