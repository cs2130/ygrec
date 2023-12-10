#ifndef __OBS_RECORDHELPER_H__
#define __OBS_RECORDHELPER_H__
#pragma once

class CObsRecordHelper
	: public CErrorBase
{
public:
	CObsRecordHelper();
	~CObsRecordHelper();

public:
	bool ObsInitial(CConfigHelper* pConfigHelper);
	void SetCallBack(
		RecEncoderCallBack& fEncoderCallBack,
		ScreenShotCallBack& fScreenShotCallBack);

public:
	bool ObsMuteApeaker(bool isMute);
	bool ObsMuteMic(bool isMute);
	bool ObsIsApeaker();
	bool ObsIsMic();

public:
	bool ObsStartPre(
		UserInfoRes* pVarUserInfo,
		OperSetApi* pOperSetApi,
		int nTaskHeight);
	bool ObsStart();
	bool ObsPause();
	bool ObsRestore();
	DWORD ObsRecTimeCount();
	DWORD ObsFileSize();
	void ObsStop(bool isForce = false);
	obs_source_t* ObsScreenShotSource();
	void ObsDisplay();
	void ObsDisplayEnd(HANDLE hProcess, HWND hWnd);

public:
	void	ObsBeginLogCatch();
	string	ObsPauseLogCatch();
	void	ObsStopLogCatch();

public:
	bool FindProximityFrame(
		std::vector<devmodes> vecMinModes, WORD wFrame, devmodes& desModes);
	std::tuple<int, int, int> SelCameraParam(
		RectX rcArea, WORD wFrameRate, vector<devmodes> vecDevModes);

public:
	void ScreenShotBack(_string wstrOutFile, _string wstrThumbnail, int nWidth, int nHeiht);

private:
	void ObsInitParam();
	void PushMediaUiList(_string strOutFile, int64_t dwTime, int nBitRate, bool isStoped);

public:
	LogOper BuildLogOper(string strStr, EnOperChar enOper);
	void PushLogOper();

private:
	bool CheckPlugins(const _string& strOwrDir);
	bool CheckModules(const _string& strOwrDir);

private:
	static void StopRecordingCallback(void* param, calldata_t* data);
	static void RecordFileChangedCallback(void* param, calldata_t* data);

private:
	RecEncoderCallBack		mRecordCallBack;
	ScreenShotCallBack		mScreenShotCallBack;

private:
	CLogHelper*				m_pLogHelper;

private:
	obs_source_t*			m_pObsRecSource;
	c_epool*				m_pLoopRecordFile;
	c_epool*				m_pScreenEvent;

private:
	CObsSceneHelper			m_ObsSceneNormalHelper;
	CObsSceneHelper			m_ObsSceneCameraHelper;
	CObsAudioRecord			m_ObsAudioRecord;
	CObsVideoRecord			m_ObsVideoRecord;
	CObsOutPutHelper		m_ObsOutPutHelper;
	CObsCaptureHelper		m_ObsCaptureHelper;
	CObsScreenHelper		m_ObsScreenHelper;
	CObsSceneRotate			m_ObsSceneRotate;

private:
	CObsTextRecord			m_ObsTextRecord;

private:
	HWND					m_hCameraPreWnd;
	CUtilsAtomic<bool>		m_isLogCatch;
	string					m_strLogCatch;
	bstring					m_pLogCatch;


private:
	bool					m_isOutLog;
};

#endif // !__RESHELPER_H__

