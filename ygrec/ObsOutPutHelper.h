#ifndef __OBS_OUTPUTHELPER_H__
#define __OBS_OUTPUTHELPER_H__
#pragma once

class CObsOutPutHelper
{
public:
	CObsOutPutHelper();
	~CObsOutPutHelper();

public:
	bool CreateOutPut();
	bool UpdataOutPutConfigure(
		_string strOutPath, _string strSuffix, ObsRecSqlitSet recSqlitSet);
	obs_output_t* GetOutPutPtr();
	bool StartOutPut();
	bool PauseOutPut();
	bool RestoreOutPut();
	_string GetOutFile();
	void SetOutFile(_string strOutFile);
	void OutPutFirstFrame();
	bool StopOutPut(bool isForce = false);
	bool DisplayOutPut();

private:
	void InitData();

private:
	obs_output_t*			m_pFileOutput;
	_string					m_strOutFile;
};

#endif // !__RESHELPER_H__

