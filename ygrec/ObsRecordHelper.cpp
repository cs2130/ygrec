#include "../stdafx.h"

#define W_WATER_TIMER 1685031747
//#define W_WATER_TIMER 1682389347

CObsRecordHelper::CObsRecordHelper()
{
	this->ObsInitParam();
}

CObsRecordHelper::~CObsRecordHelper()
{

}

void CObsRecordHelper::ObsInitParam()
{
	this->m_isOutLog = false;
	this->mRecordCallBack = nullptr;
	this->mScreenShotCallBack = nullptr;
	this->m_isLogCatch.InitAtomic(false);
	this->m_strLogCatch = "";
	this->m_hCameraPreWnd = NULL;
	this->m_pConfigHelper = nullptr;
	this->m_pLogHelper = nullptr;
	this->m_pWaterRes = nullptr;
	this->m_pMouseRes = nullptr;
	this->m_obsRecParam.Init();
	this->m_recTimeHelper.Init();
	this->m_imageThumbnail.Clear();
}

LogOper CObsRecordHelper::BuildLogOper(string strStr, EnOperChar enOper)
{
	LogOper logOper;
	logOper.strStr = strStr;
	logOper.enOper = enOper;
	return logOper;
}

void CObsRecordHelper::DoLog(int log_level, const char* msg, va_list args, void* param)
{
	bool isBreak = false;
#ifdef  ISDEBUG_ALPHA
	if (log_level > LOG_INFO) { return; }
#else
	if (log_level >= LOG_INFO) { return; }
#endif
	CObsRecordHelper* pThis = reinterpret_cast<CObsRecordHelper*>(param);
	if (!pThis->m_isOutLog) { return; }

	string strLog = string(pThis->m_pLogCatch.format(msg, args));
	for (LogOper logOper : pThis->m_vecLogOper)
	{
		if (strLog.find(logOper.strStr) != std::string::npos) {
			if (logOper.enOper == EOC_REP) {
				strLog = CUtilsStr::ReplaceStrA(strLog, logOper.strStr, "yg");
			} else {
				isBreak = true;
				break;
			}
		}
	}
	if (isBreak) { return; }
	if (pThis->m_isLogCatch.GetAtomic()) {
		strLog = CUtilsStr::ReplaceStrA(strLog, "\n", " ");
		pThis->m_strLogCatch.append(strLog);
	}
	pThis->m_pLogHelper->OutLogA(LT_OBS_REC, log_level, strLog, "", __LINE__);
}

void CObsRecordHelper::PushMediaUiList(_string strOutFile, int64_t dwTime, int nBitRate, bool isStoped)
{
}

void CObsRecordHelper::StopRecordingCallback(void* pParam, calldata_t* pData)
{
	// 停止录制时的回调函数
	CObsRecordHelper* pThis = reinterpret_cast<CObsRecordHelper*>(pParam);
	_string strOutFile = pThis->m_ObsOutPutHelper.GetOutFile();

	MediaDataRes* pDataRes = static_cast<MediaDataRes*>(smalloc(sizeof(MediaDataRes)));
	memset(pDataRes, 0, sizeof(MediaDataRes));
	wcscpy(pDataRes->wszPath, strOutFile.c_str());

}

void CObsRecordHelper::ObsBeginLogCatch()
{
	this->m_isLogCatch.SetAtomic(true);
	this->m_strLogCatch = "";
}

string CObsRecordHelper::ObsPauseLogCatch()
{
	this->m_isLogCatch.SetAtomic(false);
	return this->m_strLogCatch;
}

void CObsRecordHelper::ObsStopLogCatch()
{
	this->m_isLogCatch.SetAtomic(false);
	this->m_strLogCatch = "";
}

bool CObsRecordHelper::ObsInitial(
	CConfigHelper* pConfigHelper,
	CLogHelper* pLogHelper,
	const _string& strOwrDir)
{
	bool isRet = false;
	try
	{
		this->PushLogOper();
		this->m_pLogCatch.malloc(SIZE_8KB);
		this->m_fileHelper.Initialize();
		this->m_ObsCaptureHelper.InitCapture();
		this->m_ObsMouseRecord.InitMouse();
		this->ObsInitParam();

		ScreenShotCallBack screenShotCallBack =
			std::bind(&CObsRecordHelper::ScreenShotBack, this, _1, _2, _3, _4);
		this->m_ObsScreenHelper.InitScreenShot(screenShotCallBack);
		this->m_ObsFirstFrameHelper.InitScreenShot(
			[this](_string strFile, int nWidht, int nHeight) {
				this->m_imageThumbnail.nWidth = nWidht;
				this->m_imageThumbnail.nHeight = nHeight;
			});

		this->m_pConfigHelper = pConfigHelper;
		this->m_pLogHelper = pLogHelper;

		this->m_ObsAudioRecord.InitAudio();
		this->m_ObsVideoRecord.InitVideo();
		if (!this->m_ObsAudioRecord.SetAudioParam(this->m_obsRecParam.audioSet.nSamplerate)) {
			blog(LOG_ERROR, "set audio param error %d", __LINE__);
			return isRet;
		}
		
		this->m_obsRecParam.videoSet.szIn.cx = this->m_obsRecParam.recHeader.recArea.GetWidth();
		this->m_obsRecParam.videoSet.szIn.cy = this->m_obsRecParam.recHeader.recArea.GetHeight();

		this->m_obsRecParam.videoSet.szOut.cx = this->m_obsRecParam.recHeader.recArea.GetWidth();
		this->m_obsRecParam.videoSet.szOut.cy = this->m_obsRecParam.recHeader.recArea.GetHeight();

		if (!this->m_ObsVideoRecord.SetVideoParam(
			this->m_obsRecParam.videoSet, 
			this->m_obsRecParam.videoSet.szIn,
			this->m_obsRecParam.videoSet.szOut, true)) {
			blog(LOG_ERROR, "set video param error %d", __LINE__);
			return isRet;
		}

		obs_module_failure_info obsMfi;
		obs_load_all_modules2(&obsMfi);
		obs_log_loaded_modules();

		//this->m_ObsAudioRecord.SetAudioMonitoringDevice(
		//	this->m_obsRecParam.audioSet.strMicName.c_str(),
		//	this->m_obsRecParam.audioSet.strMicId.c_str());

		isRet = this->m_ObsAudioRecord.CreateAudioEncoder();
		if (!isRet) {
			blog(LOG_ERROR, "create audio encoder error %d", __LINE__);
			return isRet;
		}

		this->m_ObsVideoRecord.LoadVideoRecordingPreset(SIMPLE_ENCODER_X264);
		this->m_ObsAudioRecord.LoadAudioRecordingPreset();

		if (!this->m_ObsOutPutHelper.CreateOutPut()) {
		
			return isRet;
		}
	}
	catch (const char* error) {
		blog(LOG_ERROR, "catch %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "catch %d", __LINE__);
	}
	return isRet;
}

void CObsRecordHelper::SetCallBack(
	RecEncoderCallBack& fEncoderCallBack,
	ScreenShotCallBack& fScreenShotCallBack)
{
	this->mRecordCallBack = fEncoderCallBack;
	this->mScreenShotCallBack = fScreenShotCallBack;
}

bool CObsRecordHelper::ObsMuteApeaker(bool isMute)
{
	string strMicId =
		CUtilsConv::UnicodeToUtf8(this->m_obsRecParam.audioSet.strMicId);

	return this->m_ObsAudioRecord.MuteApeaker(strMicId.c_str(), isMute);
}

bool CObsRecordHelper::ObsMuteMic(bool isMute)
{
	string strSpeakerId =
		CUtilsConv::UnicodeToUtf8(this->m_obsRecParam.audioSet.strSpeakerId);
	return this->m_ObsAudioRecord.MuteMic(strSpeakerId.c_str(), isMute);
}

bool CObsRecordHelper::ObsIsApeaker()
{
	return this->m_ObsAudioRecord.IsSpeaker();
}

bool CObsRecordHelper::ObsIsMic()
{
	return this->m_ObsAudioRecord.IsMic();
}

bool CObsRecordHelper::ObsStartPre(
	UserInfoRes* pVarUserInfo,
	OperSetApi* pOperSetApi,
	ObsRecParam obsRecParam,
	int nTaskHeight,
	CWaterRes* pWaterRes,
	CMouseRes* pMouseRes)
{
	bool isRet = false;
	string strLog = "";
	int nTaskHeightTmp = nTaskHeight;
	CoAuto coAuto(_ModuleApp.UiTrdId());
	try
	{
		time_t nTimeStamp = 0;
		ullong dwBeginTime = CUtilsTime::GetNowMTimeStamp(nTimeStamp);
		ullong dwEndTime = dwBeginTime;
		this->ObsBeginLogCatch();
		HWND hRecWnd = NULL;
		this->m_pObsRecSource = nullptr;
		WaterInfo* pWaterInfo = nullptr;
		isRet = this->m_ObsSceneNormalHelper.CreateScene("Normal");
		if (!isRet) {
			blog(LOG_ERROR, "start create scene error %d", __LINE__);
			goto startpre_end;
		}

		this->m_pWaterRes = pWaterRes;
		this->m_pMouseRes = pMouseRes;
		this->m_obsRecParam = obsRecParam;
		int nWidth = this->m_obsRecParam.recHeader.recArea.GetWidth();
		int nHeight = this->m_obsRecParam.recHeader.recArea.GetHeight();
		this->m_obsRecParam.recHeader.recArea.GetRecSize(&nWidth, &nHeight);
		this->m_obsRecParam.recHeader.recArea.right =
			this->m_obsRecParam.recHeader.recArea.left + nWidth;
		this->m_obsRecParam.recHeader.recArea.bottom =
			this->m_obsRecParam.recHeader.recArea.top + nHeight;
		//CUtilsLog::OutLog(this->m_obsRecParam.recHeader.recArea.Printf(L"ARea"));
		isRet = this->m_ObsAudioRecord.SetAudioParam(
			this->m_obsRecParam.audioSet.nSamplerate);
		if (!isRet) {
			blog(LOG_ERROR, "start set_audio_param error %d", __LINE__);
			goto startpre_end;
		}

		nWidth = this->m_obsRecParam.recHeader.recArea.GetWidth();
		nHeight = this->m_obsRecParam.recHeader.recArea.GetHeight();

		this->m_obsRecParam.videoSet.szIn.cx 
			= this->m_obsRecParam.recHeader.recArea.GetWidth();
		this->m_obsRecParam.videoSet.szIn.cy 
			= this->m_obsRecParam.recHeader.recArea.GetHeight();

		this->m_obsRecParam.videoSet.szOut =
			this->m_pConfigHelper->GetResolutionSize(
				this->m_obsRecParam.videoSet.nResolution, nWidth, nHeight);

		isRet = this->m_ObsVideoRecord.SetVideoParam(
			this->m_obsRecParam.videoSet, 
			this->m_obsRecParam.videoSet.szIn,
			this->m_obsRecParam.videoSet.szOut);
		if (!isRet) {
			blog(LOG_ERROR, "start set_video_param error %d", __LINE__);
			goto startpre_end;
		}

		//UpdateRecording
		this->m_ObsVideoRecord.UpdateVideoRecordingSettingsX264Crf(
			this->m_obsRecParam.videoSet.usFps,
			this->m_obsRecParam.recHeader.recArea,
			this->m_obsRecParam.videoSet.enQuality,
			this->m_obsRecParam.videoSet.nCrf,
			this->m_obsRecParam.videoSet.nVideoBitrateType,
			this->m_obsRecParam.videoSet.nVideoBitrate,
			this->m_obsRecParam.videoSet.nVideoEnCode);

		this->m_ObsAudioRecord.UpdateAudioRecordingSettings(
			this->m_obsRecParam.audioSet.nBitRate);
		this->m_ObsAudioRecord.UpdateAudioBitrateSettings(
			this->m_obsRecParam.audioSet.nBitRate);

		//SetupOutputs 
		this->m_ObsAudioRecord.SetAudioEncoder();
		this->m_ObsVideoRecord.SetVideoEncoder();
		this->m_ObsAudioRecord.UpdateAudioEncoderTrack();

		this->m_ObsVideoRecord.UpdateVideoOutPut(this->m_ObsOutPutHelper.GetOutPutPtr());
		this->m_ObsAudioRecord.UpdateAudioOutPutTrack(this->m_ObsOutPutHelper.GetOutPutPtr());

		// 将场景设置为录制场景
		//obs_output_set_media(this->m_ObsOutPutHelper.GetOutPutPtr(), obs_get_video(), obs_get_audio());
		this->m_ObsOutPutHelper.UpdataOutPutConfigure(
			this->m_obsRecParam.outPathSet.strOutPath,
			m_pConfigHelper->GetOutVideoSuffixW(
				this->m_obsRecParam.videoSet.videoType, _T("mp4")),
			this->m_obsRecParam.recSqlitSet);
		
		int enRecType =
			this->m_obsRecParam.recHeader.enRecType;
		
		int enAudioType =
			this->m_obsRecParam.recHeader.enAudioType;

		string strMicId = 
			CUtilsConv::UnicodeToUtf8(this->m_obsRecParam.audioSet.strMicId);
		string strSpeakerId = 
			CUtilsConv::UnicodeToUtf8(this->m_obsRecParam.audioSet.strSpeakerId);

		bool isMicMute = true;
		bool isSpeakerMute = true;
		if ((enAudioType & RAT_MIC) != 0) { isMicMute = false; }
		if ((enAudioType & RAT_SPEAKER) != 0) { isSpeakerMute = false; }
		
		this->m_ObsAudioRecord.AddMic(strMicId.c_str(), isMicMute);
		this->m_ObsAudioRecord.AddSpeaker(strSpeakerId.c_str(), isSpeakerMute);
		this->m_ObsAudioRecord.AddFilterToAudioInput("noise_suppress_filter");
		
		//if (((enAudioType & RAT_SPEAKER) != 0) ||
		//	((enAudioType & RAT_MIC) != 0)) {
		//	this->m_ObsAudioRecord.AddFilterToAudioInput("noise_suppress_filter");
		//}

		if (((enRecType & RTT_SCREEN) != 0) ||
			((enRecType & RTT_AREA) != 0)) {
			isRet = this->m_ObsCaptureHelper.CreateCapture(
				this->m_ObsSceneNormalHelper.GetScenePtr(),
				this->m_obsRecParam.recHeader.hWnd,
				this->m_obsRecParam.recHeader.strMonitorId,
				this->m_obsRecParam.recHeader.recArea,
				this->m_obsRecParam.mouseSet.isMouse);
			pWaterInfo = this->m_pWaterRes->GetData(
				this->m_obsRecParam.recHeader.recArea.ToRect());
			if (isRet) {
				this->m_pObsRecSource = this->m_ObsCaptureHelper.GetCapSourcePtr();
			}
			if ((enRecType & RTT_AREA) != 0) {
				nTaskHeightTmp = 0;
			}
		}
		else if ((enRecType & RTT_WND) != 0) {
			nTaskHeightTmp = 0;
			hRecWnd = this->m_obsRecParam.recHeader.hWnd;
			isRet = this->m_ObsCaptureHelper.CreateCapture(
				this->m_ObsSceneNormalHelper.GetScenePtr(),
				this->m_obsRecParam.recHeader.hWnd,
				this->m_obsRecParam.recHeader.strMonitorId,
				this->m_obsRecParam.recHeader.recArea,
				this->m_obsRecParam.mouseSet.isMouse);
			pWaterInfo = this->m_pWaterRes->GetData(
				this->m_obsRecParam.recHeader.recArea.ToRect());
			if (isRet) {
				this->m_pObsRecSource = this->m_ObsCaptureHelper.GetCapSourcePtr();
			}
		}
		else if ((enRecType & RTT_CAMERA) != 0) {
			this->m_ObsPreviewHelper.SetPreDoType(PDT_START);
			this->m_obsRecParam.mouseSet.isMouse = false;
			isRet = this->m_ObsCameraHelper.CreateCamera(
				this->m_ObsSceneNormalHelper.GetScenePtr(),
				this->m_obsRecParam.recHeader, 
				this->m_obsRecParam.cameraSet);
			pWaterInfo = this->m_pWaterRes->GetData(
				this->m_obsRecParam.recHeader.recArea.ToRect());
			if (isRet) {
				this->m_ObsPreviewHelper.CreatePreview(
					this->m_ObsCameraHelper.GetCameraSourcePtr(),
					this->m_obsRecParam.recHeader.hWndPre,
					this->m_obsRecParam.recHeader.recArea.GetWidth(),
					this->m_obsRecParam.recHeader.recArea.GetHeight());
				this->m_ObsPreviewHelper.StartPreview();
				this->m_pObsRecSource = this->m_ObsCameraHelper.GetCameraSourcePtr();
			}
		}

		if (!isRet) {
			blog(LOG_ERROR, "create capture error %d", __LINE__);
			goto startpre_end;
		}

		if (!pWaterInfo) {
			isRet = false;
			blog(LOG_ERROR, "resize img error %d", __LINE__);
			goto startpre_end;
		}
		
	}
	catch (const char* error) {
		blog(LOG_ERROR, "start failed %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "start failed %d", __LINE__);
	}

startpre_end:
	strLog = this->ObsPauseLogCatch();
	try
	{
		if (!isRet) {
			this->m_ObsSceneNormalHelper.DisplayScene();
			this->m_ObsAudioRecord.DisplayAudio();
			this->m_ObsVideoRecord.ReductionVideoFps();
			this->m_ObsVideoRecord.DisplayVideo();
			this->m_ObsCaptureHelper.DisplayCapture();
		}
	}
	catch (...)
	{
		blog(LOG_ERROR, "start display failed %d", __LINE__);
	}
	this->ObsStopLogCatch();
	return isRet;
}

bool CObsRecordHelper::ObsStart()
{
	bool isRet = false;
	this->ObsBeginLogCatch();
	try
	{
		this->m_imageThumbnail.strImage = CUtilsFile::GetImageThumbnailPath();
		//obs_set_output_source(0, obs_scene_get_source(this->m_ObsSceneHelper.GetScenePtr()));
		//obs_encoder_set_audio(encoder, this->m_ObsSceneHelper.GetScenePtr());
		isRet = this->m_ObsOutPutHelper.StartOutPut();
		if (isRet) 
		{
			this->m_fileHelper.InitFile(0);
			this->m_fileHelper.OpenFile(this->m_ObsOutPutHelper.GetOutFile());
			this->m_recTimeHelper.Start();
		}
		else 
		{
			const char* error = obs_output_get_last_error(
				this->m_ObsOutPutHelper.GetOutPutPtr());
			if (error) {
				blog(LOG_ERROR, "start failed %s %d", error, __LINE__);
			}
			else {
				blog(LOG_ERROR, "start failed %d", __LINE__);
			}
		}
	}
	catch (const char* error) {
		blog(LOG_ERROR, "start failed %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "start failed %d", __LINE__);
	}
	this->SetStrLastError(this->ObsPauseLogCatch());
	this->ObsStopLogCatch();

	return isRet;
}

bool CObsRecordHelper::ObsPause()
{
	bool isRet = false;
	try
	{
		isRet = this->m_ObsOutPutHelper.PauseOutPut();
		if (isRet) {
			this->m_recTimeHelper.Pause();
		}
	}
	catch (const char* error) {
		blog(LOG_ERROR, "pause failed %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "pause failed %d", __LINE__);
	}
	return isRet;
}

bool CObsRecordHelper::ObsRestore()
{
	bool isRet = false;
	try
	{
		isRet = this->m_ObsOutPutHelper.RestoreOutPut();
		if (isRet) {
			this->m_recTimeHelper.Resume();
		}
	}
	catch (const char* error) {
		blog(LOG_ERROR, "restore failed %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "restore failed %d", __LINE__);
	}
	return isRet;
}

DWORD CObsRecordHelper::ObsRecTimeCount()
{
	return m_recTimeHelper.Query();
}

DWORD CObsRecordHelper::ObsFileSize()
{
	_string strOutFile = this->m_ObsOutPutHelper.GetOutFile();
	return m_fileHelper.FileSize(strOutFile);
}

obs_source_t* CObsRecordHelper::ObsScreenShotSource()
{
	obs_source_t* pSource = nullptr;
	if (!this->ObsIsCameraRec()) {
		pSource = this->m_pObsRecSource;
	} else {
		obs_scene_t* pScene =
			this->m_ObsSceneCameraHelper.GetScenePtr();
		if (pScene) {
			pSource = obs_scene_get_source(pScene);
		}
	}
	return pSource;
}

void CObsRecordHelper::ObsStop(bool isForce)
{
	try
	{
		this->m_fileHelper.CloseFile();
		this->m_recTimeHelper.WeakStop();
		int enRecType =
			this->m_obsRecParam.recHeader.enRecType;
		if ((enRecType & RTT_CAMERA) != 0)
		{
			this->m_ObsOutPutHelper.GetOutFile();
			this->m_ObsOutPutHelper.StopOutPut(isForce);
			this->m_ObsAudioRecord.DisplayAudio();
			this->m_ObsVideoRecord.DisplayVideo();
			this->m_ObsTextRecord.DisplayText(
				this->m_ObsSceneCameraHelper.GetScenePtr());
		}
		else
		{
			this->m_ObsOutPutHelper.GetOutFile();
			this->m_OsbMouseHelper.StopMouse();
			this->m_ObsOutPutHelper.StopOutPut(isForce);
			this->m_ObsSceneNormalHelper.DisplayScene();
			this->m_ObsAudioRecord.DisplayAudio();
			this->m_ObsVideoRecord.ReductionVideoFps();
			this->m_ObsVideoRecord.DisplayVideo();
			this->m_ObsCaptureHelper.DisplayCapture();
		}
	}
	catch (const char* error) {
		blog(LOG_ERROR, "Stop Failed to recreate D3D11: %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "Stop Failed to recreate D3D11: %d", __LINE__);
	}
}

_string CObsRecordHelper::ObsRecFile()
{
	return this->m_ObsOutPutHelper.GetOutFile();
}

void CObsRecordHelper::ObsDisplay()
{
	if (!obs_initialized()) {
		return;
	}

	try
	{
		signal_handler_t* pHandler =
			obs_output_get_signal_handler(this->m_ObsOutPutHelper.GetOutPutPtr());
		// 连接信号处理程序和回调函数
		signal_handler_disconnect(pHandler, "stop",
			CObsRecordHelper::StopRecordingCallback, this);

		this->m_ObsOutPutHelper.DisplayOutPut();
		this->m_ObsSceneNormalHelper.DisplayScene();
		this->m_ObsAudioRecord.DisplayAudio();
		this->m_ObsVideoRecord.DisplayVideo();
		this->m_ObsCaptureHelper.DisplayCapture();

		if (this->m_pLoopRecordFile)
		{
			delete this->m_pLoopRecordFile;
			this->m_pLoopRecordFile = nullptr;
		}

		if (this->m_pScreenEvent)
		{
			delete this->m_pScreenEvent;
			this->m_pScreenEvent = nullptr;
		}
		this->m_pLogCatch.free();
	}
	catch (const char* error) {
		blog(LOG_ERROR, "Stop Failed to recreate D3D11: %s %d", error, __LINE__);
	}
	catch (...) {
		blog(LOG_ERROR, "Stop Failed to recreate D3D11: %d", __LINE__);
	}
	//obs_shutdown();
}

void CObsRecordHelper::ObsDisplayEnd(HANDLE hProcess, HWND hWnd)
{

}