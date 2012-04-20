# config.mk
#
# Product-specific compile-time definitions.

TARGET_BOARD_PLATFORM := omap3

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := false
TARGET_NO_RADIOIMAGE := true

BOARD_EGL_CFG := device/lge/ignite/configs/egl.cfg
BOARD_WIRELESS_CHIP := bcm4329
BOARD_SENSOR_CHIP := ak8973
TARGET_USE_COMMON_SENSOR := false
TARGET_USE_COMMON_SENSOR_DAEMON := false
USE_GESTURE_SENSOR := true
USE_IMMERSION_VIBRATOR := true
BOARD_CAMERA_CHIP := imx072

# Board configuration
#
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_PROVIDES_INIT_RC := true
OMAP_ENHANCEMENT := true
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_GLOBAL_CFLAGS += -D__ARM_NEON__ -D__ARM_ARCH_7A__ -mtune=cortex-a8
TARGET_GLOBAL_CPPFLAGS += -D__ARM_NEON__ -D__ARM_ARCH_7A__ -mtune=cortex-a8

# Kernel/Bootloader machine name
#
TARGET_BOOTLOADER_BOARD_NAME := bproj
BOARD_KERNEL_BASE := 0x80000000

# HW Graphcis
#OMAP3_GL := false

# Wifi
##USES_TI_WL1271 := true
BOARD_WPA_SUPPLICANT_DRIVER := CUSTOM
##BOARD_WPA_SUPPLICANT_DRIVER := WEXT
##ifdef USES_TI_WL1271
##BOARD_WLAN_DEVICE           := wl1271
##BOARD_SOFTAP_DEVICE         := wl1271
##endif
##BOARD_WLAN_TI_STA_DK_ROOT   := system/wlan/ti/wilink_6_1
WPA_SUPPLICANT_VERSION      := VER_0_6_X
#WIFI_DRIVER_MODULE_PATH     := "/system/etc/wifi/wireless.ko"
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/wireless.ko"
WIFI_DRIVER_MODULE_NAME     := "wireless"
WIFI_FIRMWARE_LOADER        := ""

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
#+++ BRCM
BT_ALT_STACK :=true
BRCM_BT_USE_BTL_IF :=true
BRCM_BTL_INCLUDE_A2DP :=true
BRCM_BTL_INCLUDE_OBEX :=true
BRCM_BTL_OBEX_USE_DBUS :=true
#--- BRCM


# FM
BUILD_FM_RADIO := false
BOARD_HAVE_FM_ROUTING := false
FM_CHR_DEV_ST := true

# MultiMedia defines
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
BUILD_WITH_ALSA_UTILS := true
BOARD_USES_TI_CAMERA_HAL := true
CAMERA_YUV_FAST_CONVERT := true
CAMERA_ALGO := true
HARDWARE_OMX := true
FW3A := true
ICAP := true
IMAGE_PROCESSING_PIPELINE := true
USE_OVERLAY_FORMAT_DEFAULT_ONLY := true
ifdef HARDWARE_OMX
OMX_JPEG := true
OMX_VENDOR := ti
OMX_VENDOR_INCLUDES := \
   hardware/ti/omx/system/src/openmax_il/omx_core/inc \
   hardware/ti/omx/image/src/openmax_il/jpeg_enc/inc
OMX_VENDOR_WRAPPER := TI_OMX_Wrapper
BOARD_OPENCORE_LIBRARIES := libOMX_Core
BOARD_OPENCORE_FLAGS := -DHARDWARE_OMX=1
BOARD_CAMERA_LIBRARIES := libcamera
endif

USE_CAMERA_STUB := false

ifdef OMAP_ENHANCEMENT
COMMON_GLOBAL_CFLAGS += -DOMAP_ENHANCEMENT -DTARGET_OMAP3
endif

# This define enables the compilation of OpenCore's command line TestApps
#BUILD_PV_TEST_APPS :=1

#############################################################
## BEGIN - Setting image sizes and etc.
##         These settings should be dealt with greatest care
##         and should conform to flash memory map
##TARGET_USERIMAGES_USE_EXT4 := true
##TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true
##BOARD_SYSTEMIMAGE_PARTITION_SIZE := 402653184  # = 786432 * 512 = 384Mbyte
##BOARD_USERDATAIMAGE_PARTITION_SIZE := 1024768 # fake size for formatting on first boot
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 1467351040 # = 2865920 * 512 = 1.4Gbyte
##BOARD_FLASH_BLOCK_SIZE := 4096
## END - setting image sizes and etc.
#############################################################

#############################################################
##
##  Quick Build for specific low-level targets
##
LGE_QUICK_BUILD_TARGETS := xloader cxloader uboot cuboot cimages ckernel cgfx
ifneq (,$(MAKECMDGOALS))
ifeq (,$(filter-out $(LGE_QUICK_BUILD_TARGETS),$(MAKECMDGOALS)))
BUILD_TINY_ANDROID := true
endif

ifeq (,$(filter-out clean,$(MAKECMDGOALS)))
clean: cxloader cuboot ckernel cgfx
include bootable/bootloader/x-loader/AndroidXLoader.mk
include bootable/bootloader/u-boot/AndroidUBoot.mk
include kernel/AndroidKernel.mk
endif

.PHONY: cgfx
cgfx:
#LGE_CHANGE_S [shaun.hong@lge.com] 2011-03-25 : Clean	
	cd vendor/ti/GFX_Linux_DDK && build_DDK.sh --build zoom3 release clobber
#LGE_CHANGE_E [shaun.hong@lge.com]
endif
##############################################################################
#  Broadcom GPS
##############################################################################

BOARD_GPS_LIBRARIES := gps.$(TARGET_BOARD_PLATFORM)

CONFIG_HAL_SERIAL_TYPE=UART
CONFIG_HAL_SERIAL_DEV=/dev/ttyO0
CONFIG_HAL_CMD=yes
CONFIG_HAL_CMD_FILE=/data/gps/glgpsctrl
CONFIG_HAL_LTO=yes
CONFIG_HAL_LTO_DIR=/data/gps/
CONFIG_HAL_LTO_FILE=lto.dat
CONFIG_HAL_NMEA_PIPE=yes
CONFIG_HAL_NMEA_FILE=/data/gps/gpspipe
CONFIG_HAL_NV=yes
CONFIG_HAL_NV_DIR=/data/gps/
CONFIG_HAL_NV_FILE=gldata.sto
CONFIG_HAL_RRC=no
CONFIG_HAL_GPIO_SYSFS=yes
CONFIG_HAL_GPIO_NRESET_INIT=yes

CONFIG_HAL_CATCH_SIGNALS=yes
CONFIG_HAL_EE_DIR=./gps/
CONFIG_HAL_EE_FILE=cbee.cbee
CONFIG_HAL_LCS_API=yes
CONFIG_HAL_TIME_MONOTONIC=yes
# Disabling $PGLOR,NET for current AT-command CP implementation.
#CONFIG_HAL_NET_REPORT=yes

CONFIG_HAL_SUPL=true
ENABLE_TLS=yes

TARGET_RELEASE_CFLAGS=-O2 -fno-strict-aliasing

