#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

static int hd44780_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	pr_err("%s: enter\n", __func__);
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
