#ifndef __OBS_SCENEHELPER_H__
#define __OBS_SCENEHELPER_H__
#pragma once

class CObsSceneHelper
{
public:
	CObsSceneHelper();
	~CObsSceneHelper();

public:
	bool CreateScene(string strScene);
	obs_scene_t* GetScenePtr();
	obs_source_t* GetSourcePtr();
	bool ClearSource(string strSourceName);
	bool ClearSource();
	bool ClearScene();
	bool StopScene();
	bool DisplayScene();

private:
	void InitData();
	
private:
	obs_scene_t* m_pObsScene;
};

#endif