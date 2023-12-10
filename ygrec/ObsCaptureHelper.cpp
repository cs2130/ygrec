#include "../stdafx.h"
#include "ObsCaptureHelper.h"


enum window_capture_method {
	METHOD_AUTO,
	METHOD_BITBLT,
	METHOD_WGC,
};

CObsCaptureHelper::CObsCaptureHelper()
{
	this->InitCapture();
}

CObsCaptureHelper::~CObsCaptureHelper()
{

}

void CObsCaptureHelper::InitCapture()
{
	this->m_pCapSource = nullptr;
	this->m_strMonitorId = "";
	this->m_pProperties = nullptr;
	this->InitSourceParam();
	this->SetSourceBaseName(VIDEO_CAPTURE_NAME);
}

bool CObsCaptureHelper::CreateCapture(
	obs_scene_t* pScene, HWND hWnd, 
	string strMonitorId, RectX recArea,
	bool isMouse)
{
	bool isRet = false;
	this->ReInitCapture();
	this->m_recArea = recArea;
	this->m_strMonitorId = strMonitorId;
	if (hWnd == NULL) {
		isRet = this->CreateDesk(pScene, isMouse);
	}
	else {
		isRet = this->CreateWnd(pScene, hWnd, isMouse);
	}
	return isRet;
}

bool CObsCaptureHelper::DisplayCapture()
{
	if (m_pProperties) {
		obs_properties_destroy(m_pProperties);
	}
	this->m_pProperties = nullptr;
	return true;
}

void CObsCaptureHelper::ReInitCapture()
{
	this->m_pCapSource = nullptr;
	this->m_strMonitorId = "";
	this->m_pProperties = nullptr;
	this->ClearSourceName();
}

void CObsCaptureHelper::AddSource(void* _data, obs_scene_t* scene)
{
	// 为这个 source 创建一个 scene item
	obs_source_t* source = (obs_source_t*)_data;
	obs_sceneitem_t* item = obs_scene_add(scene, source);

	//if (m_nIndex == 2) {
	//	vec2 pos;
	//	obs_sceneitem_get_pos(item, &pos);
	//	vec2_set(&pos, 200.0, 200.0);
	//	obs_sceneitem_set_pos(item, &pos);
	//}

	obs_source_release(source);
}

bool CObsCaptureHelper::CreateWnd(obs_scene_t* pScene, HWND hWnd, bool isMouse)
{
	bool isRet = false;
	obs_data_t* pSetting = nullptr;
	obs_data_t* pCurSetting = nullptr;
	string strWndProItemName = "";

	string strSourceName = this->BuildSourceName("Wnd");
	// 创建窗口捕获源，它是 scene 里唯一的一个 scene item
	this->m_pCapSource = obs_source_create(VIDEO_WINDOW_CAPTURE_ID, strSourceName.c_str(),
		NULL, nullptr);
	if (this->m_pCapSource) {
		obs_scene_atomic_update(pScene, AddSource, this->m_pCapSource);
	}
	else {
		blog(LOG_ERROR, "create source failed %d", __LINE__);
		return isRet;
	}

	// 添加窗口捕获源的剪裁过滤器，可以实现录制窗口特区域
	this->AddFilterToSource(this->m_pCapSource, VIDEO_CROP_FILTER_ID);
	this->SetFilterToSource(this->m_pCapSource);

	// 设置窗口捕获原的窗口
	pSetting = obs_data_create();
	pCurSetting = obs_source_get_settings(this->m_pCapSource);
	obs_data_apply(pSetting, pCurSetting);
	obs_data_release(pCurSetting);
	strWndProItemName = this->GetWndProItemName(hWnd);

	this->m_pProperties = obs_source_properties(this->m_pCapSource);
	if (!isRet) {
		isRet = this->FindWnd(this->m_pProperties, strWndProItemName, pSetting);
	}
	if (!isRet) {
		isRet = this->FindWndEx(this->m_pProperties, strWndProItemName, pSetting);
	}
	obs_source_update(this->m_pCapSource, pSetting);
	obs_data_release(pSetting);
	obs_source_set_enabled(this->m_pCapSource, true);
	return isRet;
}

bool CObsCaptureHelper::CreateDesk(obs_scene_t* pScene, bool isMouse)
{
	bool isRet = false;
	obs_data_t* pSetting = nullptr;
	obs_data_t* pCurSetting = nullptr;

	string strSourceName = this->BuildSourceName("");
	this->m_pCapSource = obs_source_create(VIDEO_MONITOR_CAPTURE_ID, strSourceName.c_str(),
		NULL, nullptr);
	if (this->m_pCapSource) {
		obs_scene_atomic_update(pScene, AddSource, this->m_pCapSource);
	}
	else {
		blog(LOG_ERROR, "create source failed %d", __LINE__);
		return isRet;
	}

	// 添加窗口捕获源的剪裁过滤器，可以实现录制窗口特区域
	this->AddFilterToSource(this->m_pCapSource, VIDEO_CROP_FILTER_ID);
	this->SetFilterToSource(this->m_pCapSource);

	// 设置窗口捕获原的窗口
	pSetting = obs_data_create();
	pCurSetting = obs_source_get_settings(this->m_pCapSource);
	obs_data_apply(pSetting, pCurSetting);
	obs_data_release(pCurSetting);

	obs_source_update(this->m_pCapSource, pSetting);
	obs_data_release(pSetting);
	obs_source_set_enabled(this->m_pCapSource, true);
	isRet = true;
	return isRet;
}

void CObsCaptureHelper::AddFilterToSource(obs_source_t* pSource, const char* id)
{
	if (!id || *id == '\0')
		return;

	obs_source_t* existing_filter;
	std::string name = obs_source_get_display_name(id);
	existing_filter = obs_source_get_filter_by_name(this->m_pCapSource, name.c_str());
	if (existing_filter) {
		blog(LOG_WARNING, "filter %s exists %d", id, __LINE__);
		obs_source_release(existing_filter);
		return;
	}

	name = std::string(OBS_TAG) + id;
	obs_source_t* filter = obs_source_create(id, name.c_str(), nullptr, nullptr);
	if (filter) {
		const char* sourceName = obs_source_get_name(this->m_pCapSource);

		blog(LOG_INFO, "add filter '%s' (%s) to source '%s'",
			name.c_str(), id, sourceName);
		obs_source_filter_add(this->m_pCapSource, filter);
		obs_source_release(filter);
	}
}

void CObsCaptureHelper::SetFilterToSource(obs_source_t* pSource)
{
	bool isRelative = false;
	std::string strName = OBS_TAG VIDEO_CROP_FILTER_ID;
	obs_source_t* pExistingFilter =
		obs_source_get_filter_by_name(this->m_pCapSource, strName.c_str());
	if (pExistingFilter) {
		//CUtilsLog::OutLog(this->m_recArea.Printf(L"SetFilterToSource >> Area"));
		obs_data_t* pSettings = obs_source_get_settings(pExistingFilter);
		obs_data_set_bool(pSettings, "relative", isRelative);
		int nLeft = this->m_recArea.left;
		int nTop = this->m_recArea.top;
		if (nLeft < 0) { nLeft = 0; }
		if (nTop < 0) { nTop = 0; }
		obs_data_set_int(pSettings, "left", nLeft);
		obs_data_set_int(pSettings, "top", nTop);
		obs_source_update(pExistingFilter, pSettings);
		obs_data_release(pSettings);
		obs_source_release(pExistingFilter);
	}
}

bool CObsCaptureHelper::FindWnd(obs_properties_t* pProperties, string strWndProItemName, obs_data_t* pSetting)
{
	bool isRet = false;
	obs_property_t* pProperty = nullptr;
	pProperty = obs_properties_first(pProperties);

	while (pProperty) {
		const char* name = obs_property_name(pProperty);
		if (strcmp(name, "window") == 0) {
			size_t count = obs_property_list_item_count(pProperty);
			const char* str = nullptr;
			for (size_t i = 0; i < count; i++) {
				const char* itemname = obs_property_list_item_name(pProperty, i);
				if (stricmp(itemname, strWndProItemName.c_str()) == 0) {
					str = obs_property_list_item_string(pProperty, i);
					break;
				}
			}
			if (str) {
				isRet = true;
				obs_data_set_string(pSetting, name, str);
				blog(LOG_INFO, "!!!Found item=%s", str);
				break;
			} else {
				blog(LOG_ERROR, "find window failed. %s %d", strWndProItemName.c_str(), __LINE__);
				isRet = false;
				break;
			}
		}
		obs_property_next(&pProperty);
	}
	return isRet;
}

bool CObsCaptureHelper::FindWndEx(obs_properties_t* pProperties, string strWndProItemName, obs_data_t* pSetting)
{
	bool isRet = false;
	obs_property_t* pProperty = obs_properties_get(pProperties, "window");
	size_t sCount = obs_property_list_item_count(pProperty);
	const char* pszValue = nullptr;
	for (size_t i = 0; i < sCount; i++) {
		const char* pszName = obs_property_list_item_name(pProperty, i);
		if (stricmp(pszName, strWndProItemName.c_str()) == 0) {
			pszValue = obs_property_list_item_string(pProperty, i);
			break;
		}
	}
	if (pszValue) {
		isRet = true;
		obs_data_set_string(pSetting, "window", pszValue);
	} else {
		isRet = false;
	}
	return true;
}

bool CObsCaptureHelper::FindDesk(obs_source_t* pSource, obs_data_t* pSetting)
{
	bool isRet = true;
	obs_property_t* pProperty = nullptr;

	this->m_pProperties = obs_source_properties(pSource);
	pProperty = obs_properties_first(this->m_pProperties);

	return isRet;
}

string CObsCaptureHelper::GetWndProItemName(HWND hWnd)
{
	string strWndTitle = "";
	string strProName = "";

	wstring wstrProName = L"";
	char szItemName[2048] = { 0 };
	if (wstrProName.empty()) {
		wstrProName = this->GetProcessNameEx(hWnd);
	}
	if (wstrProName.empty()) {
		wstrProName = this->GetProcessName(hWnd);
	}
	this->GetWindowTitle(hWnd, strWndTitle);

	strProName = CUtilsConv::WCharToUtf8(wstrProName.c_str());

	return string(szItemName);
}

//获取进程完整路径
wstring CObsCaptureHelper::GetProcessName(HWND hWnd)
{
	wstring strRet = L"";
	wstring strFullPath = L"";
	DWORD dwPid = 0;
	GetWindowThreadProcessId(hWnd, &dwPid);
	TCHAR szFullPath[MAX_PATH] = { 0 };
	HANDLE hProcess = NULL;
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (!hProcess) { goto end_func; }
	if (GetModuleFileNameEx(hProcess, NULL, szFullPath, MAX_PATH) != 0) {
		strFullPath = wstring(szFullPath);
	} else {
		memset(szFullPath, 0, MAX_PATH);
		if (GetProcessImageFileName(hProcess, szFullPath, MAX_PATH) != 0) {
			strFullPath = wstring(szFullPath);
		}
	}
	if (strFullPath.empty()) { goto end_func; }
	strRet = this->GetPathFileName(strFullPath);
end_func:
	if (hProcess) { ::CloseHandle(hProcess); }
	hProcess = NULL;
	return strRet;
}

wstring CObsCaptureHelper::GetProcessNameEx(HWND hWnd)
{
	wstring strRet = L"";
	DWORD dwPid = 0;
	HANDLE hProcess = NULL;
	wchar_t wszPath[MAX_PATH] = {0};
	::GetWindowThreadProcessId(hWnd, &dwPid);
	hProcess = _ModuleApp.FuncPtr()->InvokeOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);
	if (!hProcess) { goto end_func; }
	if (!GetProcessImageFileNameW(hProcess, wszPath, MAX_PATH)) {
		goto end_func;
	}
	strRet = this->GetPathFileName(wszPath);
end_func:
	if (hProcess) { ::CloseHandle(hProcess); }
	hProcess = NULL;
	return strRet;
}

wstring CObsCaptureHelper::GetPathFileName(wstring strFile)
{
	wstring strRet = L"";
	strFile = this->Replace(strFile, L"/", L"\\");
	int pos = strFile.rfind('\\');
	if (pos != wstring::npos)
	{
		strRet = strFile.substr(pos + 1, strFile.size() - pos - 1);
	}
	return strRet;
}

wstring CObsCaptureHelper::Replace(wstring src, wstring old, wstring _new)
{
	wstring first;
	wstring last;
	int pos = src.find(old);
	if (pos != -1)
		first = src.substr(0, pos);
	else
		return src;
	pos = pos + old.length();
	if (pos < src.length())
	{
		last = src.substr(pos, src.length());
		return first + _new + this->Replace(last, old, _new);
	}
	else if (pos == src.length())
	{
		return first + _new;
	}
	else
	{
		return src;
	}
}

bool CObsCaptureHelper::GetWindowTitle(HWND hWnd, string& strTitle)
{
	_string strTitleW = _T("");
	size_t nLen = (size_t)GetWindowTextLengthW(hWnd);
	strTitleW.resize(nLen);
	if (!GetWindowTextW(hWnd, &strTitleW[0], (int)nLen + 1))
		return false;
	nLen = os_wcs_to_utf8(strTitleW.c_str(), 0, nullptr, 0);
	strTitle.resize(nLen);
	os_wcs_to_utf8(strTitleW.c_str(), 0, &strTitle[0], nLen + 1);
	return true;
}