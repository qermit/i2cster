#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>



static const unsigned short normal_i2c[] = { 0x50, 0x51, 0x52, 0x53, 0x54,
					0x55, 0x56, 0x57, I2C_CLIENT_END };
#define I2CSTER_MAX_SIZE 512
#define I2CSTER_EVLEN 10


static ssize_t event_read(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	return -1;
}

static ssize_t event_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	return -1;
}

DEVICE_ATTR(read, S_IRUSR, event_read, NULL);
DEVICE_ATTR(write, S_IWUSR, NULL, event_write);

static struct attribute *i2cster_event_attributes[] = {
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	NULL
};

static const struct attribute_group i2cster_event_group = {
	.attrs = i2cster_event_attributes,
};

struct i2cster_event {
//	char name[I2CSTER_EVLEN];
	long reg;
//	long bytes_to_read;
	long bytes_to_write;
	struct list_head list;
	struct kobject * kobj;
	char * buf;
};

struct i2cster_data {
	char val[I2CSTER_MAX_SIZE];
	struct list_head events_list;
	struct mutex userspace_clients_lock;
};

//#define to_i2cster_data(d) container_of(d, struct i2cster_data, dev)

//DEVICE_ATTR();
//FILE_ATTR();

static ssize_t i2cster_read_bin(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr,
			   char *buf, loff_t off, size_t count)
{
//	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);
//	dev_err(dev, "Creating new event: %s (%s|%s) @%d\n", buf, tmp_reg,tmp_to_write, tmp_event);
//	printk(KERN_ERR "kobj: %d\n", kobj);


	struct i2c_client *client = to_i2c_client(container_of(kobj, struct i2cster_event, kobj));
	struct i2cster_data * data = i2c_get_clientdata(client);
	char tmp;
	struct i2cster_event * tmp_event;
	struct list_head *pos, *q;
	struct i2cster_event * final_event = NULL;


	printk (KERN_ERR "i2cster_read_rw client(@%d) %d + %d\n", client, off, count);
	// search for correct file
//	list_for_each_safe(pos, q, &data->events_list){
////		tmp_event = list_entry(pos, struct i2cster_event, list);
////		if (strcmp(kobj->name, tmp_event->kobj->name) == 0) {
////			final_event = tmp_event;
////			break;
////		}
//	}

	if (final_event == NULL)
		return -EINVAL;


//
	return -EINVAL;
//	if (off > I2CSTER_MAX_SIZE/2)
//		return 0;
//	if (off + count > I2CSTER_MAX_SIZE/2)
//		count = I2CSTER_MAX_SIZE/2 - off;
//
//	tmp = data->val[0];
//	data->val[0] = tmp+1;
////	memset(data->val, tmp+1, I2CSTER_MAX_SIZE*sizeof(char));
//	memset(buf, tmp, count);
//	return count;
}

static struct bin_attribute i2cster_bin_attr = {
	.attr = { .name = "raw", .mode = S_IRUGO, },
	.size = I2CSTER_MAX_SIZE, /* more or less standard */
//	.write = i2cster_write_raw,
	.read = i2cster_read_bin,
};

//static struct bin_attribute i2cster_bin_attr = {
//	.attr = { .name = "raw", .mode = S_IRUGO, },
//	.size = I2CSTER_MAX_SIZE, /* more or less standard */
////	.write = i2cster_write_raw,
//	.read = i2cster_read_hex,
//};


static ssize_t set_create_event(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct i2cster_event * tmp_event;
//	struct gpio_desc *desc;
	int	status, status2, status3;


	char *tmp_reg;
	char *tmp_to_write;
	char *tmp_to_read;

	struct i2c_client *client = to_i2c_client(dev);
	struct i2cster_data * data = i2c_get_clientdata(client);

	tmp_reg = strchr(buf, ' ');
	if (!tmp_reg) {
		dev_err(dev, "%s: Missing parameters\n", "new_device");
		return -EINVAL;
	}
	*tmp_reg = 0;
	tmp_reg++;

	tmp_to_write = strchr(tmp_reg, ' ');
	if (!tmp_to_write) {
		dev_err(dev, "%s: Missing parameters\n", "new_device");
		return -EINVAL;
	}
	*tmp_to_write = 0;
	tmp_to_write++;

//	tmp_to_read = strchr(tmp_to_write, ' ');
//	if (!tmp_to_read) {
//		dev_err(dev, "%s: Missing parameters\n", "new_device");
//		return -EINVAL;
//	}
//	*tmp_to_read = 0;
//	tmp_to_read++;


	tmp_event = kzalloc(sizeof(*tmp_event), GFP_KERNEL);


	status = kstrtol(tmp_reg, 0, &tmp_event->reg);
	status2 = kstrtol(tmp_to_write, 0, &tmp_event->bytes_to_write);
//	status3 = kstrtol(tmp_to_read, 0, &tmp_event->bytes_to_read);

	if(status || status2 ){
		status = -EINVAL;
		goto end_remove_tmp_event;
	}

	dev_err(dev, "Creating new client(@%d) event: %s (%d|%d) @%d\n", client, buf, tmp_event->reg,tmp_event->bytes_to_write, tmp_event);

	tmp_event->kobj = kobject_create_and_add(buf, &dev->kobj);
	if (tmp_event->kobj == NULL) {
		status = -ENOMEM;
		goto end_remove_tmp_event;
	}

	i2cster_bin_attr.size = tmp_event->bytes_to_write;
	status = sysfs_create_bin_file(tmp_event->kobj, &i2cster_bin_attr);

//	status = sysfs_create_group(tmp_event->kobj, &i2cster_event_group);

	if (status) {
		status = -ENOMEM;
		goto end_remove_kobject;
	}
//	status = kobject_init(&tmp_event->kobj, )

//	status = kobject_init_and_add(&tmp_event->kobj, &dev,
//	                                    NULL, "%s", buf);
//	if (status)
//       goto remove_tmp_event;


	mutex_lock(&data->userspace_clients_lock);
	list_add_tail(&(tmp_event->list), &(data->events_list));
	mutex_unlock(&data->userspace_clients_lock);

	return size;

end_remove_kobject:
	kobject_put(tmp_event->kobj);
end_remove_tmp_event:
	kzfree(tmp_event);
	return status;
}

//static void i2cster_remove_event()

static ssize_t set_remove_event(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	struct i2c_client *client = to_i2c_client(dev);
	struct i2cster_data * data = i2c_get_clientdata(client);

	struct list_head *pos, *q;
	struct i2cster_event * tmp;

	char *tmp_buf =  strim((char *)buf);

	dev_err(dev, "remove: %s\n", tmp_buf);

    list_for_each_safe(pos, q, &data->events_list){
             tmp=list_entry(pos, struct i2cster_event, list);
             if (strcmp(tmp_buf, tmp->kobj->name) == 0) {
            	 list_del(pos);
//            	 sysfs_remove_group(tmp->kobj, &i2cster_event_group);
            	 sysfs_remove_bin_file(tmp->kobj, &i2cster_bin_attr);
            	 kobject_put(tmp->kobj);
            	 kzfree(tmp);
            	 break;
             }
    }


	return size;
}


static DEVICE_ATTR(create_event, S_IWUSR,	NULL, set_create_event);
static DEVICE_ATTR(remove_event, S_IWUSR,	NULL, set_remove_event);
//static SENSOR_DEVICE_ATTR(temp1_max_hyst, S_IWUSR | S_IRUGO, show_temp, set_temp, 2);
//static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL, 0);

static struct attribute *i2cster_attributes[] = {
	&dev_attr_create_event.attr,
	&dev_attr_remove_event.attr,
//	&sensor_dev_attr_temp1_max_hyst.dev_attr.attr,

	NULL
};

static const struct attribute_group i2cster_group = {
	.attrs = i2cster_attributes,
};




static int
i2cster_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2cster_data *data;
	int status;

//	printk (KERN_ERR "i2cster_for adapter %d, endpoint %d\n", client->adapter->nr, client->addr);

//	u8 set_mask, clr_mask;
//	int new;
//	enum lm75_type kind = id->driver_data;
//
//	if (!i2c_check_functionality(client->adapter,
//			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
//	return -EIO;

	data = devm_kzalloc(&client->dev, sizeof(struct i2cster_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	mutex_init(&data->userspace_clients_lock);

	i2c_set_clientdata(client, data);
//	mutex_init(&data->update_lock);

	/* Set to LM75 resolution (9 bits, 1/2 degree C) and range.
	 * Then tweak to be more precise when appropriate.
	 */
//	set_mask = 0;
//	clr_mask = LM75_SHUTDOWN;		/* continuous conversions */


//	/* configure as specified */
//	status = lm75_read_value(client, LM75_REG_CONF);
//	if (status < 0) {
//		dev_dbg(&client->dev, "Can't read config? %d\n", status);
//		return status;
//	}
//	data->orig_conf = status;
//	new = status & ~clr_mask;
//	new |= set_mask;
//	if (status != new)
//		lm75_write_value(client, LM75_REG_CONF, new);
//	dev_dbg(&client->dev, "Config %02x\n", new);
//
//	/* Register sysfs hooks */

	INIT_LIST_HEAD(&data->events_list);

	status = sysfs_create_group(&client->dev.kobj, &i2cster_group);
	if (status)
		return status;
//
//	data->hwmon_dev = hwmon_device_register(&client->dev);
//	if (IS_ERR(data->hwmon_dev)) {
//		status = PTR_ERR(data->hwmon_dev);
//		goto exit_remove;
//	}
//
//	dev_info(&client->dev, "%s: sensor '%s'\n",
//		 dev_name(data->hwmon_dev), client->name);

	return 0;

//exit_remove:
//	sysfs_remove_group(&client->dev.kobj, &lm75_group);
//	return status;
}

static int i2cster_remove(struct i2c_client *client)
{
	struct list_head *pos, *q;
	struct i2cster_data *data = i2c_get_clientdata(client);
	struct i2cster_event * tmp;

	mutex_lock(&data->userspace_clients_lock);
    list_for_each_safe(pos, q, &data->events_list){
             tmp=list_entry(pos, struct i2cster_event, list);
             list_del(pos);
             sysfs_remove_group(tmp->kobj, &i2cster_event_group);

             kobject_put(tmp->kobj);
             kzfree(tmp);
    }

	list_del(&data->events_list);
	mutex_unlock(&data->userspace_clients_lock);

	sysfs_remove_group(&client->dev.kobj, &i2cster_group);

	mutex_destroy(&data->userspace_clients_lock);
//	sysfs_remove_bin_file(&client->dev.kobj, &i2cster_bin_attr);

	/// @todo: sprawdzic czy nie trzeba zrobic kfree(data);


	return 0;
}
static int i2cster_detect(struct i2c_client *new_client,
		       struct i2c_board_info *info)
{
	printk (KERN_ERR "i2cster for adapter %d, endpoint %d\n", new_client->adapter->nr, new_client->addr);

	return -ENODEV;
}

static const struct i2c_device_id i2cster_id[] = {
	{ "i2cster", 0 },
	{ }
};

static struct i2c_driver i2cster_driver = {
	.driver = {
		.name	= "i2cster",
	},
	.id_table	= i2cster_id,

	.probe		= i2cster_probe,
	.remove		= i2cster_remove,
	.detect		= i2cster_detect,
//	.address_list	= normal_i2c,
};

module_i2c_driver(i2cster_driver);

MODULE_AUTHOR("Piotr Miedzik <qermit@sezamkowa.net>");
MODULE_DESCRIPTION("I2C Ster Driver");
MODULE_LICENSE("GPL");
