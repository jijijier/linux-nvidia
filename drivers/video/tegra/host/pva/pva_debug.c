/*
 * PVA Debug Information file
 *
 * Copyright (c) 2017-2022, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include "dev.h"
#include "pva.h"
#include <uapi/linux/nvpva_ioctl.h>
#include "pva_vpu_ocd.h"

static void pva_read_crashdump(struct seq_file *s, struct pva_seg_info *seg_info)
{
	int i = 0;
	u32 *seg_addr = (u32 *) seg_info->addr;

	if (!seg_addr)
		return;

	for (i = 0; i < (seg_info->size >> 4);) {
		seq_printf(s, "0x%x 0x%x 0x%x 0x%x\n",
			seg_addr[i], seg_addr[i+1],
			seg_addr[i+2], seg_addr[i+3]);
		i = i + 4;
	}
}

static int pva_crashdump(struct seq_file *s, void *data)
{
	int err = 0;
	struct pva_crashdump_debugfs_entry *entry =
			(struct pva_crashdump_debugfs_entry *)s->private;
	struct pva *pva = entry->pva;

	err = nvhost_module_busy(pva->pdev);
	if (err) {
		nvhost_dbg_info("err in powering up pva\n");
		goto err_poweron;
	}

	pva_read_crashdump(s, &entry->seg_info);

	nvhost_module_idle(pva->pdev);

err_poweron:
	return err;
}

static int crashdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, pva_crashdump, inode->i_private);
}

static const struct file_operations pva_crashdump_fops = {
	.open = crashdump_open,
	.read = seq_read,
	.release = single_release,
};


static inline void print_version(struct seq_file *s,
				 const char *version_str,
				 const u32 version)
{
	const char type = PVA_EXTRACT(version, 31, 24, u8);
	const u32 major = PVA_EXTRACT(version, 23, 16, u32);
	const u32 minor = PVA_EXTRACT(version, 15, 8, u32);
	const u32 subminor = PVA_EXTRACT(version, 7, 0, u32);

	seq_printf(s, "%s: %c.%02u.%02u.%02u\n", version_str,
		   type, major, minor, subminor);
}

static int print_firmware_versions(struct seq_file *s, void *data)
{
	struct pva *pva = s->private;
	struct pva_version_info info;
	int ret = 0;

	ret = nvhost_module_busy(pva->pdev);
	if (ret < 0)
		goto err_poweron;

	ret = pva_get_firmware_version(pva, &info);
	if (ret < 0)
		goto err_get_firmware_version;

	nvhost_module_idle(pva->pdev);

	print_version(s, "pva_r5_version", info.pva_r5_version);
	print_version(s, "pva_compat_version", info.pva_compat_version);
	seq_printf(s, "pva_revision: %x\n", info.pva_revision);
	seq_printf(s, "pva_built_on: %u\n", info.pva_built_on);

	return 0;

err_get_firmware_version:
	nvhost_module_idle(pva->pdev);
err_poweron:
	return ret;
}

static int print_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, print_firmware_versions, inode->i_private);
}

static const struct file_operations print_version_fops = {
	.open = print_version_open,
	.read = seq_read,
	.release = single_release,
};

static int get_log_level(void *data, u64 *val)
{
	struct pva *pva = (struct pva *) data;

	*val = pva->log_level;
	return 0;
}

static int set_log_level(void *data, u64 val)
{
	struct pva *pva = (struct pva *) data;

	pva->log_level = val;
	if (pva->booted)
		return pva_set_log_level(pva, val, false);
	else
		return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(log_level_fops, get_log_level, set_log_level, "%llu");

static long vpu_ocd_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct pva_vpu_dbg_block *dbg_block = f->f_inode->i_private;
	int err = 0;

	switch (cmd) {
	case PVA_OCD_IOCTL_VPU_IO: {
		struct pva_ocd_ioctl_vpu_io_param io_param;

		if (copy_from_user(&io_param, (void __user *)arg,
				   sizeof(io_param))) {
			pr_err("failed copy ioctl buffer from user; size: %u",
			       _IOC_SIZE(cmd));
			err = -EFAULT;
			goto out;
		}
		err = pva_vpu_ocd_io(dbg_block, io_param.instr,
				     &io_param.data[0], io_param.n_write,
				     &io_param.data[0], io_param.n_read);
		if (err)
			goto out;

		err = copy_to_user((void __user *)arg, &io_param,
				   sizeof(io_param));
		if (err)
			goto out;

		break;
	}
	default:
		err = -ENOIOCTLCMD;
		break;
	}

out:
	return err;
}

static const struct file_operations pva_vpu_ocd_fops = {
	.unlocked_ioctl = vpu_ocd_ioctl
};

void pva_debugfs_init(struct platform_device *pdev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct pva *pva = pdata->private_data;
	struct dentry *de = pdata->debugfs;
	static const char *vpu_ocd_names[NUM_VPU_BLOCKS] = { "ocd_vpu0",
							     "ocd_vpu1" };
	int i, err;

	if (!de)
		return;

	pva->debugfs_entry_r5.pva = pva;
	pva->debugfs_entry_vpu0.pva = pva;
	pva->debugfs_entry_vpu1.pva = pva;

	debugfs_create_file("r5_crashdump", S_IRUGO, de,
				&pva->debugfs_entry_r5, &pva_crashdump_fops);
	debugfs_create_file("vpu0_crashdump", S_IRUGO, de,
				&pva->debugfs_entry_vpu0, &pva_crashdump_fops);
	debugfs_create_file("vpu1_crashdump", S_IRUGO, de,
				&pva->debugfs_entry_vpu1, &pva_crashdump_fops);
	debugfs_create_u32("submit_task_mode", S_IRUGO | S_IWUSR, de,
				 &pva->submit_task_mode);
	debugfs_create_bool("vpu_debug", 0644, de,
			   &pva->vpu_debug_enabled);
	debugfs_create_u32("r5_dbg_wait", 0644, de,
			   &pva->r5_dbg_wait);
	debugfs_create_bool("r5_timeout_enable", 0644, de,
			    &pva->timeout_enabled);
	debugfs_create_file("firmware_version", S_IRUGO, de, pva,
			    &print_version_fops);
	debugfs_create_u32("cg_disable", 0644, de, &pva->slcg_disable);
	debugfs_create_bool("vpu_perf_counters_enable", 0644, de,
			    &pva->vpu_perf_counters_enable);
	debugfs_create_file("log_level", 0644, de, pva, &log_level_fops);

	err = pva_vpu_ocd_init(pva);
	if (err == 0) {
		for (i = 0; i < NUM_VPU_BLOCKS; i++)
			debugfs_create_file(vpu_ocd_names[i], 0644, de,
					    &pva->vpu_dbg_blocks[i],
					    &pva_vpu_ocd_fops);
	} else {
		dev_err(&pva->pdev->dev, "VPU OCD initialization failed\n");
	}
}
