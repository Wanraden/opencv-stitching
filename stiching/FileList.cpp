#include <iostream>
#include "FileList.h"

FileList::FileList()
{
}

FileList::~FileList()
{
}

std::string FileList::getIndexPath(int n)
{
	return m_path.empty() ? std::string() : m_path.at(n) ;
}

size_t FileList::getCount()
{
	return m_path.size();
}

void FileList::delIndexPath(int n)
{
	if (n > m_path.size())
		return;
	int i = 0;
	for (auto it = m_path.begin(); it != m_path.end(); it++)
	{
		if (i++ == n)
		{
			m_path.erase(it);
			break;
		}
	}
}

void FileList::outFile()
{
	for (auto file : m_path)
		std::cout << file << std::endl;
}

std::shared_ptr<FileList> FileList::NewFileList(ENUM_FILETYPE fileType, size_t len, std::string path)
{
	auto filelist = std::make_shared<FileList>();
	filelist->m_path.resize(len);
	for (int i = 0; i < len; i++)
	{
		std::string filename = path;
		if (i < 9 )
			filename += "0";
		std::string suffix = ".tif";
		switch (fileType)
		{
		case ENUM_FILETYPE::FILETYPE_JPG:
			suffix = ".jpg";
			break;
		case ENUM_FILETYPE::FILETYPE_PNG:
			suffix = ".png";
			break;
		default:
			break;
		}
		filename += std::to_string(i + 1) + suffix;
		filelist->m_path[i] = filename;
	}
	filelist->outFile();
	return filelist;
}
