/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration for Amlogic Meson 64bits SoCs
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 */

#ifndef __MESON64_CONFIG_H
#define __MESON64_CONFIG_H

/* Generic Interrupt Controller Definitions */
#if (defined(CONFIG_MESON_AXG) || defined(CONFIG_MESON_G12A))
#define GICD_BASE			0xffc01000
#define GICC_BASE			0xffc02000
#else /* MESON GXL and GXBB */
#define GICD_BASE			0xc4301000
#define GICC_BASE			0xc4302000
#endif

/* For splashscreen */
#ifdef CONFIG_DM_VIDEO
#define STDOUT_CFG "serial"
#else
#define STDOUT_CFG "serial"
#endif

#ifdef CONFIG_USB_KEYBOARD
#define STDIN_CFG "usbkbd,serial"
#else
#define STDIN_CFG "serial"
#endif

#define CONFIG_CPU_ARMV8
#define CONFIG_REMAKE_ELF
#define CONFIG_SYS_MAXARGS		32
#define CONFIG_SYS_CBSIZE		1024

#define CONFIG_SYS_SDRAM_BASE		0
#define CONFIG_SYS_INIT_SP_ADDR		0x20000000
#define CONFIG_SYS_BOOTM_LEN		(64 << 20) /* 64 MiB */

#if defined(CONFIG_CMD_AVB)
#define AVB_VERIFY_CHECK \
	"if test \"${force_avb}\" -eq 1; then " \
		"if run avb_verify; then " \
			"echo AVB verification OK.;" \
			"setenv bootargs \"$bootargs $avb_bootargs\";" \
		"else " \
			"echo AVB verification failed.;" \
		"exit; fi;" \
	"else " \
		"setenv bootargs \"$bootargs androidboot.verifiedbootstate=orange\";" \
		"echo Running without AVB...; "\
	"fi;"

#define AVB_VERIFY_CMD "avb_verify=avb init ${mmcdev}; avb verify $slot_suffix;\0"
#else
#define AVB_VERIFY_CHECK ""
#define AVB_VERIFY_CMD ""
#endif

#if defined(CONFIG_CMD_AB_SELECT)
#define ANDROIDBOOT_GET_CURRENT_SLOT_CMD "get_current_slot=" \
	"if part number mmc ${mmcdev} " CONTROL_PARTITION " control_part_number; " \
	"then " \
		"echo " CONTROL_PARTITION \
			" partition number:${control_part_number};" \
		"ab_select current_slot mmc ${mmcdev}:${control_part_number};" \
	"else " \
		"echo " CONTROL_PARTITION " partition not found;" \
	"fi;\0"

#define AB_SELECT_SLOT \
	"run get_current_slot; " \
	"if test -e \"${current_slot}\"; " \
	"then " \
		"setenv slot_suffix _${current_slot}; " \
	"else " \
		"echo current_slot not found;" \
		"exit;" \
	"fi;"

#define AB_SELECT_ARGS \
	"setenv bootargs_ab androidboot.slot_suffix=${slot_suffix}; " \
	"echo A/B cmdline addition: ${bootargs_ab};" \
	"setenv bootargs ${bootargs} ${bootargs_ab};"

#define AB_BOOTARGS " androidboot.force_normal_boot=1"
#define RECOVERY_PARTITION "boot"
#else
#define AB_SELECT_SLOT ""
#define AB_SELECT_ARGS " "
#define ANDROIDBOOT_GET_CURRENT_SLOT_CMD ""
#define AB_BOOTARGS " "
#define RECOVERY_PARTITION "recovery"
#endif

/*
 * Prepares complete device tree blob for current board (for Android boot).
 *
 * Boot image or recovery image should be loaded into $loadaddr prior to running
 * these commands. The logic of these commnads is next:
 *
 *   1. Read correct DTB for current SoC/board from boot image in $loadaddr
 *      to $fdtaddr
 *   2. Merge all needed DTBO for current board from 'dtbo' partition into read
 *      DTB
 *   3. User should provide $fdtaddr as 3rd argument to 'bootm'
 */
#define PREPARE_FDT \
	"echo Preparing FDT...; " \
	"setenv dtb_index 0;" \
	"abootimg get dtb --index=$dtb_index dtb_start dtb_size; " \
	"cp.b $dtb_start $fdt_addr_r $dtb_size; " \
	"fdt addr $fdt_addr_r 0x80000; " \
	"setenv dtbo_index 0;" \
	"part start mmc ${mmcdev} dtbo${slot_suffix} p_dtbo_start; " \
	"part size mmc ${mmcdev} dtbo${slot_suffix} p_dtbo_size; " \
	"mmc read ${dtboaddr} ${p_dtbo_start} ${p_dtbo_size}; " \
	"echo \"  Applying DTBOs...\"; " \
	"adtimg addr $dtboaddr; " \
	"adtimg get dt --index=$dtbo_index dtbo0_addr; " \
	"fdt apply $dtbo0_addr;" \
	"setenv bootargs \"$bootargs androidboot.dtbo_idx=$dtbo_index \";"\

#define BOOT_CMD "bootm ${loadaddr} ${loadaddr} ${fdt_addr_r};"

/* fastboot: the tool is started when:
 *  1. the board was booted over USB
 *  2. if the misc partition says so (Android)
 */
#define BOOTENV_DEV_FASTBOOT(devtypeu, devtypel, instance) \
	"bootcmd_fastboot=" \
		"setenv run_fastboot 0;" \
		"if test \"${boot_source}\" = \"usb\"; then " \
			"echo Fastboot forced by usb rom boot;" \
			"setenv run_fastboot 1;" \
		"fi;" \
		"if test \"${run_fastboot}\" -eq 0; then " \
			"if bcb load ${mmcdev} misc; then " \
				"if bcb test command = bootonce-bootloader; then " \
					"echo BCB: Bootloader boot...; " \
					"bcb clear command; bcb store; " \
					"setenv run_fastboot 1;" \
				"fi; " \
			"else " \
				"echo Warning: BCB is corrupted or does not exist; " \
			"fi;" \
		"fi;" \
		"if test \"${run_fastboot}\" -eq 1; then " \
			"echo Running Fastboot...;" \
			"fastboot " __stringify(CONFIG_FASTBOOT_USB_DEV) "; " \
		"fi\0"

#define BOOTENV_DEV_NAME_FASTBOOT(devtypeu, devtypel, instance)	\
		"fastboot "

/* recovery: goes into recovery console when:
 *  1. the misc partition contains 'boot-recovery' (Android)
 *  2. the misc partition contains 'boot-fastboot' (Android)
 */
#define BOOTENV_DEV_RECOVERY(devtypeu, devtypel, instance) \
	"bootcmd_recovery=" \
		"setenv run_recovery 0;" \
		"if bcb load ${mmcdev} misc; then " \
			"if bcb test command = boot-recovery; then " \
				"echo BCB: Recovery boot...; " \
				"setenv run_recovery 1;" \
			"elif bcb test command = boot-fastboot; then " \
				"echo BCB: fastbootd boot...; " \
				"setenv run_recovery 1;" \
			"fi;" \
		"else " \
			"echo Warning: BCB is corrupted or does not exist; " \
		"fi;" \
		"if test \"${run_recovery}\" -eq 1; then " \
			"echo Running Recovery...;" \
			"mmc dev ${mmcdev};" \
			"setenv bootargs \"${bootargs} androidboot.serialno=${serial#}\";" \
			AB_SELECT_SLOT \
			AB_SELECT_ARGS \
			AVB_VERIFY_CHECK \
			"part start mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_start;" \
			"part size mmc ${mmcdev} " RECOVERY_PARTITION "${slot_suffix} boot_size;" \
			"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
				PREPARE_FDT \
				"echo Running Android Recovery...;" \
				BOOT_CMD \
			"fi;" \
			"echo Failed to boot Android...;" \
			"reset;" \
		"fi\0"

#define BOOTENV_DEV_NAME_RECOVERY(devtypeu, devtypel, instance)	\
		"recovery "

/* system: boots the Android OS:
 *  1. the misc partition contains 'boot-recovery' (Android)
 *  2. the misc partition contains 'boot-fastboot' (Android)
 */
#define BOOTENV_DEV_SYSTEM(devtypeu, devtypel, instance) \
	"bootcmd_system=" \
		"echo Loading Android boot partition...;" \
		"mmc dev ${mmcdev};" \
		"setenv bootargs ${bootargs} androidboot.serialno=${serial#};" \
		AB_SELECT_SLOT \
		AB_SELECT_ARGS \
		AVB_VERIFY_CHECK \
		"part start mmc ${mmcdev} boot${slot_suffix} boot_start;" \
		"part size mmc ${mmcdev} boot${slot_suffix} boot_size;" \
		"if mmc read ${loadaddr} ${boot_start} ${boot_size}; then " \
			PREPARE_FDT \
			"setenv bootargs \"${bootargs} " AB_BOOTARGS "\"  ; " \
			"echo Running Android...;" \
			BOOT_CMD \
		"fi;" \
		"echo Failed to boot Android...;\0"

#define BOOTENV_DEV_NAME_SYSTEM(devtypeu, devtypel, instance)	\
		"system "

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#if CONFIG_IS_ENABLED(CMD_PXE)
# define BOOT_TARGET_PXE(func) func(PXE, pxe, na)
#else
# define BOOT_TARGET_PXE(func)
#endif

#ifdef CONFIG_CMD_NVME
	#define BOOT_TARGET_NVME(func) func(NVME, nvme, 0)
#else
	#define BOOT_TARGET_NVME(func)
#endif

#ifdef CONFIG_CMD_SCSI
	#define BOOT_TARGET_SCSI(func) func(SCSI, scsi, 0)
#else
	#define BOOT_TARGET_SCSI(func)
#endif

#if CONFIG_IS_ENABLED(CMD_MMC)
	#define BOOT_TARGET_MMC(func) func(MMC, mmc, 0) func(MMC, mmc, 1)
#else
	#define BOOT_TARGET_MMC(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	func(FASTBOOT, fastboot, na) \
	func(RECOVERY, recovery, na) \
	func(SYSTEM, system, na) \
	BOOT_TARGET_MMC(func) \
	BOOT_TARGET_DEVICES_USB(func) \

#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE 8192000
#define CONFIG_VIDEO_BMP_GZIP 1

#define CONSOLE_FONT_COLOR 14

#define CONFIG_HOSTNAME CONFIG_DEFAULT_DEVICE_TREE
#define CONFIG_BOOTP_SEND_HOSTNAME 1

#include <config_distro_bootcmd.h>

#define PARTS_DEFAULT \
	"name=logo,start=512K,size=2M;" \
	"name=misc,size=512K;" \
	"name=dtbo,size=8M;" \
	"name=vbmeta,size=512K;" \
	"name=boot,size=32M,bootable;" \
	"name=recovery,size=32M;" \
	"name=cache,size=1280M;" \
	"name=super,size=2304M;" \
	"name=metadata,size=8M;" \
	"name=userdata,size=-;" \

#ifndef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"console=ttyAML0\0" \
	"dtboaddr=0x08200000\0"                                       \
	"fastboot_raw_partition_bootloader=0x1 0xfff mmcpart 1\0" \
	"fastboot_raw_partition_bootloader-env=0x1000 0x10 mmcpart 1\0" \
	"fdt_addr_r=0x01000000\0" \
	"fdtfile=meson-g12b-a311d-bd-som.dtb\0" \
	"fdtoverlay_addr_r=0x01000000\0" \
	"kernel_addr_r=0x01080000\0" \
	"kernel_comp_addr_r=0x0d080000\0" \
	"kernel_comp_size=0x2000000\0" \
	"loadaddr=0x01080000\0" \
	"mmcdev=" __stringify(CONFIG_FASTBOOT_FLASH_MMC_DEV) "\0"                                                  \
	"netargs=setenv bootargs console=${console},115200 root=/dev/nfs rw " \
		"ip=dhcp nfsroot=${tftpserverip}:${nfsroot},v3,tcp\0" \
	"netboot=run netargs; " \
		"if test -z \"${fdtfile}\" -a -n \"${soc}\"; then " \
			"setenv fdtfile ${soc}-${board}${boardver}.dtb; " \
		"fi; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${loadaddr} ${tftpserverip}:Image; " \
		"if ${get_cmd} ${fdt_addr_r} ${tftpserverip}:${fdtfile}; then " \
			"booti ${loadaddr} - ${fdt_addr_r}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0" \
	"partitions=" PARTS_DEFAULT "\0" \
	"pxefile_addr_r=0x01080000\0" \
	"ramdisk_addr_r=0x13000000\0" \
	"scriptaddr=0x08000000\0" \
	"stderr=" STDOUT_CFG "\0" \
	"stdin=" STDIN_CFG "\0" \
	"stdout=" STDOUT_CFG "\0" \
	BOOTENV
#endif


#endif /* __MESON64_CONFIG_H */
