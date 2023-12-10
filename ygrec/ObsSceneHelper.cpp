#include "../stdafx.h"
#include "ObsSceneHelper.h"

#define OBS_CENE_NAME					""

CObsSceneHelper::CObsSceneHelper()
{
	this->InitData();
}

CObsSceneHelper::~CObsSceneHelper()
{

}

bool CObsSceneHelper::CreateScene(string strScene)
{
	if (this->m_pObsScene) { return true; }
	bool			isRet = false;
	const char*		szId = nullptr;
	size_t			ullIdx = 0;
	obs_source_t*	pFadeTransition = nullptr;
	obs_source_t*	pSource = nullptr;

	obs_set_output_source(SOURCE_CHANNEL_TRANSITION, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, nullptr);

	// 场景过度 - 淡出
	// 参见 window-basic-main-transitions.cpp -> OBSBasic::InitDefaultTransitions
	while (obs_enum_transition_types(ullIdx++, &szId)) {
		if (!obs_is_source_configurable(szId)) {
			if (strcmp(szId, "fade_transition") == 0) {
				const char* szName = obs_source_get_display_name(szId);
				obs_source_t* tr = obs_source_create_private(szId, szName, NULL);
				pFadeTransition = tr;
				break;
			}
		}
	}
	if (!pFadeTransition) {
		blog(LOG_ERROR, "cannot find fade transition. %d", __LINE__);
		return isRet;
	}
	obs_set_output_source(SOURCE_CHANNEL_TRANSITION, pFadeTransition);
	obs_source_release(pFadeTransition);
	//pFadeTransition = nullptr;

	string strSceneName = "";
	strSceneName = fmt::format("{0}{1}", OBS_CENE_NAME, strScene);
	this->m_pObsScene = obs_scene_create(strSceneName.c_str());
	if (!this->m_pObsScene) {
		blog(LOG_ERROR, "create scene failed. %d", __LINE__);
		return isRet;
	}
	pSource = obs_get_output_source(SOURCE_CHANNEL_TRANSITION);
	obs_transition_set(pSource, obs_scene_get_source(this->m_pObsScene));
	obs_source_release(pSource); 
	isRet = true;
	return isRet;
}

obs_scene_t* CObsSceneHelper::GetScenePtr()
{
	return this->m_pObsScene;
}

obs_source_t* CObsSceneHelper::GetSourcePtr()
{
	return reinterpret_cast<obs_source_t*>(this->m_pObsScene);
}

bool CObsSceneHelper::StopScene()
{
	obs_set_output_source(SOURCE_CHANNEL_TRANSITION, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, nullptr);

	auto cb = [](void* unused, obs_source_t* source)
	{ 
		const char* name = obs_source_get_name(source);
		//OBSSourceAutoRelease source =
		//	obs_get_source_by_name(sourceName.c_str());
		//obs_source_remove(source);
		if (source && name) {
			obs_source_remove(source);
		}
		return true;
	};
	obs_enum_scenes(cb, nullptr);
	
	//这样释放后 再次启动 obs_output_start 返回 false
	//并且无法获取到错误信息 
	//obs_enum_sources(cb, nullptr);
	return true;
}

bool CObsSceneHelper::ClearSource(string strSourceName)
{
	OBSSourceAutoRelease source =
		obs_get_source_by_name(strSourceName.c_str());
	if (source) {
		obs_source_remove(source);
	}
	return true;
}

bool CObsSceneHelper::ClearSource()
{
	if (!this->m_pObsScene) { return false; }
	auto cb = [](void* unused, obs_source_t* source) {
		const char* name = obs_source_get_name(source);
		if (source && name) {
			obs_source_remove(source);
		}
		return true;
	};
	obs_enum_scenes(cb, nullptr);
}

bool CObsSceneHelper::ClearScene()
{
	if (!this->m_pObsScene) {
		return false;
	}
	this->StopScene();
}

bool CObsSceneHelper::DisplayScene()
{
	if (this->m_pObsScene) {
		this->StopScene();
		obs_scene_release(this->m_pObsScene);
	}

	this->m_pObsScene = nullptr;
	return true;
}

void CObsSceneHelper::InitData()
{
	this->m_pObsScene = nullptr;
}
