#pragma once

#include <Windows.h>
#include <string>
#include <Shobjidl.h>  // Include Windows Shell API for IFileOpenDialog and IFileSaveDialog

class FileManager
{
public:
	FileManager();
	~FileManager();

	bool OpenFileDialog(std::string& filePath, std::string& selectedFile);
	bool SaveFileDialog(std::string& filePath, const std::wstring& defaultFileName);

private:
	std::wstring sFilePath; // Use wstring to handle wide characters
};
