#ifndef __TC_COMPRESS_H__
#define __TC_COMPRESS_H__
#include "TcKits/TcType.h"
#include "TcKits/TcGlobalDef.h"
class TCEXPORT TcCompress
{
public:
	TcCompress(void);
	virtual ~TcCompress(void);
public:
	static bool unzipFile(u8String u8zipFileName, u8String u8dstDir);
	static bool unzipFile(u8String u8zipFileName, u8String u8dstDir, std::vector<u8String>& u8fileItems);
	static bool CreateZipfromDir(const u8String& u8dirpathName, const u8String& u8zipfileName);
    static int ExtractZipData(unsigned const char* pBinarySrcData, unsigned long nSrclen, unsigned char* pBinaryDestData, unsigned long& nDestLen);
    static int CompressZipData(unsigned char* pBinaryDestData, unsigned long& nDestLen, const unsigned char* pBinarySrcData, unsigned long nSrcLen, int level /* = -1 */);

	// Lzma Compress
	static int LzmaCompressFile(const u8String& u8strSrcFile, const u8String& u8strDstFile);
	static int LzmaDeCompressFile(const u8String& u8strSrcFile, const u8String& u8strDstFile);
};

#endif //__TC_COMPRESS_H__

******************************

#include "TcCompress.h"
#include "zlib.h"
#include "boost/filesystem.hpp"
#include "TcKits/Helper/TcPathHelper.h"
#include "boost/locale.hpp"
#include "LzmaLib.h"
#include "TcKits/Misc/TcFStream.h"
#include "zip.h"
#include "unzip.h"
#include "TcKits/TcUtility.h"
#include "TcKits/TcType.h"
class  TcCompressPrivate
{
private:
	TcCompressPrivate() {};
	~TcCompressPrivate() {}
public:
	static bool unzipCurrentFile(unzFile uf, const char* localstrDestFolder)
	{
		localString fileitem;
		return unzipCurrentFile(uf, localstrDestFolder, fileitem);
	}

	static bool unzipCurrentFile(unzFile uf, const char* localDestFolder,localString& localStrFileitem)
	{
		char szFilePath[512];
		unz_file_info64 FileInfo;

		if (unzGetCurrentFileInfo64(uf, &FileInfo, szFilePath, sizeof(szFilePath), NULL, 0, NULL, 0) != UNZ_OK)
			return false;

		size_t len = strlen(szFilePath);
		if (len <= 0)
		{
			return false;
		}

		boost::filesystem::path fullFileName(localDestFolder);
		fullFileName /= szFilePath;
		string strDir = TcUtility::fromLocalString(fullFileName.string());

		if (szFilePath[len - 1] == '\\' || szFilePath[len - 1] == '/')
		{
			TcPathHelper::createDirectory(strDir);
			return true;
		}

		//防止解压路径直接是文件路径而不是文件夹导致解压失败
		if (!TcPathHelper::IsDirectoryExists(TcPathHelper::GetDirectory(strDir)))
		{
			TcPathHelper::createDirectory(TcPathHelper::GetDirectory(strDir));
		}

		auto file = fopen(fullFileName.string().c_str(), "wb");

		if (file == nullptr)
		{
			return false;
		}
		localStrFileitem = szFilePath;
		if (unzOpenCurrentFile(uf) != UNZ_OK)
		{
			std::fclose(file);
			return false;
		}
		char* byBuffer = NULL;
		byBuffer = new char[FileInfo.uncompressed_size];
		memset(byBuffer, 0x00, FileInfo.uncompressed_size);
		bool bRet = true;
		while (true)
		{
			int nSize = unzReadCurrentFile(uf, byBuffer, FileInfo.uncompressed_size);

			if (nSize < 0)
			{
				bRet = false;
				break;
			}
			else if (nSize == 0)
			{
				break;
			}
			else
			{
				size_t wSize = std::fwrite(byBuffer, 1, nSize, file);
				if (wSize != nSize)
				{
					bRet = false;
					break;
				}
			}
		}
		SAFE_DELETE_ARRAY(byBuffer);
		unzCloseCurrentFile(uf);
		std::fclose(file);
		return bRet;
	}

	static bool AddfiletoZip(zipFile zfile, const localString& localstrFilePath, const localString& localstrZipParentDir)
	{
		if (NULL == zfile || localstrFilePath.empty())
		{
			return false;
		}
		int nErr = 0;
		zip_fileinfo zinfo = { 0 };
		tm_zip tmz = { 0 };
		zinfo.tmz_date = tmz;
		zinfo.dosDate = 0;
		zinfo.internal_fa = 0;
		zinfo.external_fa = 0;

		boost::filesystem::path zipDirPath(localstrZipParentDir);
		zipDirPath /= boost::filesystem::path(localstrFilePath).filename().string();

		// if the path end with "/" or "\\" ,zlib will create zipdir,so if dir ,please append the dir separator tag
		if (boost::filesystem::is_directory(localstrFilePath))
		{
			zipDirPath /= boost::filesystem::path("/").make_preferred();
		}
		// musr convert to gbk ,because the zib use assiic encode
		nErr = zipOpenNewFileInZip(zfile, boost::locale::conv::from_utf(zipDirPath.wstring().c_str(), "GBK").c_str(), &zinfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
		if (nErr != ZIP_OK)
		{
			return false;
		}
		if (!boost::filesystem::is_directory(localstrFilePath))
		{
			FILE* srcfp = fopen(localstrFilePath.c_str(), "rb");
			if (NULL == srcfp)
			{
				return false;
			}
			int numBytes = 0;
			const int filelen = 1024 * 100 * 1024;
			char* pBuf = new char[filelen];
			if (NULL == pBuf)
			{
				return false;
			}
			while (!feof(srcfp))
			{
				memset(pBuf, 0, filelen);
				numBytes = fread(pBuf, 1, filelen, srcfp);
				nErr = zipWriteInFileInZip(zfile, pBuf, numBytes);
				if (ferror(srcfp))
				{
					break;
				}
			}
			SAFE_DELETE_ARRAY(pBuf);
			std::fclose(srcfp);
		}
		zipCloseFileInZip(zfile);

		return true;
	}
	static bool CollectfileInDirtoZip(zipFile zfile, const localString& localstrFilepath, const localString& localstrZipParentDir)
	{
		boost::filesystem::path path(localstrFilepath);
		if (!boost::filesystem::exists(path))
		{
			return false;
		}
		if (boost::filesystem::is_directory(path))
		{
			AddfiletoZip(zfile, path.string(), localstrZipParentDir);
			boost::filesystem::directory_iterator end_iter;
			for (boost::filesystem::directory_iterator iter(path); iter != end_iter; iter++)
			{
				boost::filesystem::path subpath(localstrZipParentDir);
				subpath /= iter->path().parent_path().filename();
				try {
					if (boost::filesystem::is_directory(*iter))
					{
						CollectfileInDirtoZip(zfile, iter->path().string(), subpath.string());
					}
					else
					{
						AddfiletoZip(zfile, iter->path().string(), subpath.string());
					}
				}
				catch (const std::exception&)
				{
					continue;
				}
			}
		}
		else
		{
			AddfiletoZip(zfile, localstrFilepath.c_str(), localstrZipParentDir);
		}

		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
TcCompress::TcCompress(void)
{
	
}

TcCompress::~TcCompress(void)
{
}



//解压zip
bool TcCompress::unzipFile(u8String u8zipFileName, u8String u8dstDir)
{
	std::vector<u8String> u8filetiems;
	return unzipFile(u8zipFileName, u8dstDir, u8filetiems);
}
 

bool TcCompress::unzipFile(u8String u8zipFileName, u8String u8dstDir,std::vector<u8String>& u8fileItems)
{
	std::string zipFileName = TcUtility::toLocalString(u8zipFileName);
	std::string dstDir = TcUtility::toLocalString(u8dstDir);

	unzFile uf = unzOpen64(zipFileName.c_str());
	if (uf == NULL)
		return false;

	unz_global_info64 gi;
	if (unzGetGlobalInfo64(uf, &gi) != UNZ_OK)
	{
		unzClose(uf);
		return false;
	}

	for (int i = 0; i < gi.number_entry; ++i)
	{
		localString fileitem;
		if (!TcCompressPrivate::unzipCurrentFile(uf, dstDir.c_str(), fileitem))
		{
			unzClose(uf);
			return false;
		}
		u8fileItems.push_back(TcUtility::fromLocalString(fileitem));
		if (i < gi.number_entry - 1)
		{
			if (unzGoToNextFile(uf) != UNZ_OK)
			{
				unzClose(uf);
				return false;
			}
		}
	}
	unzClose(uf);
	return true;
}

bool TcCompress::CreateZipfromDir(const u8String& u8dirpathName, const u8String& u8zipfileName)
{
	localString zipfileName = TcUtility::toLocalString(u8dirpathName);
	localString dirpathName = TcUtility::toLocalString(u8zipfileName);
	zipFile newZipFile = zipOpen(zipfileName.c_str(), APPEND_STATUS_CREATE);
	if (newZipFile == NULL)
	{
		return false;
	}
	TcCompressPrivate::CollectfileInDirtoZip(newZipFile, dirpathName, "");
	zipClose(newZipFile, NULL);
	return true;
}
int TcCompress::ExtractZipData(unsigned const char* pSrcData, unsigned long nSrclen, unsigned char* pDestData, unsigned long& nDestLen)
{
	return uncompress(pDestData, &nDestLen, pSrcData, nSrclen);
}

int TcCompress::CompressZipData(unsigned char* pDestData, unsigned long& nDestLen, const unsigned char* pSrcData, unsigned long nSrcLen, int level /* = -1 */)
{
	if (!pDestData)
	{
		nDestLen = compressBound(nSrcLen);
		return Z_BUF_ERROR;
	}
	return compress2(pDestData, &nDestLen, pSrcData, nSrcLen, level);
}

int TcCompress::LzmaCompressFile(const u8String& u8strSrcFile, const u8String& u8strDstFile)
{
	localString strDstFile = TcUtility::toLocalString(u8strDstFile);
	TcFStream is(u8strSrcFile, ios::in | ios::binary);

	if (!is.IsOpen())
	{
		return -1;
	}

	size_t srcLen = is.GetLength();
	size_t destLen = srcLen * 2;
	unsigned char* psrcRead;// = new unsigned char[srcLen]; //原始文件数据
	unsigned char* pLzma = new unsigned char[destLen]; //存放压缩数据
	memset(pLzma, 0, destLen);

	char* pReadChar = new char[srcLen];
	is.Read(pReadChar, srcLen);
	psrcRead = (unsigned char*)pReadChar;

	unsigned char prop[5] =
	{
		0
	};
	size_t sizeProp = 5;

	if (SZ_OK != LzmaCompress(pLzma, &destLen, psrcRead, srcLen, prop,
		&sizeProp, 9, (1 << 24), 3, 0, 2, 32, 2))
	{
		//出错了
		delete[] pReadChar;
		delete[] pLzma;
		is.Close();
		return  -1;
	}

	ofstream compressFile(strDstFile, ios::out);
	//写入压缩后的数据
	if (!compressFile.is_open())
	{
		delete[] pReadChar;
		delete[] pLzma;
		is.Close();
		return  -1;
	}
	// 先写入prop数据和原始文件大小数据，读取时需要用到
	compressFile.write((char*)prop, sizeProp);
	compressFile.write((char*)&srcLen, 4);
	compressFile.write((char*)pLzma, destLen);

	delete[] pReadChar;
	delete[] pLzma;
	compressFile.close();
	is.Close();

	return 0;
}

int TcCompress::LzmaDeCompressFile(const u8String& u8strSrcFile, const u8String& u8strDstFile)
{
	localString strDstFile = TcUtility::toLocalString(u8strDstFile);
	TcFStream is(u8strSrcFile, ios::in | ios::binary);

	if (!is.IsOpen())
	{
		return -1;
	}

	size_t srcLen = is.GetLength();

	unsigned char prop[5] =
	{
		0
	};
	size_t sizeProp = 5;

	// 获取压缩时的prop数据
	if (!is.Read((char*)prop, sizeProp))
	{
		is.Close();
		return -1;
	}

	// 获取原始文件大小
	size_t destLen = 0;
	is.Read((char*)&destLen, 4);

	srcLen -= 9;   // 前面预留9字节非压缩文件内容，为prop参数（5）和原始文件大小（4）
	unsigned char* psrcRead;// = new unsigned char[srcLen]; //原始文件数据
	unsigned char* pDecomress = new unsigned char[destLen]; //存放解压缩数据
	memset(pDecomress, 0, destLen);

	char* pReadChar = new char[srcLen];
	memset(pReadChar, 0, srcLen);
	is.Read(pReadChar, srcLen);
	psrcRead = (unsigned char*)pReadChar;

	ofstream deCompressFile(strDstFile, ios::out|ios::binary);
	//写入解压缩后的数据
	if (!deCompressFile.is_open())
	{
		delete[] pReadChar;
		delete[] pDecomress;
		is.Close();
		return -1;
	}
	//注意：解压缩时props参数要使用压缩时生成的outProps，这样才能正常解压缩
	int nRet = LzmaUncompress(pDecomress, &destLen, psrcRead, &srcLen, prop, 5);
	if (SZ_OK != nRet)
	{
		delete[] psrcRead;
		delete[] pDecomress;
		deCompressFile.close();
		is.Close();
		return -1;
	}

	deCompressFile.write((char*)pDecomress, destLen);

	delete[] pReadChar;
	delete[] pDecomress;
	deCompressFile.close();
	is.Close();

	return 0;
}
