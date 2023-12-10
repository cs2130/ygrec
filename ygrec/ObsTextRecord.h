#ifndef __OBS_TEXTRECORD_H__
#define __OBS_TEXTRECORD_H__
#pragma once

class CObsTextRecord
	: public CObsBaseHelper
{
public:
	CObsTextRecord();
	~CObsTextRecord();

public:
	bool InitText();
	bool AddText(obs_scene_t* pScene, WaterTextInfo wtInfo);
	void MoveText(obs_scene_t* pScene, POINT pt);
	void DisplayText(obs_scene_t* pScene);

private:
	bool ReInitText();
	uint32_t RgbToBgr(uint32_t rgb);
};
#endif