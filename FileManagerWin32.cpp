#include "FileManagerWin32.h"

FileManager::FileManager() {}

FileManager::~FileManager() {}

bool FileManager::OpenFileDialog(std::string& filePath, std::string& selectedFile)
{
	// Initialize COM for the current thread
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
		return false;

	// Create the FileOpenDialog object
	IFileOpenDialog* pFileDialog;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));
	if (FAILED(hr)) {
		CoUninitialize();
		return false;
	}

	// Show the Open File dialog window
	hr = pFileDialog->Show(NULL);
	if (FAILED(hr)) {
		pFileDialog->Release();
		CoUninitialize();
		return false;
	}

	// Get the selected file
	IShellItem* pItem;
	hr = pFileDialog->GetResult(&pItem);
	if (FAILED(hr)) {
		pFileDialog->Release();
		CoUninitialize();
		return false;
	}

	// Get the file path
	PWSTR pszFilePath;
	hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
	if (FAILED(hr)) {
		pItem->Release();
		pFileDialog->Release();
		CoUninitialize();
		return false;
	}

	// Convert WCHAR to std::wstring
	sFilePath = pszFilePath;

	// Convert std::wstring to std::string
	filePath.assign(sFilePath.begin(), sFilePath.end());

	// Extract the selected file name
	size_t pos = sFilePath.find_last_of(L"/\\");
	if (pos != std::wstring::npos) {
		selectedFile = std::string(filePath.begin() + pos + 1, filePath.end());
	}

	// Cleanup
	CoTaskMemFree(pszFilePath);
	pItem->Release();
	pFileDialog->Release();
	CoUninitialize();

	return true;
}

bool FileManager::SaveFileDialog(std::string& filePath, const std::wstring& defaultFileName)
{
	// Initialize COM for the current thread
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
		return false;

	// Create the FileSaveDialog object
	IFileSaveDialog* pFileSaveDialog;
	hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSaveDialog));
	if (FAILED(hr)) {
		CoUninitialize();
		return false;
	}

	// Set default file name
	pFileSaveDialog->SetFileName(defaultFileName.c_str());

	// Show the Save File dialog window
	hr = pFileSaveDialog->Show(NULL);
	if (FAILED(hr)) {
		pFileSaveDialog->Release();
		CoUninitialize();
		return false;
	}

	// Get the selected file
	IShellItem* pItem;
	hr = pFileSaveDialog->GetResult(&pItem);
	if (FAILED(hr)) {
		pFileSaveDialog->Release();
		CoUninitialize();
		return false;
	}

	// Get the file path
	PWSTR pszFilePath;
	hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
	if (FAILED(hr)) {
		pItem->Release();
		pFileSaveDialog->Release();
		CoUninitialize();
		return false;
	}

	// Convert WCHAR to std::wstring
	sFilePath = pszFilePath;

	// Convert std::wstring to std::string
	filePath.assign(sFilePath.begin(), sFilePath.end());

	// Cleanup
	CoTaskMemFree(pszFilePath);
	pItem->Release();
	pFileSaveDialog->Release();
	CoUninitialize();

	return true;
}
