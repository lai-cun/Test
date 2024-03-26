TcMesh* TcReadWriteFile::ReadPLY(const u8String& u8pathName)
{
	bool binary = true;

	TcFStream is(u8pathName, ios::binary | ios::in);
	if (!is.IsOpen())
	{
		return nullptr;
	}
	string strtemp;
	while(1)
	{
		strtemp.clear();
		is.GetLine(strtemp);
		if ((int)strtemp.find("end_header")==0)
		{
			break;
		}
		if ((int)(strtemp.find("format"))==0)
		{
			if ((int)strtemp.find("binary")>0)
			{
				binary = true;
			}
			else if((int)strtemp.find("ascii")>0)
			{
				binary = false;
			}
			break;
		}
	}
	vector<TcPoint> vtxs;
	vector<TcColor> vtxColors;
	vector<Triangle> faces;
	vector<vector<float>>texCoords;
	if (!binary)
	{
		is.Close();
		ReadPLYAscii(u8pathName, vtxs, vtxColors, faces);
	}
	else
	{
		is.Close();
		ReadPLYBinary(u8pathName, vtxs, vtxColors, faces, texCoords);
	}

	TcMesh* pMesh = new TcMesh();
	pMesh->AllocateMemory((int)vtxs.size(), (int)faces.size());
	pMesh->SetVtxColor(vtxColors);
	pMesh->SetVtxTexCrood(texCoords);
	for (int i = 0; i < (int)vtxs.size(); i++)
	{
		pMesh->AddVertex(vtxs[i]);
	}
	for (int i = 0; i < (int)faces.size(); i++)
	{
		pMesh->AddFace(faces[i].v);
	}

	pMesh->CalculateFaceNormal();

	return pMesh;
}

bool TcReadWriteFile::ReadPLYAscii(const string& pathName,vector<TcPoint>& vecpoint,vector<TcColor>& veccolor,vector<Triangle>& vecface)
{
	int vertexNum;
	vector<int>count;
	vector<PlyEnum>propertyType;
	vector<PlyEnum>elementType;
	int faceNum;

	fstream fopen;
	fopen.open(TcUtility::toLocalString(pathName),ios::in);
		
	if(!fopen.is_open())
	{
		fopen.close();
		return false;
	}
	string filein;

	while(!fopen.eof())
	{
		fopen>>filein;
		if(filein=="ply"||filein=="PLY")
		{	
		}else if(filein=="comment")
		{	
			getline(fopen,filein,'\n');
		}else if(filein=="format")
		{	
			getline(fopen,filein,'\n');
		}else if(filein=="element")
		{	
			fopen>>filein;
			if(filein=="vertex")
			{	
				fopen>>vertexNum;
			}else if(filein=="face")
			{	
				fopen>>faceNum;
				
				getline(fopen,filein,'\n');
				getline(fopen,filein,'\n');
			}else
			{	//自定义类型目前忽略
				getline(fopen,filein,'\n');
			}
		}
		else if(filein=="property")
		{	//property, xyz/nxyz/rgb
			fopen>>filein;
			if(filein=="char"||filein=="int8")
			{
				propertyType.push_back(PLY_BYTE);
			}
			else if(filein=="uchar"||filein=="uint8")
			{
				propertyType.push_back(PLY_UNSIGNED_BYTE);
			}
			else if(filein=="short"||filein=="int16")
			{
				propertyType.push_back(PLY_SHORT);
			}
			else if(filein=="ushort"||filein=="uint16")
			{
				propertyType.push_back(PLY_UNSIGNED_SHORT);
			}
			else if(filein=="int"||filein=="int32")
			{
				propertyType.push_back(PLY_INT);
			}
			else if(filein=="uint"||filein=="uint32")
			{
				propertyType.push_back(PLY_UNSIGNED_INT);
			}
			else if(filein=="float"||filein=="float32")
			{
				propertyType.push_back(PLY_FLOAT);
			}
			else if(filein=="double"||filein=="float64")
			{
				propertyType.push_back(PLY_DOUBLE);
			}else
			{	
				propertyType.push_back(PLY_DOUBLE);
			}
			fopen>>filein;
			if(filein=="x")
			{
				elementType.push_back(PLY_VERTEX_ARRAY);
				getline(fopen,filein,'\n');//x
				getline(fopen,filein,'\n');//y
				count.push_back(2);
			}else if(filein=="nx")
			{
				elementType.push_back(PLY_NORMAL_ARRAY);
				getline(fopen,filein,'\n');//nx
				getline(fopen,filein,'\n');//ny
				getline(fopen,filein,'\n');//nz
				count.push_back(3);
			}else if(filein=="red")
			{
				elementType.push_back(PLY_COLOR_ARRAY);
				getline(fopen,filein,'\n');//red
				getline(fopen,filein,'\n');//green
				getline(fopen,filein,'\n');//blue
				count.push_back(3);
			}else if(filein=="z")
			{
				count[count.size()-1]+=1;//3维坐标
				propertyType.pop_back();
			}
		}
		else if(filein=="end_header")
		{	
			if(count.size()!=propertyType.size()||count.size()!=elementType.size())
			{
				fopen.close();
				return false;
			}
			
			vecpoint.reserve(vertexNum);
			veccolor.reserve(vertexNum);
			vecface.reserve(faceNum);

			float x,y,z,nx,ny,nz;
			unsigned char red,green,blue;
			string strred,strgreen,strblue;
			for(int i=0;i<vertexNum;i++)//接下来的vertexNum行都是顶点信息
			{
				for(size_t j=0;j<count.size();j++)
				{
					switch (elementType[j])
					{
					case PLY_VERTEX_ARRAY:
						fopen>>x>>y>>z;
						break;
					case PLY_NORMAL_ARRAY:
						fopen>>nx>>ny>>nz;
						break;
					case  PLY_COLOR_ARRAY:
						fopen>>strred>>strgreen>>strblue;
						break;
					}
				}
				red = atoi(strred.c_str());
				green = atoi(strgreen.c_str());
				blue = atoi(strblue.c_str());
				getline(fopen,filein,'\n');
				
				vecpoint.push_back(TcPoint(x,y,z));

				veccolor.push_back(TcColor(red,green,blue));
			}
			//read face information
			int front=0,index=0;;
			fopen>>front;
			
			while(front==3 && index<faceNum)
			{
				Triangle tringle;
				fopen>>tringle.v[0]>>tringle.v[1]>>tringle.v[2];
				vecface.push_back(tringle);
				getline(fopen,filein,'\n');
				fopen>>front;
				index++;
			}
		}else
		{
			continue;
		}
	}
	fopen.close();
	return true;
}

bool TcReadWriteFile::ReadPLYBinary(const u8String& u8pathName, vector<TcPoint>& points, vector<TcColor>& vtxColors, vector<Triangle>& faces,  vector<vector<float>>& vectexCoords)
{
	TcFStream is(u8pathName, ios::binary | ios::in);
	is.SetIndex(m_nProgressIndex);
	if (!is.IsOpen())
	{
		return false;
	}
	vector<string> vtxDataType;
	vector<string> faceDataType;
	string strtemp;

	int vtxNum = 0;
	int faceNum = 0;

	while(1)
	{
		strtemp.clear();
		is.GetLine(strtemp);
		if ((int)strtemp.find("end_header")==0)
		{
			break;
		}

		if ((int)strtemp.find("element")==0)
		{
			if ((int)strtemp.find("vertex")>0)
			{
				int ipos = (int)strtemp.rfind(' ');
				assert(ipos>0);

				vtxNum = atoi((strtemp.substr(ipos+1,strtemp.size() - ipos -1)).c_str());
				strtemp.clear();
				is.GetLine(strtemp);
				while (strtemp.find("property")==0)
				{
					ipos = (int)strtemp.rfind(' ');
					vtxDataType.push_back(strtemp.substr(ipos+1,(int)strtemp.size() - ipos -1));
					streampos postemp = is.Tellg();
					is.GetLine(strtemp);
					if (strtemp.find("element")==0)
					{
						is.Seekg(postemp);
						break;
					}
				}
			}
			else if ((int)strtemp.find("face")>0)
			{
				int ipos = (int)strtemp.rfind(' ');
				assert(ipos>0);

				faceNum = atoi((strtemp.substr(ipos+1,strtemp.size() - ipos -1)).c_str());
				strtemp.clear();
				is.GetLine(strtemp);
				while (strtemp.find("property")==0)
				{
					ipos = (int)strtemp.rfind(' ');
					faceDataType.push_back(strtemp.substr(ipos+1,(int)strtemp.size() - ipos -1));
					streampos postemp = is.Tellg();
					is.GetLine(strtemp);
					if (strtemp.find("element")==0 || strtemp.find("end_header")==0)
					{
						is.Seekg(postemp);
						break;
					}
				}
			}
		}
	}

	points.reserve(vtxNum);
	vtxColors.reserve(vtxNum);
	faces.reserve(faceNum);

	float x,y,z,nx,ny,nz;
	unsigned char r = '0', g ='0', b = '0', a = '0';

	for (int i = 0; i < vtxNum; i++)
	{
		for (int j = 0; j < (int)vtxDataType.size(); j++)
		{
			if (vtxDataType[j].find("x") == 0)
			{
				is.Read((char*)&x, sizeof(float));
			}
			else if (vtxDataType[j].find("y") == 0)
			{
				is.Read((char*)&y,sizeof(float));
			}
			else if (vtxDataType[j].find("z") == 0)
			{
				is.Read((char*)&z,sizeof(float));
			}
			else if (vtxDataType[j].find("nx") == 0)
			{
				is.Read((char*)&nx,sizeof(float));
			}
			else if (vtxDataType[j].find("ny") == 0)
			{
				is.Read((char*)&ny,sizeof(float));
			}
			else if (vtxDataType[j].find("nz") == 0)
			{
				is.Read((char*)&nz,sizeof(float));
			}
			else if (vtxDataType[j].find("red") == 0)
			{
				is.Read((char*)&r,sizeof(unsigned char));
			}
			else if (vtxDataType[j].find("green") == 0)
			{
				is.Read((char*)&g,sizeof(unsigned char));
			}
			else if (vtxDataType[j].find("blue") == 0)
			{
				is.Read((char*)&b,sizeof(unsigned char));
			}
			else if (vtxDataType[j].find("alpha") == 0)
			{
				is.Read((char*)&a,sizeof(unsigned char));
			}
		}

		points.push_back(TcPoint(x, y, z));	
		if (r != '0' || g != '0' || b != '0' || a != '0')
		{
			vtxColors.push_back(TcColor(r, g, b));
		}
	}

	Triangle tringle;
	std::uint8_t numTemp;
	int obj;
	int vId;
	for (int i = 0; i < faceNum; i++)
	{
		for (int j = 0; j < (int)faceDataType.size(); j++)
		{
			if (faceDataType[j].find("object") == 0)
			{
				is.Read((char*)&obj, sizeof(int));
			}
			else if (faceDataType[j].find("texcoord") == 0)
			{
				is.Read((char*)&numTemp, sizeof(std::uint8_t));
				assert(numTemp == 6);
				for (int j = 0; j < 3; j++)
				{
					vector<float> crood;
					for (int k = 0; k< 2;k++)
					{
						float vCrood;
						is.Read((char*)&vCrood, sizeof(float));
						crood.push_back(vCrood);
					}
					vectexCoords.push_back(crood);
				}
			}
			else if(faceDataType[j].find("vertex_indices") == 0)
			{
				is.Read((char*)&numTemp, sizeof(std::uint8_t));

				assert(numTemp == 3);
				for (int j = 0; j < 3; j++)
				{
					is.Read((char*)&vId, sizeof(int));
					tringle.v[j] = vId;
				}

				faces.emplace_back(tringle);
			}
		}
	}

	//assert(is.GetPos() == is.GetLength());
	is.Close();
	return true;
}

TcMesh* TcReadWriteFile::ReadOBJ(const u8String& u8pathName)
{
	TcFStream is(u8pathName, ios::binary | ios::in);
	if (!is.IsOpen())
	{
		return nullptr;
	}

	vector<TcPoint> vecpoint;
	vector<TcColor> vtxColor;
	vector<Triangle> vecface;
	map<string,vector<Triangle>> mapColorFace;

	vector<Triangle> vecfacetemp;
	string strtemp;

	int vertexNum = 0;
	int faceNum = 0;
	string strMtl;
	string strColor;
	while(1)
	{
		strtemp.clear();
		is.GetLine(strtemp);
		vector<string> vecResult;

		if ((int)strtemp.find("mtllib")==0)
		{
			vecResult.clear();
			TcUtility::SplitString(strtemp, ' ', vecResult);
			if ((int)vecResult.size() > 0)
			{
				strMtl = vecResult[1];
			}
		}
		else if ((int)strtemp.find("usemtl")==0)
		{
			vecResult.clear();
			TcUtility::SplitString(strtemp, ' ', vecResult);
			if ((int)vecResult.size() > 0)
			{
				strColor = vecResult[1];
			}
			vecfacetemp.clear();
		}
		else if( (int)strtemp.length() == 0)
		{
			is.Close();
			break;
		}
		else
		{
			vecResult.clear();
			TcUtility::SplitString(strtemp, ' ', vecResult);
			if ((int)vecResult.size() > 0)
			{
				float x,y,z;
				unsigned char red,green,blue;
				string strred,strgreen,strblue;

				if (vecResult[0].compare("v") == 0)
				{
					if (vecResult.size() >= 4)
					{
						x = atof(vecResult[1].c_str());
						y = atof(vecResult[2].c_str());
						z = atof(vecResult[3].c_str());
					}
					if (vecResult.size() >= 7)
					{
						red = atoi(vecResult[4].c_str());
						green = atoi(vecResult[5].c_str());
						blue = atoi(vecResult[6].c_str());
						vtxColor.push_back(TcColor(red,green,blue));
					}

					vecpoint.push_back(TcPoint(x,y,z));
				}
				else if(vecResult[0].compare("f") == 0)
				{
					Triangle tringle;
					tringle.v[0] = atoi(vecResult[1].c_str()) - 1;
					tringle.v[1] = atoi(vecResult[2].c_str()) - 1;
					tringle.v[2] = atoi(vecResult[3].c_str()) - 1;

					if(strColor.length() > 0)
					{
						vecfacetemp.push_back(tringle);
						mapColorFace[strColor] = vecfacetemp;
					}

					vecface.push_back(tringle);

				}
			}
		}	
	}

	map<string,TcColor> mapColor;
	if (vtxColor.size() == 0)
	{
		vtxColor.resize(vecpoint.size());
		int ipos = u8pathName.rfind('\\');
		if (ipos == std::string::npos)
			ipos = u8pathName.rfind('/');
		string strPath = u8pathName.substr(0, ipos) + "/" + strMtl;
		TcFStream isMtl(strPath, ios::binary | ios::in);
		if (isMtl.IsOpen())
		{
			while(1)
			{
				vector<string> vecResult;
				strtemp.clear();
				isMtl.GetLine(strtemp);
				for (map<string,vector<Triangle>>::iterator iter = mapColorFace.begin();iter != mapColorFace.end();++iter)
				{
					if (strtemp.find(iter->first)>= 0)
					{
						strtemp.clear();
						isMtl.GetLine(strtemp);
						vecResult.clear();
						TcUtility::SplitString(strtemp,' ',vecResult);
						if ((int)vecResult.size()>0)
						{
							if (vecResult[0].compare("Kd") == 0)
							{
								for (int i = 0; i < (int)vecpoint.size(); ++i)
								{
									int red = (int)(atoi(vecResult[1].c_str()) * 255);
									int green = (int)(atoi(vecResult[2].c_str()) * 255);
									int blue = (int)(atoi(vecResult[3].c_str()) * 255);
									mapColor[iter->first] = TcColor(red,green,blue);
								}
							}
						}
					}

				}
				if (strtemp.length() == 0)
				{
					isMtl.Close();
					break;
				}
			}
		}
	}

	TcMesh* pMesh = new TcMesh();
	pMesh->AllocateMemory((int)vecpoint.size(), (int)vecface.size());

	for (map<string,TcColor>::iterator itercolor = mapColor.begin();itercolor != mapColor.end();++itercolor)
	{
		for (map<string,vector<Triangle>>::iterator iter = mapColorFace.begin();iter != mapColorFace.end();++iter)
		{
			if (itercolor->first.compare(iter->first) == 0)
			{
				for (int i = 0; i < (int)iter->second.size(); i++)
				{
					int index0 = iter->second[i].v[0];
					int index1 = iter->second[i].v[1];
					int index2 = iter->second[i].v[2];
					vtxColor[index0] = itercolor->second;
					vtxColor[index1] = itercolor->second;
					vtxColor[index2] = itercolor->second;
				}
			}
		}
	}

	pMesh->SetVtxColor(vtxColor);
	for (int i = 0; i < (int)vecpoint.size(); i++)
	{
		pMesh->AddVertex(vecpoint[i]);
	}

	for (int i = 0; i < (int)vecface.size(); i++)
	{
		pMesh->AddFace(vecface[i].v);
	}

	pMesh->CalculateFaceNormal();
	return pMesh;
}

