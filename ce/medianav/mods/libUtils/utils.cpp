#include "utils.h"
#include <Tlhelp32.h>
#include <cctype>
#include <algorithm>
#include <cassert>


namespace Utils {

bool RegistryAccessor::setValue(HKEY hRootKey, const std::wstring& subKey, DWORD dwType, const std::wstring& valueName, const std::vector<BYTE>& data)
{
	HKEY hKey;
	if(RegCreateKeyEx(hRootKey, subKey.c_str(), 0, NULL, 0, NULL, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return false;

	if(RegSetValueEx(hKey, valueName.c_str(), 0, dwType, &data[0], data.size()) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

bool RegistryAccessor::setString(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& value)
{
	std::vector<BYTE> data((value.size()+1) * 2);
    memcpy(&data[0], value.c_str(), data.size() + 2);
	return RegistryAccessor::setValue(hRootKey, subKey, REG_SZ, valueName, data);
}

bool RegistryAccessor::setInt(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, unsigned int value)
{
	std::vector<BYTE> data(sizeof(value));
    memcpy(&data[0], &value, sizeof(value));
	return RegistryAccessor::setValue(hRootKey, subKey, sizeof(value), valueName, data);
}

bool RegistryAccessor::setBool(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, bool value)
{
    return setInt(hRootKey, subKey, valueName, value ? 1 : 0);
}

bool RegistryAccessor::getValue(HKEY hRootKey, const std::wstring& subKey, DWORD dwType, const std::wstring& valueName, std::vector<BYTE>& data)
{
	HKEY hKey;
	if(RegCreateKeyEx(hRootKey, subKey.c_str(), 0, NULL, 0, NULL, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return false;

	DWORD sizeRequired = 0;
    DWORD dwTypeStored = 0;
    if(RegQueryValueEx(hKey, valueName.c_str(), 0, &dwTypeStored, NULL, &sizeRequired) != ERROR_SUCCESS)
	{
    	RegCloseKey(hKey);
        return false;
	}

    if(dwTypeStored != dwType)
    {
    	RegCloseKey(hKey);
        return false;
    }

    data.resize(sizeRequired);
    if(RegQueryValueEx(hKey, valueName.c_str(), 0, NULL, &data[0], &sizeRequired) != ERROR_SUCCESS)
	{
	    RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);
	return true;
}

std::wstring RegistryAccessor::getString(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& defaultValue)
{
    std::vector<BYTE> pureData;
    if(!RegistryAccessor::getValue(hRootKey, subKey, REG_SZ, valueName, pureData))
        return defaultValue;

    std::wstring data;
    data.assign(reinterpret_cast<wchar_t *>(&pureData[0]), pureData.size());
    data.resize(wcslen(&data[0]));
    return data;
}

DWORD RegistryAccessor::getInt(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, DWORD defaultValue)
{
    std::vector<BYTE> pureData;
    if(!RegistryAccessor::getValue(hRootKey, subKey, REG_DWORD, valueName, pureData))
        return defaultValue;
    
    assert(pureData.size() == sizeof(DWORD));
    return *(reinterpret_cast<DWORD *>(&pureData[0]));;
}

bool RegistryAccessor::getBool(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, bool defaultValue)
{
    DWORD value = defaultValue ? 1 : 0;
    return getInt(hRootKey, subKey, valueName, value) == 1 ? true : false;
}

    
DWORD getCurrentProcessImageBase()
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
     
    DWORD curProcessID = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        
    DWORD imageBase = (DWORD)-1;

    if(snapshot != INVALID_HANDLE_VALUE && Process32First(snapshot, &entry) == TRUE)
    {
        do
        {
            if(entry.th32ProcessID == curProcessID)
            {
                imageBase = entry.th32MemoryBase;
                break;
            }
        }while (Process32Next(snapshot, &entry) == TRUE);
        CloseToolhelp32Snapshot(snapshot);
    }
    return imageBase;
}

DWORD getCurrentProcessCodeBase()
{
    SYSTEM_INFO sinf;
    GetSystemInfo(&sinf);
    void* baseAddr = sinf.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mem;
    while(VirtualQuery(baseAddr, &mem, sizeof(mem)) == sizeof(mem))
    {
        if(mem.Protect & (PAGE_READONLY | PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
            return (DWORD)mem.BaseAddress;
        baseAddr = (void*)((DWORD)mem.BaseAddress + mem.RegionSize);
    }
    return -1;
}

void toUpper(std::wstring& str)
{
    std::for_each(str.begin(), str.end(),std::toupper);
}

SystemWideUniqueInstance::SystemWideUniqueInstance(const std::wstring& name)
    :isUnique_(false)
{
    hMutex_ = CreateMutex(NULL, FALSE, name.c_str());
    
    if(hMutex_ == NULL)
    {
        logPrintf(L"Unable to CREATE mutex: name %s\r\n", name.c_str());
        return;
    }
    if(WaitForSingleObject(hMutex_, 0) != WAIT_OBJECT_0)
    {
        if(!CloseHandle(hMutex_))
        {
            logPrintf(L"Unable to CLOSE mutex: name %s\r\n", name.c_str());
        }
        hMutex_ = NULL;
        return;
    }
    isUnique_ = true;
}

SystemWideUniqueInstance::~SystemWideUniqueInstance()
{
    if(hMutex_ != NULL )
    {
        if(!ReleaseMutex(hMutex_))
        {
            logPrintf(L"Unable to RELEASE mutex\r\n");
        }
        if(!CloseHandle(hMutex_))
        {
            logPrintf(L"Unable to CLOSE mutex\r\n");
        }
    }
}

bool SystemWideUniqueInstance::isUnique()
{
    return isUnique_;
}

}; //namespace Utils

