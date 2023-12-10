#include "../stdafx.h"
#include "ObsBaseHelper.h"

CObsBaseHelper::CObsBaseHelper()
{

}

CObsBaseHelper::~CObsBaseHelper()
{

}

void CObsBaseHelper::InitSourceParam()
{
	this->m_strSourceBaseName = "";
	this->m_dwSourceNo = 0;
	this->m_mapSourceName.clear();
}

void CObsBaseHelper::SetSourceBaseName(string strSourceBaseName)
{
	this->m_strSourceBaseName = strSourceBaseName;
}

void CObsBaseHelper::ClearSourceName()
{
	this->m_mapSourceName.clear();
}

string CObsBaseHelper::BuildSourceName()
{
	string strRet = "";
	strRet = fmt::format("{0}_{1}",
		this->m_strSourceBaseName, ::InterlockedIncrement(&m_dwSourceNo));

	std::map<string, string>::iterator iter
		= this->m_mapSourceName.find(this->m_strSourceBaseName);
	if (iter != this->m_mapSourceName.end()) {
		iter->second = strRet;
	} else {
		this->m_mapSourceName.insert(std::pair<string, string>(this->m_strSourceBaseName, strRet));
	}
	return strRet;
}

string CObsBaseHelper::BuildSourceName(string strAttach)
{
	string strRet = "";
	string strKey = "";
	strKey = fmt::format("{0}_{1}",
		this->m_strSourceBaseName, strAttach);
	strRet = fmt::format("{0}_{1}_{2}",
		this->m_strSourceBaseName, ::InterlockedIncrement(&m_dwSourceNo), strAttach);
	
	std::map<string, string>::iterator iter
		= this->m_mapSourceName.find(strKey);
	if (iter != this->m_mapSourceName.end()) {
		iter->second = strRet;
	}
	else {
		this->m_mapSourceName.insert(std::pair<string, string>(strKey, strRet));
	}
	return strRet;
}


string CObsBaseHelper::GetSouceName()
{
	string strRet = "";
	std::map<string, string>::iterator iter
		= this->m_mapSourceName.find(this->m_strSourceBaseName);
	if (iter != this->m_mapSourceName.end()) {
		strRet = iter->second;
	}
	return strRet;
}

string CObsBaseHelper::GetSourceName(string strAttach)
{
	string strRet = "";
	string strKey = "";
	strKey = fmt::format("{0}_{1}",
		this->m_strSourceBaseName, strAttach);

	std::map<string, string>::iterator iter
		= this->m_mapSourceName.find(strKey);
	if (iter != this->m_mapSourceName.end()) {
		strRet = iter->second;
	}
	return strRet;
}