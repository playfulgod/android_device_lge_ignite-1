/* ALSAControl.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "ALSAControl"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

namespace android
{

ALSAControl::ALSAControl(const char *device)
{
    snd_ctl_open(&mHandle, device, 0);
}

ALSAControl::~ALSAControl()
{
    if (mHandle) snd_ctl_close(mHandle);
}

#ifdef AUDIO_MODEM_TI
status_t ALSAControl::getmin(const char *name, unsigned int &min)
{
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    min = snd_ctl_elem_info_get_min(info);

    return NO_ERROR;
}

status_t ALSAControl::getmax(const char *name, unsigned int &max)
{
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    max = snd_ctl_elem_info_get_max(info);

    return NO_ERROR;
}
#endif // AUDIO_MODEM_TI




 //[LG_FW_AUDIO_TTY Start] - jungsoo1221.lee
const char *control_elemens_of_ttymode[]={
  "TTY_OFF",
  "TTY_VCO",
  "TTY_HCO",
  "TTY_FULL"
};
 //[LG_FW_AUDIO_TTY End]


/* TODO:
 * replace the hard-coded _enumerated/_boolean/_integer by
 * something better, also remove the hard-coded values_of_control
 */

 //[LG_FW_P970_MERGE Start] 20110719 jungsoo1221.lee - Headset L/R Volume Balance (For Dolby)
  const char *control_elements_of_interest[] = {
	  "Analog Capture Volume",
	  /* change for donut branch -
	   * kernel 2.6.29 has changed the mixer names
	   */
	  "Analog Left AUXL Capture Switch", //20101026 inbang.park@lge.com FM_RADIO_PORTING 
	  "Analog Right AUXR Capture Switch",//20101026 inbang.park@lge.com FM_RADIO_PORTING
	  "Left2 Analog Loopback Switch",
	  "Right2 Analog Loopback Switch",
	  "DAC2 AnalogL Playback Volume", // 20100426 junyeop.kim@lge.com FM radio audio Path [START_LGE]
	  "DAC2 AnalogR Playback Volume"  // 20100426 junyeop.kim@lge.com FM radio audio Path [END_LGE]
  };
 
  static int g_t2_default_dac2_analogl, g_t2_default_dac2_analogr;	  /* LGE_CHANGE_S, [junyeop.kim@lge.com] 2010-04-24, Left/Right volume balance */
 
  /* LGE_CHANGE_S, [junyeop.kim@lge.com] 2010-04-24, Left/Right volume balance */
  int configure_T2_DAC2_AnalogL_Playback_Volume(snd_ctl_t *ctl,char on_off_status)
  {
	   snd_ctl_elem_value_t *value;
  
	   snd_ctl_elem_value_alloca(&value);
	   snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
	   snd_ctl_elem_value_set_name(value, control_elements_of_interest[5]);
  
	  if (on_off_status) {
		  g_t2_default_dac2_analogl = snd_ctl_elem_value_get_enumerated(value,0);
		  snd_ctl_elem_value_set_enumerated(value,0, on_off_status);
	  } else {
		  snd_ctl_elem_value_set_enumerated(value,0, g_t2_default_dac2_analogl);
	  }
  
	  return(snd_ctl_elem_write(ctl, value));
  }
  
  int configure_T2_DAC2_AnalogR_Playback_Volume(snd_ctl_t *ctl,char on_off_status)
  {
	   snd_ctl_elem_value_t *value;
  
	   snd_ctl_elem_value_alloca(&value);
	   snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
	   snd_ctl_elem_value_set_name(value, control_elements_of_interest[6]);
  
	  if (on_off_status) {
		  g_t2_default_dac2_analogr = snd_ctl_elem_value_get_enumerated(value,0);
		  snd_ctl_elem_value_set_enumerated(value,0, on_off_status);
	  } else {
		  snd_ctl_elem_value_set_enumerated(value,0, g_t2_default_dac2_analogr);
	  }
  
	  return(snd_ctl_elem_write(ctl, value));
  }
  //[LG_FW_P970_MERGE End] 20110719 jungsoo1221.lee - Headset L/R Volume Balance (For Dolby)

status_t ALSAControl::setMasterVolume(int value)	// 20100515 junyeop.kim@lge.com set the master volume gain for HW request [START_LGE]
{
	LOGD("[LUCKYJUN77] setMasterVolume");
//[LG_FW_P970_MERGE Start] 20110719 jungsoo1221.lee - Headset L/R Volume Balance (For Dolby)
	configure_T2_DAC2_AnalogL_Playback_Volume(mHandle, value);
	configure_T2_DAC2_AnalogR_Playback_Volume(mHandle, value);
//[LG_FW_P970_MERGE End] 20110719 jungsoo1221.lee - Headset L/R Volume Balance (For Dolby)
	set("DAC Voice Digital Downlink Volume", 0x00, 0);	//default vol (0x14 reg)

	return NO_ERROR;
}	// 20100515 junyeop.kim@lge.com set the master volume gain for HW request [START_LGE]




#if 1 /* [2011.02.21]jung.chanmin@lge.com - add the voice volume gain */
//2011.03.21 [jungsoo1221.lee] - p970 merge
int cur_voice_val = 0x08;	// current voice level
int cur_voice_dev = 0;		// 0 : receiver, 1 : spk, 2 : headset

status_t ALSAControl::setVoiceVolume(int value)	
{

    int voice_vol = 0x22;  //default
	
	if(value == 0xff)	// for reduce noise
	{
		LOGV("ALSAControl::delete and setVoiceVolume : %d cur_voice_dev = %d", cur_voice_val,cur_voice_dev);
		value = cur_voice_val;
	}
	else
	{
		LOGV("ALSAControl::setVoiceVolume : %d cur_voice_dev=%d", value,cur_voice_dev);
		cur_voice_val = value;
	}

	if(cur_voice_dev == 0)	//0: receiver 
	{
	
	    if(value == 6)	//-2dB(10)
		    voice_vol = 0x28;
		else if(value == 5)	//-5dB(9)
		    voice_vol = 0x25;
		else if(value == 4)	//-8dB(8)
		    voice_vol = 0x22;
		else if(value == 3) 	//-11dB(7)
		    voice_vol = 0x1f;
		else if(value == 2)	//-14dB(6)
		    voice_vol = 0x1c;
		else if(value == 1)	//-18dB(5)
		    voice_vol = 0x19;
		else if(value == 0)	//-22dB(4)
		    voice_vol = 0x16;
		else	//current vol restore
		{
		    LOGD("[LUCKYJUN77] Invalid volume");
	    }	
	}
	else if(cur_voice_dev == 1) //1: SPK-call
	{
		if(value == 6)	//-2dB(10)
			voice_vol = 0x26 /*0x24*/;
		else if(value == 5) //-5dB(9)
			voice_vol = 0x22;
		else if(value == 4) //-8dB(8)
			voice_vol = 0x1f;
		else if(value == 3) 	//-11dB(7)
			voice_vol = 0x1b;
		else if(value == 2) //-14dB(6)
			voice_vol = 0x17;
		else if(value == 1) //-18dB(5)
			voice_vol = 0x13;
		else if(value == 0) //-22dB(4)
			voice_vol = 0x0f;
		else	//current vol restore
		{
			LOGD("[LUCKYJUN77] Invalid volume");
		}	
	}
	else if(cur_voice_dev == 2)	// 2: headset or headphone call
	{
	    if(value == 6)	//-3dB(10)
		    voice_vol = 0x25;
		else if(value == 5)	//-6dB(9)
		    voice_vol = 0x22;
		else if(value == 4)	//-8dB(8)
		    voice_vol = 0x1F;
		else if(value == 3) 	//-10dB(7)
		    voice_vol = 0x1C;
		else if(value == 2)	//-12dB(6)
		    voice_vol = 0x19;
		else if(value == 1)	//-15dB(5)
		    voice_vol = 0x16;
		else if(value == 0)	//-18dB(4)
		    voice_vol = 0x13;
		else	//current vol restore
		{
		    LOGD("[LUCKYJUN77] Invalid volume");
	    }	
	}
	else
	{
	    LOGD("[LUCKYJUN77] Invalid voice dev");	
	}

	set("DAC Voice Digital Downlink Volume", voice_vol, 0);

	return NO_ERROR;
}	
#endif /* [2011.02.21]jung.chanmin@lge.com - add the voice volume gain */
// 20101011 junyeop.kim@lge.com voice_cur_device & volume[START_LGE]
status_t ALSAControl::setVoiceCurrentDev(int cur_dev)	
{
	cur_voice_dev = cur_dev;

	return NO_ERROR;
}
// 20101011 junyeop.kim@lge.com voice_cur_device & volume [END_LGE]

//[LG_FW_AUDIO_TTY Start] - jungsoo1221.lee
int tty_mode = TTY_OFF;
status_t ALSAControl::setTtyMode(int mode)	
{
    	LOGD("[TTY MODE] setTtyMode : %d", mode);	
	tty_mode = mode;
	set("TTY Mode",control_elemens_of_ttymode[tty_mode]);
	return NO_ERROR;
}
int ALSAControl::getTtyMode()
{	
    	LOGD("[TTY MODE] getTtyMode : %d", tty_mode);	

	return tty_mode;
}
 //[LG_FW_AUDIO_TTY End]  

status_t ALSAControl::get(const char *name, unsigned int &value, int index)
{
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *control;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int count = snd_ctl_elem_info_get_count(info);
    if (index >= count) {
        LOGE("Control '%s' index is out of range (%d >= %d)", name, index, count);
        return BAD_VALUE;
    }

    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);

    ret = snd_ctl_elem_read(mHandle, control);
    if (ret < 0) {
        LOGE("Control '%s' cannot read element value: %d", name, ret);
        return BAD_VALUE;
    }

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);
    switch (type) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            value = snd_ctl_elem_value_get_boolean(control, index);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER:
            value = snd_ctl_elem_value_get_integer(control, index);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER64:
            value = snd_ctl_elem_value_get_integer64(control, index);
            break;
        case SND_CTL_ELEM_TYPE_ENUMERATED:
            value = snd_ctl_elem_value_get_enumerated(control, index);
            break;
        case SND_CTL_ELEM_TYPE_BYTES:
            value = snd_ctl_elem_value_get_byte(control, index);
            break;
        default:
            return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ALSAControl::set(const char *name, unsigned int value, int index)
{
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int count = snd_ctl_elem_info_get_count(info);
    if (index >= count) {
        LOGE("Control '%s' index is out of range (%d >= %d)", name, index, count);
        return BAD_VALUE;
    }

    if (index == -1)
        index = 0; // Range over all of them
    else
        count = index + 1; // Just do the one specified

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);

    snd_ctl_elem_value_t *control;
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);

    for (int i = index; i < count; i++)
        switch (type) {
            case SND_CTL_ELEM_TYPE_BOOLEAN:
                snd_ctl_elem_value_set_boolean(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_INTEGER:
                snd_ctl_elem_value_set_integer(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_INTEGER64:
                snd_ctl_elem_value_set_integer64(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_ENUMERATED:
                snd_ctl_elem_value_set_enumerated(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_BYTES:
                snd_ctl_elem_value_set_byte(control, i, value);
                break;
            default:
                break;
        }

    ret = snd_ctl_elem_write(mHandle, control);
    return (ret < 0) ? BAD_VALUE : NO_ERROR;
}

status_t ALSAControl::set(const char *name, const char *value)
{
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int items = snd_ctl_elem_info_get_items(info);
    for (int i = 0; i < items; i++) {
        snd_ctl_elem_info_set_item(info, i);
        ret = snd_ctl_elem_info(mHandle, info);
        if (ret < 0) continue;
        if (strcmp(value, snd_ctl_elem_info_get_item_name(info)) == 0)
            return set(name, i, -1);
    }

    LOGE("Control '%s' has no enumerated value of '%s'", name, value);

    return BAD_VALUE;
}

};        // namespace android
