#ifndef __OBS_CAPTUREHELPER_H__
#define __OBS_CAPTUREHELPER_H__
#pragma once

class CObsCaptureHelper
	: public CObsBaseHelper
{
public:
	CObsCaptureHelper();
	~CObsCaptureHelper();

public:
	void InitCapture();
	bool CreateCapture(obs_scene_t* pScene, HWND hWnd, string strMonitorId, RectX recArea, bool isMouse);
	inline obs_source_t* GetCapSourcePtr() {
		return this->m_pCapSource;
	}
	bool DisplayCapture();

private:
	void ReInitCapture();
	bool CreateWnd(obs_scene_t* pScene, HWND hWnd, bool isMouse);
	bool CreateDesk(obs_scene_t* pScene, bool isMouse);
	void AddFilterToSource(obs_source_t* pSource, const char* id);
	void SetFilterToSource(obs_source_t* pSource);

private:
	static void AddSource(void* _data, obs_scene_t* scene);
	bool FindWnd(obs_properties_t* pProperties, string strWndProItemName, obs_data_t* pSetting);
	bool FindWndEx(obs_properties_t* pProperties, string strWndProItemName, obs_data_t* pSetting);
	bool FindDesk(obs_source_t* pSource, obs_data_t* pSetting);

private:
	string  GetWndProItemName(HWND hWnd);
	wstring GetProcessName(HWND hWnd);
	wstring GetProcessNameEx(HWND hWnd);
	wstring GetPathFileName(wstring strFile);
	wstring Replace(wstring src, wstring old, wstring _new);
	bool GetWindowTitle(HWND hWnd, string& strTitle);

private:
	obs_source_t*		m_pCapSource;
	obs_properties_t*	m_pProperties;
	RectX				m_recArea;
	string				m_strMonitorId;
};

#endif