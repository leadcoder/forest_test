#pragma once
#include "ov_Common.h"
#include "ov_Shadow.h"
#include "ov_Fog.h"

namespace osgVegetation
{
	class SceneConfig
	{
	public:
		SceneConfig() : FogMode(FM_DISABLED)
		{
		}
		ShadowSettings Shadow;
		FogModeEnum FogMode;

		void Apply(osg::StateSet* state_set)
		{
			std::string shadow_mode_str;
			switch (Shadow.Mode)
			{
			case SM_DISABLED:
			case SM_UNDEFINED:
				break;
			case SM_LISPSM:
				shadow_mode_str = "SM_LISPSM";
				break;
			case SM_VDSM1:
				shadow_mode_str = "SM_VDSM1";
				break;
			case SM_VDSM2:
				shadow_mode_str = "SM_VDSM2";
				break;
			}
			if (shadow_mode_str != "")
				state_set->setDefine(shadow_mode_str);

			std::string fog_mode_str;
			switch (FogMode)
			{
			case FM_LINEAR:
				fog_mode_str = "FM_LINEAR";
				break;
			case FM_EXP:
				fog_mode_str = "FM_EXP";
				break;
			case FM_EXP2:
				fog_mode_str = "FM_EXP2";
				break;
			}
			if (fog_mode_str != "")
				state_set->setDefine(fog_mode_str);
		}
	};
}