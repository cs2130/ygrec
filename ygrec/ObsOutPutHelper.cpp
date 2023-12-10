#include "../stdafx.h"
#include "ObsOutPutHelper.h"

#define AUTO_SPLIT_SIZE 4 * 1024

CObsOutPutHelper::CObsOutPutHelper()
{
	this->InitData();
}

CObsOutPutHelper::~CObsOutPutHelper()
{

}

bool CObsOutPutHelper::CreateOutPut()
{
	bool isRet = true;
	this->InitData();
	this->m_pFileOutput = obs_output_create(
		"ffmpeg_muxer", "simple_file_output", nullptr, nullptr);
	if (!m_pFileOutput) {
		return isRet;
	}
	return isRet;
}

bool CObsOutPutHelper::DisplayOutPut()
{
	this->StopOutPut(true);
	if (this->m_pFileOutput) {
		obs_output_release(this->m_pFileOutput);
	}
	this->m_pFileOutput = nullptr;
	return true;
}

bool CObsOutPutHelper::UpdataOutPutConfigure(
	_string strOutPath, _string strSuffix, ObsRecSqlitSet recSqlitSet)
{
	string strMux = "";
	string strFile = "";
	_string strPath = _T("");
	_string strFileW = _T("");

	strFileW = CUtilsFile::BuildFilePath(strOutPath, strSuffix);
	this->m_strOutFile = strFileW;

	strFile = CUtilsConv::WCharToUtf8(strFileW.c_str());
	OBSDataAutoRelease settings = obs_data_create();
	
	obs_data_set_string(settings, "format", "%CCYY-%MM-%DD %hh-%mm-%ss");

	obs_data_set_string(settings, "path", strFile.c_str());
	obs_data_set_string(settings, "muxer_settings", strMux.c_str());
	obs_output_update(m_pFileOutput, settings);
	return true;
}

obs_output_t* CObsOutPutHelper::GetOutPutPtr()
{
	return this->m_pFileOutput;
}

bool CObsOutPutHelper::StartOutPut()
{
	//if (!obs_output_valid(this->m_pFileOutput, "obs_output_start")) {
	//	return false;
	//}
	//if (!this->m_pFileOutput->context.data) {
	//	return false;
	//}

	return obs_output_start(this->m_pFileOutput);
}

bool CObsOutPutHelper::PauseOutPut()
{
	bool isRet = false;
	isRet = obs_output_can_pause(this->m_pFileOutput);
	if (isRet) {
		isRet = obs_output_pause(this->m_pFileOutput, true);
	}
	return isRet;
}

bool CObsOutPutHelper::RestoreOutPut()
{
	bool isRet = false;
	isRet = obs_output_paused(this->m_pFileOutput);
	if (isRet) {
		isRet = obs_output_pause(this->m_pFileOutput, false);
	}
	return isRet;
}

_string CObsOutPutHelper::GetOutFile()
{
	return this->m_strOutFile;
}

void CObsOutPutHelper::SetOutFile(_string strOutFile)
{
	this->m_strOutFile = strOutFile;
}

bool CObsOutPutHelper::StopOutPut(bool isForce)
{
	if (!this->m_pFileOutput) {
		return false;
	}

	if (obs_output_active(this->m_pFileOutput)) {
		if (isForce) {
			obs_output_force_stop(this->m_pFileOutput);
		}
		else {
			obs_output_stop(this->m_pFileOutput);
		}
	}
	//obs_output_release(this->m_pFileOutput);
	//this->m_pFileOutput = nullptr;
	return true;
}

void CObsOutPutHelper::InitData()
{
	this->m_pFileOutput = nullptr;
	this->m_strOutFile = _T("");
}