#pragma once
#include <string>
#include <vector>

class FileList
{
public:

	enum class ENUM_FILETYPE
	{
		FILETYPE_TIFF,
		FILETYPE_JPG,
		FILETYPE_PNG,
	};
	FileList();
	~FileList();

public:
	std::string getIndexPath(int n);
	size_t getCount();
	void delIndexPath(int n);
	void outFile();
	static std::shared_ptr<FileList> NewFileList(ENUM_FILETYPE fileType, size_t len, std::string path);
private:
	std::vector<std::string> m_path;
};

