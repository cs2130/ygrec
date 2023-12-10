#include "../stdafx.h"
#include "ObsTextRecord.h"

#define VIDEO_TEXT_SOURCE_ID		"text_gdiplus"
#define VIDEO_TEXT_SOURCE_NAME		"Text"

CObsTextRecord::CObsTextRecord()
{

}

CObsTextRecord::~CObsTextRecord()
{

}

bool CObsTextRecord::InitText()
{
	this->InitSourceParam();
	this->SetSourceBaseName(VIDEO_TEXT_SOURCE_ID);
	return true;
}


uint32_t CObsTextRecord::RgbToBgr(uint32_t rgb)
{
	return ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb & 0xFF0000) >> 16);
}

bool CObsTextRecord::AddText(obs_scene_t* pScene, WaterTextInfo wtInfo)
{
	this->ReInitText();
	string strSourceName = this->BuildSourceName("Add");

	OBSDataAutoRelease font = obs_data_create();

	string strFontNameUtf8 = "";
	string strTextUtf8 = "";
	strFontNameUtf8 = CUtilsConv::UnicodeToUtf8(wtInfo.strFontName);
	strTextUtf8 = CUtilsConv::UnicodeToUtf8(wtInfo.strText);

	obs_data_set_string(font, "face", strFontNameUtf8.c_str());
	obs_data_set_int(font, "flags", 1);
	obs_data_set_int(font, "size", wtInfo.nFontSize);

	OBSSourceAutoRelease sourceAuto;
	sourceAuto = obs_source_create(VIDEO_TEXT_SOURCE_ID,
		strSourceName.c_str(), NULL, nullptr);
	obs_data_t* settings = obs_source_get_settings(sourceAuto);

	obs_data_set_string(settings, "text", strTextUtf8.c_str());

	obs_source_update(sourceAuto, settings);
	obs_data_release(settings);
	obs_scene_add(pScene, sourceAuto);

	return true;
}

void CObsTextRecord::MoveText(obs_scene_t* pScene, POINT pt)
{
	string strSourceName = this->GetSourceName("Add");
	obs_sceneitem_t* pItem = obs_scene_find_source(pScene, strSourceName.c_str());
	if (!pItem) { return; }
	struct vec2 pos = { pt.x, pt.y };
	obs_sceneitem_set_pos(pItem, &pos);
	//obs_source_t* source = obs_sceneitem_get_source(pItem);
	//obs_transform_info info;
	//obs_sceneitem_get_info(pItem, &info);
	//CUtilsLog::OutLog(fmt::format(L"MoveText, {0},{1},{2}", 
	//	obs_source_get_width(source), obs_source_get_width(source), __LINE__));
}

bool CObsTextRecord::ReInitText()
{
	this->ClearSourceName();
	return true;
}

void CObsTextRecord::DisplayText(obs_scene_t* pScene)
{
	if (pScene) {
		string strSourceName = this->GetSourceName("Add");
		OBSSourceAutoRelease source =
			obs_get_source_by_name(strSourceName.c_str());
		if (source) {
			obs_source_remove(source);
		}
	}
}