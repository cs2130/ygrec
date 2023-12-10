#ifndef __OBS_BASE_HELPER_H__
#define __OBS_BASE_HELPER_H__
#pragma once

class CObsBaseHelper
{
public:
	CObsBaseHelper();
	~CObsBaseHelper();

protected:
	void InitSourceParam();
	void ClearSourceName();
	void SetSourceBaseName(string strSourceBaseName);
	string BuildSourceName();
	string BuildSourceName(string strAttach);
	string GetSouceName();
	string GetSourceName(string strAttach);

private:
	map<string, string>	m_mapSourceName;
	string				m_strSourceBaseName;
	volatile DWORD		m_dwSourceNo;
};

#endif // !__OBS_BASE_HELPER_H__

