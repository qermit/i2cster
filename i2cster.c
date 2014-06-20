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

struct i2cster_event {
//	char name[I2CSTER_EVLEN];
	long reg;
//	long bytes_to_read;
	long bytes_to_write;
	struct list_head list;
	struct kobject kobj;
	char * buf;
};


struct i2cster_attribute {
         struct attribute attr;
         ssize_t (*show)(struct kobject *kobj, struct foo_attribute *attr, char *buf);
         ssize_t (*store)(struct kobject *kobj, struct foo_attribute *attr, const char *buf, size_t count);
};

static void i2cster_kobj_release(struct kobject *kobj)
{
	pr_debug("kobject: (%p): %s\n", kobj, __func__);
	kfree(kobj);
}

#define to_i2cster_attr(x) container_of(x, struct i2cster_attribute, attr)

 static ssize_t i2cster_attr_show(struct kobject *kobj,
                              struct attribute *attr,
                              char *buf)
 {
         struct i2cster_attribute *attribute;
         attribute = to_i2cster_attr(attr);
         if (!attribute->show)
                 return -EIO;
         return attribute->show(kobj, attribute, buf);
 }

 /*
  * Just like the default show function above, but this one is for when the
  * sysfs "store" is requested (when a value is written to a file.)
  */
 static ssize_t i2cster_attr_store(struct kobject *kobj,
                               struct attribute *attr,
                               const char *buf, size_t len)
 {
         struct i2cster_attribute *attribute;
         attribute = to_i2cster_attr(attr);
         if (!attribute->store)
                 return -EIO;
         return attribute->store(kobj, attribute, buf, len);
}


static const struct sysfs_ops i2cster_sysfs_ops = {
          .show = i2cster_attr_show,
          .store = i2cster_attr_store,
 };

static struct kobj_type i2cster_kobj_ktype = {
		.release        = i2cster_kobj_release,
		.sysfs_ops      = &i2cster_sysfs_ops,
};


static ssize_t event_get_offset(struct kobject *kobj,  struct attribute *attr, char *buf)
{
	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);

	ssize_t status;

	status = sprintf(buf, "%d\n", tmp_event->reg);
	return status;


}

static ssize_t event_get_size(struct kobject *kobj,  struct attribute *attr, char *buf)
{
	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);

	ssize_t status;

	status = sprintf(buf, "%d\n", tmp_event->bytes_to_write);
	return status;
}

//static ssize_t event_read(struct device *dev,
//		struct device_attribute *attr, const char *buf, size_t size)
//{
//	return -1;
//}
//
//static ssize_t event_write(struct device *dev,
//		struct device_attribute *attr, const char *buf, size_t size)
//{
//	return -1;
//}

//DEVICE_ATTR(offset, S_IRUSR, event_get_offset, NULL);
//DEVICE_ATTR(size, S_IRUSR, event_get_size, NULL);
//DEVICE_ATTR(read, S_IRUSR, event_read, NULL);
//DEVICE_ATTR(write, S_IWUSR, NULL, event_write);
static struct i2cster_attribute i2cster_attr_offset =
	__ATTR(offset,S_IRUSR | S_IRGRP, event_get_offset, NULL );

static struct i2cster_attribute i2cster_attr_size =
	__ATTR(size,S_IRUSR | S_IRGRP, event_get_offset, NULL );


static ssize_t i2cster_read_hex(struct kobject *kobj,  struct attribute *attr, char *buf)
{

	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);
	struct i2c_client *client = to_i2c_client(container_of(kobj->parent, struct device, kobj));
	struct i2cster_data * data = i2c_get_clientdata(client);
	char tmp;
	int i;
	int count = tmp_event->bytes_to_write *2;

	printk (KERN_ERR "Read hex - off: %d, size: %d\n", 0 , tmp_event->bytes_to_write);

	i2c_smbus_write_byte (client, tmp_event->reg);
	for(i = 0; i < count; i=i+2) {
			tmp = i2c_smbus_read_byte(client);
			sprintf(&buf[i],"%02X", tmp);
	}

	return count;
}

static ssize_t i2cster_write_hex(struct kobject *kobj,  struct attribute *attr, const char *buf, size_t count)
{

	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);
	struct i2c_client *client = to_i2c_client(container_of(kobj->parent, struct device, kobj));
	struct i2cster_data * data = i2c_get_clientdata(client);

	int status;
	char tmp;
	char tmp2[3];
	int i;

//	char *tmp_buf =  strim((char *)buf);


    tmp2[2] = 0;
//	i2c_smbus_write_byte_data (client, tmp_event->reg);
	for(i = 0; i < count; i=i+2) {
		tmp2[0] = buf[i];
		tmp2[1] = buf[i+1];
		status = kstrtouint(tmp2, 16, &tmp);
		if (status == 0)
			tmp = i2c_smbus_write_byte_data(client, i/2, tmp);
		else
			return status;
	}

	return count;
}



static struct i2cster_attribute i2cster_attr_hex =
	__ATTR(hex,S_IRUSR | S_IRGRP | S_IWUSR, i2cster_read_hex, i2cster_write_hex );


static struct attribute *i2cster_event_attributes[] = {
		&i2cster_attr_offset.attr,
		&i2cster_attr_size.attr,
		&i2cster_attr_hex.attr,
//		&dev_attr_read.attr,
//		&dev_attr_write.attr,
		NULL
};

struct i2cster_data {
	char val[I2CSTER_MAX_SIZE];
	struct list_head events_list;
	struct mutex userspace_clients_lock;
};

//#define to_i2cster_data(d) container_of(d, struct i2cster_data, dev)

//DEVICE_ATTR();
//FILE_ATTR();
static ssize_t i2cster_write_bin(struct file *filp, struct kobject *kobj, struct bin_attribute *bin_attr,
		char *buf, loff_t off, size_t count)
{
	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);
	struct i2c_client *client = to_i2c_client(container_of(kobj->parent, struct device, kobj));
	struct i2cster_data * data = i2c_get_clientdata(client);

	int i;
	printk (KERN_ERR "Write bin - off: %d, size: %d\n", off, count);

	if (off == 0 && count == tmp_event->bytes_to_write) {
//		i2c_smbus_write_byte (client, tmp_event->reg);
		for(i = 0; i < count; i++) {
			i2c_smbus_write_byte_data(client, tmp_event->reg + i,  buf[i]);
		}
		/// @todo tutaj mutex unlock
		return count;
	} else if (count == 0) {
		return count;
	} else
		return -EINVAL;

}

static ssize_t i2cster_read_bin(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr,
			   char *buf, loff_t off, size_t count)
{

	struct i2cster_event * tmp_event = container_of(kobj, struct i2cster_event, kobj);
	struct i2c_client *client = to_i2c_client(container_of(kobj->parent, struct device, kobj));
	struct i2cster_data * data = i2c_get_clientdata(client);
	char tmp;
	int i;

	printk (KERN_ERR "Read bin - off: %d, size: %d\n", off, count);

	if (off == 0 && count == tmp_event->bytes_to_write) {

//		/// @todo: tutaj mutex lock
//		if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
//			for (i = slice << 5; i < (slice + 1) << 5; i += 32)
//				if (i2c_smbus_read_i2c_block_data(client, i,
//						32, data->data + i)
//						!= 32)
//					goto exit;
//		} else {
//			for (i = 0; i < tmp_event.bytes_to_write; i++{
//					char tmp_b = i2c_smbus_read_byte_data(client, )
//			}
//
//		}
//		memset(buf, 'a', count);
		i2c_smbus_write_byte (client, tmp_event->reg);
		for(i = 0; i < count; i++) {
			buf[i] = i2c_smbus_read_byte(client);
		}
		/// @todo tutaj mutex unlock
		return count;
	} else if (count == 0) {
		return count;
	} else
		return -EINVAL;
}


static struct bin_attribute i2cster_bin_attr = {
	.attr = { .name = "bin", .mode = S_IRUGO | S_IWUGO , },
	.size = I2CSTER_MAX_SIZE, /* more or less standard */
	.write = i2cster_write_bin,
	.read = i2cster_read_bin,
};

//BIN_ATTR(offset,  S_IRUSR | S_IRGRP, event_get_offset, NULL, 1024);
//BIN_ATTR(size,  S_IRUSR | S_IRGRP, event_get_size, NULL, 1024);
static struct bin_attribute *i2cster_event_bin_attrs [] = {
//		&bin_attr_offset.attr,
//		&bin_attr_size.attr,
		NULL
};

static const struct attribute_group i2cster_event_group = {
	.attrs = i2cster_event_attributes,
	.bin_attrs = i2cster_event_bin_attrs,
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
	int	status, status2;


	char *tmp_reg;
	char *tmp_to_write;
//	char *tmp_to_read;

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

   dev_err(dev, "Creating new event: client(%d), tmp_event(%d), kobject(%d)",
		   client,
		   tmp_event,
		   &tmp_event->kobj);


	kobject_init_and_add(&tmp_event->kobj, &i2cster_kobj_ktype, &dev->kobj, buf);
//   kobject_init(&tmp_event->kobj);
//	tmp_event->kobj = kobject_create_and_add(buf, &dev->kobj);
//	if (tmp_event->kobj == NULL) {
//		status = -ENOMEM;
//		goto end_remove_tmp_event;
//	}

	i2cster_bin_attr.size = tmp_event->bytes_to_write;
	status = sysfs_create_bin_file(&tmp_event->kobj, &i2cster_bin_attr);


	if (status) {
		status = -ENOMEM;
		goto end_remove_kobject;
	}

	status = sysfs_create_group(&tmp_event->kobj, &i2cster_event_group);

	if (status) {
		status = -ENOMEM;
		goto end_remove_group;

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
end_remove_group:

end_remove_kobject:
	kobject_put(&tmp_event->kobj);
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
             if (strcmp(tmp_buf, tmp->kobj.name) == 0) {
            	 list_del(pos);
            	 sysfs_remove_group(&tmp->kobj, &i2cster_event_group);
            	 sysfs_remove_bin_file(&tmp->kobj, &i2cster_bin_attr);
            	 kobject_put(&tmp->kobj);
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
             sysfs_remove_group(&tmp->kobj, &i2cster_event_group);

             kobject_put(&tmp->kobj);
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
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C Ster Driver");
