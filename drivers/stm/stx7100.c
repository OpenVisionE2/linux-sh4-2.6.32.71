#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/ethtool.h>
#include <linux/dma-mapping.h>
#include <linux/ata_platform.h>
#include <linux/mtd/partitions.h>
#include <linux/stm/pad.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/emi.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7100.h>
#include <asm/irq-ilc.h>



/* EMI resources ---------------------------------------------------------- */

static int __initdata stx7100_emi_bank_configured[EMI_BANKS];



/* PATA resources --------------------------------------------------------- */

/* EMI A21 = CS1 (active low)
 * EMI A20 = CS0 (active low)
 * EMI A19 = DA2
 * EMI A18 = DA1
 * EMI A17 = DA0 */
static struct resource stx7100_pata_resources[] = {
	/* I/O base: CS1=N, CS0=A */
	[0] = STM_PLAT_RESOURCE_MEM(1 << 21, 8 << 17),
	/* CTL base: CS1=A, CS0=N, DA2=A, DA1=A, DA0=N */
	[1] = STM_PLAT_RESOURCE_MEM((1 << 20) + (6 << 17), 4),
	/* IRQ */
	[2] = STM_PLAT_RESOURCE_IRQ(-1, -1),
};

static struct platform_device stx7100_pata_device = {
	.name		= "pata_platform",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(stx7100_pata_resources),
	.resource	= stx7100_pata_resources,
	.dev.platform_data = &(struct pata_platform_info) {
		.ioport_shift	= 17,
	},
};

void __init stx7100_configure_pata(struct stx7100_pata_config *config)
{
	unsigned long bank_base;

	if (!config) {
		BUG();
		return;
	}

	BUG_ON(config->emi_bank < 0 || config->emi_bank >= EMI_BANKS);
	BUG_ON(stx7100_emi_bank_configured[config->emi_bank]);
	stx7100_emi_bank_configured[config->emi_bank] = 1;

	bank_base = emi_bank_base(config->emi_bank);

	stx7100_pata_resources[0].start += bank_base;
	stx7100_pata_resources[0].end += bank_base;
	stx7100_pata_resources[1].start += bank_base;
	stx7100_pata_resources[1].end += bank_base;
	stx7100_pata_resources[2].start = config->irq;
	stx7100_pata_resources[2].end = config->irq;

	emi_config_pata(config->emi_bank, config->pc_mode);

	platform_device_register(&stx7100_pata_device);
}



/* FDMA resources --------------------------------------------------------- */

#ifdef CONFIG_STM_DMA

#include "fdma_firmware_7100.h"
#include "fdma_firmware_7109c2.h"
#include "fdma_firmware_7109c3.h"

static struct stm_plat_fdma_hw stx7100_fdma_hw = {
	.slim_regs = {
		.id       = 0x0000 + (0x000 << 2), /* 0x0000 */
		.ver      = 0x0000 + (0x001 << 2), /* 0x0004 */
		.en       = 0x0000 + (0x002 << 2), /* 0x0008 */
		.clk_gate = 0x0000 + (0x003 << 2), /* 0x000c */
	},
	.periph_regs = {
		.sync_reg = 0x8000 + (0xfe2 << 2), /* 0xbf88 */
		.cmd_sta  = 0x8000 + (0xff0 << 2), /* 0xbfc0 */
		.cmd_set  = 0x8000 + (0xff1 << 2), /* 0xbfc4 */
		.cmd_clr  = 0x8000 + (0xff2 << 2), /* 0xbfc8 */
		.cmd_mask = 0x8000 + (0xff3 << 2), /* 0xbfcc */
		.int_sta  = 0x8000 + (0xff4 << 2), /* 0xbfd0 */
		.int_set  = 0x8000 + (0xff5 << 2), /* 0xbfd4 */
		.int_clr  = 0x8000 + (0xff6 << 2), /* 0xbfd8 */
		.int_mask = 0x8000 + (0xff7 << 2), /* 0xbfdc */
	},
	.dmem_offset = 0x8000,
	.dmem_size   = 0x600 << 2, /* 1536 * 4 = 6144 */
	.imem_offset = 0xc000,
	.imem_size   = 0xa00 << 2, /* 2560 * 4 = 10240 */
};

static struct stm_plat_fdma_data stx7100_fdma_platform_data = {
	.hw = &stx7100_fdma_hw,
	.fw = &stm_fdma_firmware_7100,
	.min_ch_num = CONFIG_MIN_STM_DMA_CHANNEL_NR,
	.max_ch_num = CONFIG_MAX_STM_DMA_CHANNEL_NR,
};

static struct stm_plat_fdma_hw stx7109c2_fdma_hw = {
	.slim_regs = {
		.id       = 0x0000 + (0x000 << 2), /* 0x0000 */
		.ver      = 0x0000 + (0x001 << 2), /* 0x0004 */
		.en       = 0x0000 + (0x002 << 2), /* 0x0008 */
		.clk_gate = 0x0000 + (0x003 << 2), /* 0x000c */
	},
	.periph_regs = {
		.sync_reg = 0x8000 + (0xfe2 << 2), /* 0xbf88 */
		.cmd_sta  = 0x8000 + (0xff0 << 2), /* 0xbfc0 */
		.cmd_set  = 0x8000 + (0xff1 << 2), /* 0xbfc4 */
		.cmd_clr  = 0x8000 + (0xff2 << 2), /* 0xbfc8 */
		.cmd_mask = 0x8000 + (0xff3 << 2), /* 0xbfcc */
		.int_sta  = 0x8000 + (0xff4 << 2), /* 0xbfd0 */
		.int_set  = 0x8000 + (0xff5 << 2), /* 0xbfd4 */
		.int_clr  = 0x8000 + (0xff6 << 2), /* 0xbfd8 */
		.int_mask = 0x8000 + (0xff7 << 2), /* 0xbfdc */
	},
	.dmem_offset = 0x8000,
	.dmem_size   = 0x600 << 2, /* 1536 * 4 = 6144 */
	.imem_offset = 0xc000,
	.imem_size   = 0xa00 << 2, /* 2560 * 4 = 10240 */
};

static struct stm_plat_fdma_data stx7109c2_fdma_platform_data = {
	.hw = &stx7109c2_fdma_hw,
	.fw = &stm_fdma_firmware_7109c2,
	.min_ch_num = CONFIG_MIN_STM_DMA_CHANNEL_NR,
	.max_ch_num = CONFIG_MAX_STM_DMA_CHANNEL_NR,
};

static struct stm_plat_fdma_hw stx7109c3_fdma_hw = {
	.slim_regs = {
		.id       = 0x0000 + (0x000 << 2), /* 0x0000 */
		.ver      = 0x0000 + (0x001 << 2), /* 0x0004 */
		.en       = 0x0000 + (0x002 << 2), /* 0x0008 */
		.clk_gate = 0x0000 + (0x003 << 2), /* 0x000c */
	},
	.periph_regs = {
		.sync_reg = 0x8000 + (0xfe2 << 2), /* 0xbf88 */
		.cmd_sta  = 0x8000 + (0xff0 << 2), /* 0xbfc0 */
		.cmd_set  = 0x8000 + (0xff1 << 2), /* 0xbfc4 */
		.cmd_clr  = 0x8000 + (0xff2 << 2), /* 0xbfc8 */
		.cmd_mask = 0x8000 + (0xff3 << 2), /* 0xbfcc */
		.int_sta  = 0x8000 + (0xff4 << 2), /* 0xbfd0 */
		.int_set  = 0x8000 + (0xff5 << 2), /* 0xbfd4 */
		.int_clr  = 0x8000 + (0xff6 << 2), /* 0xbfd8 */
		.int_mask = 0x8000 + (0xff7 << 2), /* 0xbfdc */
	},
	.dmem_offset = 0x8000,
	.dmem_size   = 0x800 << 2, /* 2048 * 4 = 8192 */
	.imem_offset = 0xc000,
	.imem_size   = 0x1000 << 2, /* 4096 * 4 = 16384 */
};

static struct stm_plat_fdma_data stx7109c3_fdma_platform_data = {
	.hw = &stx7109c3_fdma_hw,
	.fw = &stm_fdma_firmware_7109c3,
	.min_ch_num = CONFIG_MIN_STM_DMA_CHANNEL_NR,
	.max_ch_num = CONFIG_MAX_STM_DMA_CHANNEL_NR,
};



#endif /* CONFIG_STM_DMA */

static struct platform_device stx7100_fdma_device = {
	.name		= "stm-fdma",
	.id		= -1,
	.num_resources	= 2,
	.resource = (struct resource[2]) {
		STM_PLAT_RESOURCE_MEM(0x19220000, 0x10000),
		STM_PLAT_RESOURCE_IRQ(140, -1),
	},
};

static void stx7100_fdma_setup(void)
{
#ifdef CONFIG_STM_DMA
	switch (cpu_data->type) {
	case CPU_STX7100:
		stx7100_fdma_device.dev.platform_data =
				&stx7100_fdma_platform_data;
		break;
	case CPU_STX7109:
		switch (cpu_data->cut_major) {
		case 1:
			BUG();
			break;
		case 2:
			stx7100_fdma_device.dev.platform_data =
					&stx7109c2_fdma_platform_data;
			break;
		default:
			stx7100_fdma_device.dev.platform_data =
					&stx7109c3_fdma_platform_data;
			break;
		}
		break;
	default:
		BUG();
		break;
	}
#endif
}



/* Hardware RNG resources ------------------------------------------------- */

static struct platform_device stx7100_rng_hwrandom_device = {
	.name = "stm-hwrandom",
	.id = -1,
	.num_resources = 1,
	.resource = (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0x19250000, 0x1000),
	}
};

static struct platform_device stx7100_rng_devrandom_device = {
	.name = "stm-rng",
	.id = -1,
	.num_resources = 1,
	.resource = (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0x19250000, 0x1000),
	}
};



/* PIO ports resources ---------------------------------------------------- */

static struct platform_device stx7100_pio_devices[] = {
	[0] = {
		.name = "stm-gpio",
		.id = 0,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18020000, 0x100),
			STM_PLAT_RESOURCE_IRQ(80, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(0),
	},
	[1] = {
		.name = "stm-gpio",
		.id = 1,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18021000, 0x100),
			STM_PLAT_RESOURCE_IRQ(84, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(1),
	},
	[2] = {
		.name = "stm-gpio",
		.id = 2,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18022000, 0x100),
			STM_PLAT_RESOURCE_IRQ(88, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(2),
	},
	[3] = {
		.name = "stm-gpio",
		.id = 3,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18023000, 0x100),
			STM_PLAT_RESOURCE_IRQ(115, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(3),
	},
	[4] = {
		.name = "stm-gpio",
		.id = 4,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18024000, 0x100),
			STM_PLAT_RESOURCE_IRQ(114, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(4),
	},
	[5] = {
		.name = "stm-gpio",
		.id = 5,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0x18025000, 0x100),
			STM_PLAT_RESOURCE_IRQ(113, -1),
		},
		.dev.platform_data = &STM_PLAT_PIO_DATA_LABELS_ONLY(5),
	},
};

static void __init stx7100_pio_late_setup(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stx7100_pio_devices); i++)
		platform_device_register(&stx7100_pio_devices[i]);
}



/* sysconf resources ------------------------------------------------------ */

static struct platform_device stx7100_sysconf_device = {
	.name		= "stm-sysconf",
	.id		= -1,
	.num_resources	= 1,
	.resource	= (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0x19001000, 0x194),
	},
	.dev.platform_data = &(struct stm_plat_sysconf_data) {
		.groups_num = 3,
		.groups = (struct stm_plat_sysconf_group []) {
			PLAT_SYSCONF_GROUP(SYS_DEV, 0x000),
			PLAT_SYSCONF_GROUP(SYS_STA, 0x008),
			PLAT_SYSCONF_GROUP(SYS_CFG, 0x100),
		},
	},
};



/* Early initialisation-----------------------------------------------------*/

/* Initialise devices which are required early in the boot process. */
void __init stx7100_early_device_init(void)
{
	struct sysconf_field *sc;
	unsigned long devid;
	unsigned long chip_7109, chip_revision;

	/* Create a PMB mapping so that the ioremap calls these drivers
	 * will make can be satisfied without having to call get_vm_area
	 * or cause a fault. Its probably also a good for efficiency as
	 * there will be lots of devices in this range.
	 */
	ioremap_nocache(0x18000000, 0x04000000);

	/* Initialise PIO and sysconf drivers */

	sysconf_early_init(&stx7100_sysconf_device, 1);
	stm_gpio_early_init(stx7100_pio_devices,
			ARRAY_SIZE(stx7100_pio_devices), 176);

	sc = sysconf_claim(SYS_DEV, 0, 0, 31, "devid");
	devid = sysconf_read(sc);
	chip_7109 = (((devid >> 12) & 0x3ff) == 0x02c);
	chip_revision = (devid >> 28) + 1;
	boot_cpu_data.cut_major = chip_revision;

	printk(KERN_INFO "%s version %ld.x\n",
	       chip_7109 ? "STx7109" : "STx7100", chip_revision);

	if (chip_7109) {
		boot_cpu_data.type = CPU_STX7109;
		sc = sysconf_claim(SYS_STA, 9, 0, 7, "devid");
		devid = sysconf_read(sc);
		printk(KERN_INFO "Chip version %ld.%ld\n",
				(devid >> 4)+1, devid & 0xf);
		boot_cpu_data.cut_minor = devid & 0xf;
	}

	/* Configure the ST40 RTC to source its clock from clockgenB.
	 * In theory this should be board specific, but so far nobody
	 * has ever done this. */
	sc = sysconf_claim(SYS_CFG, 8, 1, 1, "rtc");
	sysconf_write(sc, 1);

	/* We haven't configured the LPC, so the sleep instruction may
	 * do bad things. Thus we disable it here. */
	disable_hlt();
}



/* Pre-arch initialisation ------------------------------------------------ */

static struct platform_device emi = {
	.name = "emi",
	.id = -1,
	.num_resources = 2,
	.resource = (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0, 64*1024*1024),
		STM_PLAT_RESOURCE_MEM(0x1a100000, 0x874),
	},
};

static int __init stx7100_postcore_setup(void)
{
	return platform_device_register(&emi);
}
postcore_initcall(stx7100_postcore_setup);



/* Late initialisation ---------------------------------------------------- */

static struct platform_device *stx7100_devices[] __initdata = {
	&stx7100_fdma_device,
	&stx7100_sysconf_device,
	&stx7100_rng_hwrandom_device,
	&stx7100_rng_devrandom_device,
};

static int __init stx7100_devices_setup(void)
{
	stx7100_fdma_setup();
	stx7100_pio_late_setup();

	return platform_add_devices(stx7100_devices,
			ARRAY_SIZE(stx7100_devices));
}
device_initcall(stx7100_devices_setup);

