
#ifndef __TC_IMAGE_H__
#define __TC_IMAGE_H__
#include "TcKits/TcType.h"
#include "TcKits/TcGlobalDef.h"

struct RGBApixel_s
{
    unsigned char Red;
	unsigned char Green;
    unsigned char Blue;
	unsigned char Alpha;
};

class TCEXPORT TcImage
{
    friend class TcBitmap;
public:
    TcImage();
    virtual ~TcImage();
    TcImage(int width, int height, int Bpp);
    //TcImage坐标系原点坐标为屏幕左上角，OpenGL为屏幕左下角
public:
    static TcImage* CreateBy(TcImage* image);
    
public:
    void Create(int width, int height, int Bpp);

    bool Load(const u8String& u8strFileName);
    int GetHeight() const;
    int GetWidth() const;
    int GetBPP() const;
    int GetPitch() const;
    void* GetBits();
    
    RGBApixel_s* operator()(int i,int j);
    void GetBitMapBits(void* Buffer,bool Inversion = false) const;
    bool IsNull() const;

    void Destroy();
    void Save(const u8String& strFileName);
    
private:
    int                 m_nWidth;
    int                 m_nHeight;
    int                 m_nBpp;
    
    RGBApixel_s**       m_ppPixels;
};

#endif



*************************

#include "TcImage.h"
#include "TcKits/TcUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "TcKits/TcUtility.h"
#include "stb_image.h"
#include "stb_image_write.h"

TcImage::TcImage()
{
    m_nWidth = 1;
    m_nHeight = 1;
    m_nBpp = 24;
    m_ppPixels = new RGBApixel_s* [m_nWidth];
    m_ppPixels[0] = new RGBApixel_s [m_nHeight];
}

TcImage::TcImage(int width, int height, int Bpp)
{
    m_nWidth = width;
    m_nHeight = height;
    m_nBpp = Bpp;
    m_ppPixels = new RGBApixel_s* [m_nWidth];
    for(int i=0; i<m_nWidth; i++)
    {
        m_ppPixels[i] = new RGBApixel_s[m_nHeight];
    }
    
    for( int i=0 ; i < m_nWidth ; i++)
    {
        for( int j=0 ; j < m_nHeight ; j++ )
        {
            m_ppPixels[i][j].Red = 0;
            m_ppPixels[i][j].Green = 0;
            m_ppPixels[i][j].Blue = 0;
            m_ppPixels[i][j].Alpha = 0;
        }
    }
}

TcImage::~TcImage()
{  
	for (int i = 0; i < m_nWidth; i++)
	{
		SAFE_DELETE_ARRAY(m_ppPixels[i]);
	}
	SAFE_DELETE_ARRAY(m_ppPixels);
}

TcImage* TcImage::CreateBy(TcImage* image)
{
    int width = image->GetWidth();
    int height = image->GetHeight();
    int bpp = image->GetBPP();
    TcImage* newImage = new TcImage(width, height, bpp);
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            RGBApixel_s* oldImageInfo = image->operator()(i, j);
            RGBApixel_s* newImageInfo = newImage->operator()(i, j);
            newImageInfo->Red = oldImageInfo->Red;
            newImageInfo->Green = oldImageInfo->Green;
            newImageInfo->Blue = oldImageInfo->Blue;
            newImageInfo->Alpha = oldImageInfo->Alpha;
        }
    }
    return newImage;
}

void TcImage::Create(int width, int height, int Bpp)
{
    if (!IsNull())
    {
        Destroy();
    }

	m_nWidth = width;
	m_nHeight = height;
	m_nBpp = Bpp;
	m_ppPixels = new RGBApixel_s * [m_nWidth];
	for (int i = 0; i < m_nWidth; i++)
	{
		m_ppPixels[i] = new RGBApixel_s[m_nHeight];
	}

	for (int i = 0; i < m_nWidth; i++)
	{
		for (int j = 0; j < m_nHeight; j++)
		{
			m_ppPixels[i][j].Red = 0;
			m_ppPixels[i][j].Green = 0;
			m_ppPixels[i][j].Blue = 0;
			m_ppPixels[i][j].Alpha = 0;
		}
	}
}

bool TcImage::Load(const u8String& strFileName)
{
    string::size_type pos = strFileName.rfind(".");
    if (pos == string::npos)
    {
        return false;
    }

    string ext = strFileName.substr(pos + 1, strFileName.length());
    if (ext.length() == 0)
    {
        return false;
    }

    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

     if (ext.compare("png") == 0 || ext.compare("bmp") == 0
        || ext.compare("jpg") == 0 || ext.compare("jpeg") == 0)
     {
        int width, height, chanels;
        unsigned char* data = NULL;

        data = stbi_load(TcUtility::toLocalString(strFileName).c_str(), &width, &height, &chanels, 0);
        if (data == NULL)
            return false;
      
        for(int i=0; i<m_nWidth; i++)
        {
            SAFE_DELETE_ARRAY(m_ppPixels[i]);
        }
        SAFE_DELETE_ARRAY(m_ppPixels);

		m_nHeight = height;
		m_nWidth = width;
        m_nBpp = chanels*8;

        m_ppPixels = new RGBApixel_s* [m_nWidth];
        for(int i=0; i<m_nWidth; i++)
        {
            m_ppPixels[i] = new RGBApixel_s[m_nHeight];
        }
      
        for (int y = 0; y < m_nHeight; y++)
        {
            for (int x = 0; x < m_nWidth; x++)
            {
                m_ppPixels[x][y].Red = data[y * m_nWidth * chanels + x * chanels];
                m_ppPixels[x][y].Green = data[y * m_nWidth * chanels + x * chanels + 1];
                m_ppPixels[x][y].Blue = data[y * m_nWidth * chanels + x * chanels + 2];
                if (chanels==4)
                {
                    m_ppPixels[x][y].Alpha = data[y * m_nWidth * chanels + x * chanels + 3];
                }
                else
                {
                    m_ppPixels[x][y].Alpha = 255;
                }
            }
        }
        stbi_image_free(data);
        return true;
     } 
    return false;
}

int TcImage::GetHeight() const
{
    return m_nHeight;
}

int TcImage::GetWidth() const
{
    return m_nWidth;
}

int TcImage::GetBPP() const
{
    return m_nBpp;
}

int TcImage::GetPitch() const
{
    return (m_nBpp/8)*m_nWidth;
}

RGBApixel_s* TcImage::operator()(int i,int j)
{
    if(i >= m_nWidth)
    {
        i = m_nWidth-1;
    }
    if(i < 0)
    {
        i = 0;
    }
    
    if(j >= m_nHeight)
    {
        j = m_nHeight-1;
    }
    
    if(j < 0)
    {
        j = 0;
    }
    
    return &(m_ppPixels[i][j]);
}

void* TcImage::GetBits()
{
    return m_ppPixels;
}

void TcImage::GetBitMapBits(void* Buffer , bool Inversion) const
{
    int pixBit = m_nBpp/8;
    unsigned char* pBuf = (unsigned char*)Buffer;
    
    char colorR(0), colorG(0), colorB(0), colorA(0);
    if (Inversion)
    {
		for (int y = 0; y < m_nHeight; y++)
		{
			for (int x = 0; x < m_nWidth; x++)
			{
				colorR = m_ppPixels[x][m_nHeight - y -1].Red;
				if (pixBit == 1)
				{
					pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
				}
				else if (pixBit == 3)
				{
					colorG = m_ppPixels[x][m_nHeight - y - 1].Green;
					colorB = m_ppPixels[x][m_nHeight - y - 1].Blue;

					pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
					pBuf[y * m_nWidth * pixBit + x * pixBit + 1] = colorG;
					pBuf[y * m_nWidth * pixBit + x * pixBit + 2] = colorB;
				}
				else
				{
					colorG = m_ppPixels[x][m_nHeight - y - 1].Green;
					colorB = m_ppPixels[x][m_nHeight - y - 1].Blue;
					colorA = m_ppPixels[x][m_nHeight - y - 1].Alpha;

					pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
					pBuf[y * m_nWidth * pixBit + x * pixBit + 1] = colorG;
					pBuf[y * m_nWidth * pixBit + x * pixBit + 2] = colorB;
					pBuf[y * m_nWidth * pixBit + x * pixBit + 3] = colorA;
				}
			}
		}
    }
    else
    {
        for (int y = 0; y < m_nHeight; y++)
        {
            for (int x = 0; x < m_nWidth; x++)
            {
                colorR = m_ppPixels[x][y].Red;
                if (pixBit == 1)
                {
                    pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
                }
                else if (pixBit == 3)
                {
                    colorG = m_ppPixels[x][y].Green;
                    colorB = m_ppPixels[x][y].Blue;

                    pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
                    pBuf[y * m_nWidth * pixBit + x * pixBit + 1] = colorG;
                    pBuf[y * m_nWidth * pixBit + x * pixBit + 2] = colorB;
                }
                else
                {
                    colorG = m_ppPixels[x][y].Green;
                    colorB = m_ppPixels[x][y].Blue;
                    colorA = m_ppPixels[x][y].Alpha;

                    pBuf[y * m_nWidth * pixBit + x * pixBit] = colorR;
                    pBuf[y * m_nWidth * pixBit + x * pixBit + 1] = colorG;
                    pBuf[y * m_nWidth * pixBit + x * pixBit + 2] = colorB;
                    pBuf[y * m_nWidth * pixBit + x * pixBit + 3] = colorA;
                }
            }
        }
    }
}

bool TcImage::IsNull() const
{
    return (m_nWidth * m_nHeight * m_nBpp) == 0;
}

void TcImage::Destroy()
{
	for (int i = 0; i < m_nWidth; i++)
	{
        SAFE_DELETE_ARRAY(m_ppPixels[i]);
	}
    SAFE_DELETE_ARRAY(m_ppPixels);

    m_nWidth = 0;
    m_nHeight = 0;
    m_nBpp = 0;
}

void TcImage::Save(const u8String& strFileName)
{
	string::size_type pos = strFileName.rfind(".");
	if (pos == string::npos)
	{
		return;
	}

	string ext = strFileName.substr(pos + 1, strFileName.length());
	if (ext.length() == 0)
	{
		return;
	}
	transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    int channel = m_nBpp / 8;
	unsigned char* pixeData = new unsigned char[m_nHeight * m_nWidth * channel];
	GetBitMapBits((void*)pixeData);
    localString localStrFileName = TcUtility::toLocalString(strFileName);
	if (ext.compare("jpg") == 0 || ext.compare("jpeg") == 0)
	{
		stbi_write_jpg(localStrFileName.c_str(), m_nWidth, m_nHeight, channel, pixeData, 80);
	}
	if (ext.compare("bmp") == 0)
	{
		stbi_write_bmp(localStrFileName.c_str(), m_nWidth, m_nHeight, channel, pixeData);
	}
	if (ext.compare("png") == 0)
	{
		stbi_write_jpg(localStrFileName.c_str(), m_nWidth, m_nHeight, channel, pixeData, 0);
	}
	SAFE_DELETE_ARRAY(pixeData);
}