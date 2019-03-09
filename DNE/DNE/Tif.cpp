#include "Tif.h"

TTif::TTif()
{
	ByteOrder=0;   Version=0;   BitsPerSample=0;   PixelBytes=0;   SampleFormat=0;
	Compression=1;   Photometric=1;   Planar=1;   FillOrder=1;   Orientation=1;   SamplesPerPixel=1;   Nodata=0;
	memset(&IFH, 0, sizeof(IFH));   memset(&BIFH, 0, sizeof(BIFH));   DE=NULL;
	memset(&TS, 0, sizeof(TS));   memset(&GKH, 0, sizeof(GKE));   GKE=NULL;
	DECount=0;   NextIFDOffset=0;   GKECount=0;   GKH.KeyDirVersion=1;   GKH.KeyRevision=1;
	FileOffset=0;   Vmin=0;   Vmax=0;   Vmean=0;   Vstd=0;
	GeoDoubleNum=1;  GeoDouble=new(double[1]);   GeoDouble[0]=0;
	GeoNoData=NULL;   GeoAscii=NULL;   GeoMetaData=NULL;
};

TTif::~TTif()
{

};

//1. Read Tif File
bool TTif::DEM_ReadTif(TDEM & DEM)
{
	UInt32		Loop, *Tmp32;
	RowCol		i, j, ii, jj;
	Int32		Err;
	UInt16		*Tmp16;
	UInt64		ist, ied, jst, jed, k;
	float		TmpF;
	double		size;

	fTif.open(DEM.Param.DEMPath, ios::binary|ios::in);
	while (!fTif)
	{
		cout<<"  Can't Open "<<DEM.Param.DEMPath<<", Close Program Occupied and Retry!>\n";   cin.get();
		fTif.open(DEM.Param.DEMPath, ios::binary|ios::in);
	}
	fTif.read((char*)&IFH, sizeof(sTifIFH));   ByteOrder=IFH.ByteOrder;   Version=IFH.Version;
	if (ByteOrder!=0x4949)
	{
		cout<<"  Tif is Not Little-Endian Byte Order!\n\n";   return false;
	}
	if (Version==42)
	{
		fTif.seekg(IFH.IFDOffset);   DECount=0;   fTif.read((char*)&DECount, 2);
		DE=new(sBTifDE[(size_t)DECount]);   memset(DE, 0, (size_t)DECount*sizeof(sBTifDE));
		for(Loop=0;Loop<DECount;Loop++)
		{
			fTif.read((char*)&DE[Loop].TagId, 2);   fTif.read((char*)&DE[Loop].TagType, 2);
			fTif.read((char*)&DE[Loop].TagLength, 4);   fTif.read((char*)&DE[Loop].ValueOffset, 4);
		}
		fTif.read((char*)&NextIFDOffset, 4);
	}
	else if (Version==43)
	{
		fTif.seekg(0);   fTif.read((char*)&BIFH, sizeof(sBTifIFH));
		fTif.seekg(BIFH.IFDOffset);   fTif.read((char*)&DECount, 8);
		DE=new(sBTifDE[(size_t)DECount]);   memset(DE, 0, (size_t)DECount*sizeof(sBTifDE));
		for(Loop=0;Loop<DECount;Loop++)
			fTif.read((char*)&DE[Loop], sizeof(sBTifDE));
		fTif.read((char*)&NextIFDOffset, 8);
	}
	for (Loop=0;Loop<DECount;Loop++)
	{
		Err=SetTag(DEM, Loop);
		if (Err==-1)
		{
			cout<<"  Read Request Exceed TagLength!\n\n";   return false;
		}
		else if (Err==-2)
		{
			cout<<"  Read Assign Exceed Var Length!\n\n";   return false;
		}
		else if (Err==-3)
		{
			cout<<"  Read Value Exceed Var ByteSize!\n\n";   return false;
		}
	}
	PixelBytes=BitsPerSample*SamplesPerPixel/8;
	if (Compression!=1)
	{
		cout<<"  Tif Compression Must be 1 = No Compression!\n\n";   return false;
	}
	else if (Photometric!=1)
	{
		cout<<"  Tif Photometric Must be 1 = Gray Image!\n\n";   return false;
	}
	else if (Orientation!=1)
	{
		cout<<"  Tif Orientation Must be 1 = LeftTop to RightBottom!\n\n";   return false;
	}
	else if (Planar!=1)
	{
		cout<<"  Tif Planar Must be 1!\n\n";   return false;
	}
	else if (FillOrder!=1)
	{
		cout<<"  Tif FillOrder Must be 1 = Little-Endian!\n\n";   return false;
	}
	else if (!(PixelBytes==2&&(SampleFormat==1||SampleFormat==2) || PixelBytes==4&&SampleFormat==3))
	{
		cout<<"  Tif PixelBytes and SampleFormat Error! (Float32/Int16/UInt16)\n\n";   return false;
	}
	else if (TS.TileStrips<=0)
	{
		cout<<"  Tif Tiles or Strips PerImage is Null!\n\n";   return false;
	}
	cout<<"  Original DEM Scale "<<DEM.Row<<" * "<<DEM.Col<<", Read DEM and Allocate Cells...      ";
	DEM.Param.RunNow=CTime::GetCurrentTime();
	DEM.Map.assign( DEM.Row+2, vector<CellId>(DEM.Col+2, -1) );
	DEM.Cell.reserve(DEM.Row * DEM.Col);
	Tmp16=new(UInt16[(size_t)TS.ByteCounts[0]]);   Tmp32=new(UInt32[(size_t)TS.ByteCounts[0]]);
	for (Loop=0, i=0;i<TS.Down;i++)
		for (j=0;j<TS.Across;j++)
		{
			fTif.seekg(TS.Offsets[Loop]);
			if (SampleFormat==3) fTif.read((char*)Tmp32, TS.ByteCounts[Loop]);
			else fTif.read((char*)Tmp16, TS.ByteCounts[Loop]);
			ist=i*TS.Length+1;   ied=min(DEM.Row, (i+1)*TS.Length);
			jst=j*TS.Width+1;    jed=min(DEM.Col, (j+1)*TS.Width);
			for(ii=(RowCol)ist; ii<=ied; ii++)
				for(jj=(RowCol)jst; jj<=jed; jj++)
				{
					k=(ii-ist)*TS.Width+(jj-jst);
					if (SampleFormat==3)  TmpF=*(float*)&Tmp32[k];
					else if (SampleFormat==2)  TmpF=(Int16)Tmp16[k];
					else TmpF=Tmp16[k];
					if (fabs(TmpF-Nodata)<1e-4 || fabs(TmpF)<0.1)  continue;
					DEM.Map[ii][jj]= ++DEM.AllArea;
					DEM.Cell.push_back(sCell(ii, jj, TmpF));
				}
			Loop++;
			if ((UInt64)(Loop-1)*100/TS.TileStrips<(UInt64)Loop*100/TS.TileStrips)
				cout<<"\b\b\b\b"<<setw(3)<<(UInt64)Loop*100/TS.TileStrips<<"%";
		}
	DEM.Cell.shrink_to_fit();
	delete []Tmp16;   delete []Tmp32;   delete []TS.Offsets;   delete []TS.ByteCounts;   delete []DE;
	DEM.Param.ReadSpan=CTime::GetCurrentTime()-DEM.Param.RunNow;
	cout<<"  Time="<<DEM.Param.ReadSpan.Format("%#H:%#M:%#S.\n\n");
	size=sizeof(sCell)/1024.0/1024.0*DEM.AllArea+sizeof(CellId)*DEM.Row/1024.0*DEM.Col/1024.0;
	cout<<"  Input DEM has "<<DEM.AllArea<<" Cells, Percent "<<DEM.AllArea*100/DEM.Row/DEM.Col<<"%, Storage Memory "<<int(size)<<" Mb.\n\n";
	fTif.close();   return true;
};

//1.Set Tag by TagId
Int32 TTif::SetTag(TDEM & DEM, UInt32 Loop)
{
	Int32		Err=0;
	switch (DE[Loop].TagId)
	{
	case 256:
		Err=GetBuffValue(&DEM.Col, sizeof(DEM.Col), 1, Loop, 0, 1);   break;
	case 257:
		Err=GetBuffValue(&DEM.Row, sizeof(DEM.Row), 1, Loop, 0, 1);   break;
	case 258:
		Err=GetBuffValue(&BitsPerSample, sizeof(BitsPerSample), 1, Loop, 0, 1);   break;
	case 259:
		Err=GetBuffValue(&Compression, sizeof(Compression), 1, Loop, 0, 1);   break;
	case 262:
		Err=GetBuffValue(&Photometric, sizeof(Photometric), 1, Loop, 0, 1);   break;
	case 266:
		Err=GetBuffValue(&FillOrder, sizeof(FillOrder), 1, Loop, 0, 1);   break;
	case 273:   //Strip.Offsets
		TS.TileStrips=DE[Loop].TagLength;   TS.Offsets=new(UInt64[(size_t)TS.TileStrips]);
		Err=GetBuffValue(TS.Offsets, 8, TS.TileStrips, Loop, 0, DE[Loop].TagLength);   break;
	case 274:
		Err=GetBuffValue(&Orientation, sizeof(Orientation), 1, Loop, 0, 1);   break;
	case 277:
		Err=GetBuffValue(&SamplesPerPixel, sizeof(SamplesPerPixel), 1, Loop, 0, 1);   break;
	case 278:   //Strip.RowsPerStrip
		if (Err=GetBuffValue(&TS.Length, sizeof(TS.Length), 1, Loop, 0, 1) !=0)   break;
		TS.Down=(DEM.Row+TS.Length-1)/TS.Length;  TS.Width=DEM.Col; TS.Across=1;   break;
	case 279:   //Strip.ByteCounts
		TS.TileStrips=DE[Loop].TagLength;   TS.ByteCounts=new(UInt64[(size_t)TS.TileStrips]);
		Err=GetBuffValue(TS.ByteCounts, 8, TS.TileStrips, Loop, 0, DE[Loop].TagLength);   break;
	case 284:
		Err=GetBuffValue(&Planar, sizeof(Planar), 1, Loop, 0, 1);   break;
	case 322:   //Tile.Width
		if (Err=GetBuffValue(&TS.Width, sizeof(TS.Width), 1, Loop, 0, 1) !=0)   break;
		TS.Across=(DEM.Col+TS.Width-1)/TS.Width;   break;
	case 323:   //Tile.Length
		if (Err=GetBuffValue(&TS.Length, sizeof(TS.Length), 1, Loop, 0, 1) !=0)   break;
		TS.Down=(DEM.Row+TS.Length-1)/TS.Length;   break;
	case 324:   //Tile.Offsets
		TS.TileStrips=DE[Loop].TagLength;   TS.Offsets=new(UInt64[(size_t)TS.TileStrips]);
		Err=GetBuffValue(TS.Offsets, 8, TS.TileStrips, Loop, 0, DE[Loop].TagLength);   break;
	case 325:   //Tile.ByteCounts
		TS.TileStrips=DE[Loop].TagLength;   TS.ByteCounts=new(UInt64[(size_t)TS.TileStrips]);
		Err=GetBuffValue(TS.ByteCounts, 8, TS.TileStrips, Loop, 0, DE[Loop].TagLength);   break;
	case 339:
		Err=GetBuffValue(&SampleFormat, sizeof(SampleFormat), 1, Loop, 0, 1);   break;
	case 33550:
		Err=GetBuffValue(DEM.Param.GeoPixelScale, 8, 3, Loop, 0, 3);   break;
	case 33922:
		Err=GetBuffValue(DEM.Param.GeoTiePoint, 8, 6, Loop, 0, 6);   break;
	case 34735:
		if (Err=GetBuffValue(&GKH, 2, 4, Loop, 0, 4) !=0)   break;
		GKECount=GKH.NumberOfKeys;   GKE=new(sGKE[GKECount]);
		if (Err=GetBuffValue(GKE, 2, 4*GKECount, Loop, 4, 4*GKECount) !=0)   break;
		for(UInt16 s=0;s<GKECount;s++)
			if (GKE[s].KeyId==1024)   DEM.Param.GeoModel=GKE[s].ValueOffset;
			else if (GKE[s].KeyId==2048) DEM.Param.GeoCoordinate=GKE[s].ValueOffset;
		break;
	case 34736:
		GeoDoubleNum=DE[Loop].TagLength;   GeoDouble=new(double[(size_t)GeoDoubleNum]);
		Err=GetBuffValue(GeoDouble, 8, GeoDoubleNum, Loop, 0, DE[Loop].TagLength);   break;
	case 34737:
		GeoAscii=new(char[(size_t)DE[Loop].TagLength+1]);   GeoAscii[DE[Loop].TagLength]='\0';
		Err=GetBuffValue(GeoAscii, 1, DE[Loop].TagLength, Loop, 0, DE[Loop].TagLength);   break;
	case 42113:
		GeoNoData=new(char[(size_t)DE[Loop].TagLength+1]);   GeoNoData[DE[Loop].TagLength]='\0';
		Err=GetBuffValue(GeoNoData, 1, DE[Loop].TagLength, Loop, 0, DE[Loop].TagLength);
		if (Err==0)   Nodata=(float)atof(GeoNoData);   break;
	default:
		Err=0;
	}
	return Err;
};

//1.Assign Buff Value to Tag
Int32 TTif::GetBuffValue(void * Tmp, UInt32 Byte, UInt64 Len, UInt32 Id, UInt32 Offset, UInt64 Count)
{
	UInt64		*TmpLL, *Tmp64 = NULL, i, bit, TmpLen;
	UInt32		*TmpL, *Tmp32 = NULL;
	UInt16		*TmpS, *Tmp16 = NULL;
	UInt8		*TmpC, *Tmp8 = NULL;

	if ((Offset+Count)>DE[Id].TagLength) return -1;
	if (Count>Len) return -2;
	TmpLen=max(8, Offset+Count);
	switch (DE[Id].TagType)
	{
	case 1: case 2: case 6: case 7:
		bit=8;   Tmp8=new(UInt8[(size_t)TmpLen]);   break;
	case 3: case 8:
		bit=16;   Tmp16=new(UInt16[(size_t)TmpLen]);   if (Byte<2) return -3;   break;
	case 4: case 9: case 11:
		bit=32;   Tmp32=new(UInt32[(size_t)TmpLen]);   if (Byte<4) return -3;   break;
	case 5: case 10: case 12: case 16: case 17: case 18:
		bit=64;   Tmp64=new(UInt64[(size_t)TmpLen]);   if (Byte<8) return -3;   break;
	}

	if (Version==42 && bit*DE[Id].TagLength<=4*8 || Version==43 && bit*DE[Id].TagLength<=8*8)
	{
		switch (bit)
		{
		case 8:
			Tmp8[0]=(DE[Id].ValueOffset>> 0) & 0xFF;   Tmp8[1]=(DE[Id].ValueOffset>> 8) & 0xFF;
			Tmp8[2]=(DE[Id].ValueOffset>>16) & 0xFF;   Tmp8[3]=(DE[Id].ValueOffset>>24) & 0xFF;
			Tmp8[4]=(DE[Id].ValueOffset>>32) & 0xFF;   Tmp8[5]=(DE[Id].ValueOffset>>40) & 0xFF;
			Tmp8[6]=(DE[Id].ValueOffset>>48) & 0xFF;   Tmp8[7]=(DE[Id].ValueOffset>>56) & 0xFF;   break;
		case 16:
			Tmp16[0]=(DE[Id].ValueOffset>> 0) & 0xFFFF;   Tmp16[1]=(DE[Id].ValueOffset>>16) & 0xFFFF;
			Tmp16[2]=(DE[Id].ValueOffset>>32) & 0xFFFF;   Tmp16[3]=(DE[Id].ValueOffset>>48) & 0xFFFF;   break;
		case 32:
			Tmp32[0]=(UInt32)(DE[Id].ValueOffset);   Tmp32[1]=(UInt32)(DE[Id].ValueOffset>>32);   break;
		case 64:
			Tmp64[0]=DE[Id].ValueOffset;   break;
		}
	}
	else
	{
		fTif.seekg(DE[Id].ValueOffset);
		switch (bit)
		{
		case 8:   fTif.read((char*)Tmp8, Count+Offset);   break;
		case 16:   fTif.read((char*)Tmp16, 2*(Count+Offset));   break;
		case 32:   fTif.read((char*)Tmp32, 4*(Count+Offset));   break;
		case 64:   fTif.read((char*)Tmp64, 8*(Count+Offset));   break;
		}
	}
	switch (bit)
	{
	case 8:
		if (Byte==1)
		{
			TmpC=(UInt8 *)Tmp;   for (i=Count; i<Len; i++) TmpC[i]=0;
			for (i=0; i<Count; i++) TmpC[i]=Tmp8[i+Offset];
		}
		else if (Byte==2)
		{
			TmpS=(UInt16 *)Tmp;   for (i=Count; i<Len; i++) TmpS[i]=0;
			for (i=0; i<Count; i++) TmpS[i]=Tmp8[i+Offset];
		}
		else if (Byte==4)
		{
			TmpL=(UInt32 *)Tmp;   for (i=Count; i<Len; i++) TmpL[i]=0;
			for (i=0; i<Count; i++) TmpL[i]=Tmp8[i+Offset];
		}
		else if (Byte==8)
		{
			TmpLL=(UInt64 *)Tmp;   for (i=Count; i<Len; i++) TmpLL[i]=0;
			for (i=0; i<Count; i++) TmpLL[i]=Tmp8[i+Offset];
		}
		delete []Tmp8;   break;
	case 16:
		if (Byte==2)
		{
			TmpS=(UInt16 *)Tmp;   for (i=Count; i<Len; i++) TmpS[i]=0;
			for (i=0; i<Count; i++) TmpS[i]=Tmp16[i+Offset];
		}
		else if (Byte==4)
		{
			TmpL=(UInt32 *)Tmp;   for (i=Count; i<Len; i++) TmpL[i]=0;
			for (i=0; i<Count; i++) TmpL[i]=Tmp16[i+Offset];
		}
		else if (Byte==8)
		{
			TmpLL=(UInt64 *)Tmp;   for (i=Count; i<Len; i++) TmpLL[i]=0;
			for (i=0; i<Count; i++) TmpLL[i]=Tmp16[i+Offset];
		}
		delete []Tmp16;   break;
	case 32:
		if (Byte==4)
		{
			TmpL=(UInt32 *)Tmp;   for (i=Count; i<Len; i++) TmpL[i]=0;
			for (i=0; i<Count; i++) TmpL[i]=Tmp32[i+Offset];
		}
		else if (Byte==8)
		{
			TmpLL=(UInt64 *)Tmp;   for (i=Count; i<Len; i++) TmpLL[i]=0;
			for (i=0; i<Count; i++) TmpLL[i]=Tmp32[i+Offset];
		}
		delete []Tmp32;   break;
	case 64:
		if (Byte==8)
		{
			TmpLL=(UInt64 *)Tmp;   for (i=Count; i<Len; i++) TmpLL[i]=0;
			for (i=0; i<Count; i++) TmpLL[i]=Tmp64[i+Offset];
		}
		delete []Tmp64;   break;
	}
	return 0;
};

//2.Output Tif Result
bool TTif::DEM_PrintTif(TDEM & DEM, CString Kind)
{
	string		TifName;
	CellId		Now;
	RowCol		i, j, r, c;
	UInt32		Count, TmpL;
	float		TmpF;
	TifName=string(DEM.Param.FileName)+"_"+string((LPCSTR)Kind)+".Tif";
	fTif.open(string(DEM.Param.OutPath)+TifName, ios::binary|ios::out);
	while (!fTif)
	{
		cout<<"  Can't Open "<<string(DEM.Param.OutPath)+TifName<<", Close Program Occupied and Retry! >\n";   cin.get();
		fTif.open(string(DEM.Param.OutPath)+TifName, ios::binary|ios::out);
	}
	cout<<"  Printing \\"<<DEM.Param.FileName<<"\\"<<TifName<<"...      ";
	BIFH.ByteOrder=0x4949;   BIFH.Version=0x2B;   BIFH.OffsetByteSize=8;
	TS.Across=(DEM.Col+127)/128;   TS.Down=(DEM.Row+127)/128;
	TS.Width=128; TS.Length=128;   TS.TileStrips=TS.Across*TS.Down;
	TS.ByteCounts=new(UInt64[(size_t)TS.TileStrips]);
	TS.Offsets=new(UInt64[(size_t)TS.TileStrips]);
	if (Kind=="SelfDEM")
	{
		SampleFormat=3;   GeoNoData=new(char[20]);   strcpy(GeoNoData, "    -9999 ");
		Vmin=1e5;   Vmax=-1e5;   Vmean=0.0;   Vstd=0.0;
		for(Now=1; Now<=DEM.AllArea; Now++)
		{
			Vmin=min(Vmin, DEM.Cell[Now].E);   Vmax=max(Vmax, DEM.Cell[Now].E);   Vmean+=DEM.Cell[Now].E/DEM.AllArea;
		}
		for(Now=1; Now<=DEM.AllArea; Now++)
			Vstd+=(Vmean-DEM.Cell[Now].E)*(Vmean-DEM.Cell[Now].E)/DEM.AllArea;
	}
	else if (Kind=="Dir")
	{
		SampleFormat=2;   GeoNoData=new(char[20]);   strcpy(GeoNoData, "    -9999 ");
		Vmin=1e20;   Vmax=-1e20;   Vmean=0.0;   Vstd=0.0;
		for(Now=1; Now<=DEM.AllArea; Now++)
		{
			Vmin=min(Vmin, DEM.Cell[Now].D);   Vmax=max(Vmax, DEM.Cell[Now].D);   Vmean+=DEM.Cell[Now].D/DEM.AllArea;
		}
		for(Now=1; Now<=DEM.AllArea; Now++)
			Vstd+=(Vmean-DEM.Cell[Now].D)*(Vmean-DEM.Cell[Now].D)/DEM.AllArea;
	}
	else if (Kind == "Slope")
	{
		SampleFormat = 3;   GeoNoData = new(char[20]);   strcpy(GeoNoData, "    -9999 ");
		Vmin = 1e5;   Vmax = -1e5;   Vmean = 0.0;   Vstd = 0.0;
		for (Now = 1; Now <= DEM.AllArea; Now++)
		{
			Vmin = min(Vmin, DEM.Cell[Now].S);   Vmax = max(Vmax, DEM.Cell[Now].S);   Vmean += DEM.Cell[Now].S / DEM.AllArea;
		}
		for (Now = 1; Now <= DEM.AllArea; Now++)
			Vstd += (Vmean - DEM.Cell[Now].S)*(Vmean - DEM.Cell[Now].S) / DEM.AllArea;
	}
	else if (Kind == "Area")
	{
		SampleFormat = 2;   GeoNoData = new(char[20]);   strcpy(GeoNoData, "    -9999 ");
		Vmin = 1e20;   Vmax = -1e20;   Vmean = 0.0;   Vstd = 0.0;
		for (Now = 1; Now <= DEM.AllArea; Now++)
		{
			Vmin = min(Vmin, DEM.Cell[Now].A);   Vmax = max(Vmax, DEM.Cell[Now].A);   Vmean += DEM.Cell[Now].A / DEM.AllArea;
		}
		for (Now = 1; Now <= DEM.AllArea; Now++)
			Vstd += (Vmean - DEM.Cell[Now].A)*(Vmean - DEM.Cell[Now].A) / DEM.AllArea;
	}
	else if (Kind == "Mark")
	{
		SampleFormat = 2;   GeoNoData = new(char[20]);   strcpy(GeoNoData, "    -9999 ");
		Vmin = 1e20;   Vmax = -1e20;   Vmean = 0.0;   Vstd = 0.0;
		for (Now = 1; Now <= DEM.AllArea; Now++)
		{
			Vmin = min(Vmin, DEM.Cell[Now].G);   Vmax = max(Vmax, DEM.Cell[Now].G);   Vmean += DEM.Cell[Now].G / DEM.AllArea;
		}
		for (Now = 1; Now <= DEM.AllArea; Now++)
			Vstd += (Vmean - DEM.Cell[Now].G)*(Vmean - DEM.Cell[Now].G) / DEM.AllArea;
	}
	else
	{
		cout<<"  Unknow Kind of Tif "<<Kind<<"! >\n\n";   return false;
	}
	BitsPerSample=32;   PixelBytes=BitsPerSample/8;   Vstd=sqrt(Vstd);
	for (i=0;i<TS.TileStrips;i++)
		TS.ByteCounts[i]=128*128*PixelBytes;
	GeoMetaData=new(char[800]);
	sprintf(GeoMetaData, "%s%s %.3f %s %.3f %s %.3f %s %.3f %s\0", 
		"<GDALMetadata>\n<Item name=\"PyramidResamplingType\" domain=\"ESRI\">NEAREST", 
		"</Item>\n<Item name=\"STATISTICS_MINIMUM\" sample=\"0\">", Vmin, 
		"</Item>\n<Item name=\"STATISTICS_MAXIMUM\" sample=\"0\">", Vmax, 
		"</Item>\n<Item name=\"STATISTICS_MEAN\" sample=\"0\">", Vmean, 
		"</Item>\n<Item name=\"STATISTICS_STDDEV\" sample=\"0\">", Vstd, "</Item>\n</GDALMetadata>\n");
	DECount=21; DE=new(sBTifDE[(size_t)DECount]);  memset(DE, 0, (size_t)DECount*sizeof(sBTifDE));
	DE[0].TagId=256;   DE[0].TagType=3;   DE[0].TagLength=1;   DE[0].ValueOffset=DEM.Col;
	DE[1].TagId=257;   DE[1].TagType=3;   DE[1].TagLength=1;   DE[1].ValueOffset=DEM.Row;
	DE[2].TagId=258;   DE[2].TagType=3;   DE[2].TagLength=1;   DE[2].ValueOffset=BitsPerSample;
	DE[3].TagId=259;   DE[3].TagType=3;   DE[3].TagLength=1;   DE[3].ValueOffset=Compression;
	DE[4].TagId=262;   DE[4].TagType=3;   DE[4].TagLength=1;   DE[4].ValueOffset=Photometric;
	DE[5].TagId=266;   DE[5].TagType=3;   DE[5].TagLength=1;   DE[5].ValueOffset=FillOrder;
	DE[6].TagId=274;   DE[6].TagType=3;   DE[6].TagLength=1;   DE[6].ValueOffset=Orientation;
	DE[7].TagId=277;   DE[7].TagType=3;   DE[7].TagLength=1;   DE[7].ValueOffset=SamplesPerPixel;
	DE[8].TagId=284;   DE[8].TagType=3;   DE[8].TagLength=1;   DE[8].ValueOffset=Planar;
	DE[9].TagId=322;   DE[9].TagType=3;   DE[9].TagLength=1;   DE[9].ValueOffset=TS.Width;
	DE[10].TagId=323;   DE[10].TagType=3;   DE[10].TagLength=1;   DE[10].ValueOffset=TS.Length;
	DE[13].TagId=339;   DE[13].TagType=3;   DE[13].TagLength=1;   DE[13].ValueOffset=SampleFormat;
	DE[11].TagId=324;   DE[11].TagType=16;   DE[11].TagLength=TS.TileStrips;
	DE[12].TagId=325;   DE[12].TagType=16;   DE[12].TagLength=TS.TileStrips;
	DE[14].TagId=33550;   DE[14].TagType=12;   DE[14].TagLength=3;
	DE[15].TagId=33922;   DE[15].TagType=12;   DE[15].TagLength=6;
	DE[16].TagId=34735;   DE[16].TagType=3;   DE[16].TagLength=GKECount*4+4;
	DE[17].TagId=34736;   DE[17].TagType=12;   DE[17].TagLength=GeoDoubleNum;
	DE[18].TagId=34737;   DE[18].TagType=2;   DE[18].TagLength=strlen(GeoAscii);
	DE[19].TagId=42112;   DE[19].TagType=2;   DE[19].TagLength=strlen(GeoMetaData);
	DE[20].TagId=42113;   DE[20].TagType=2;   DE[20].TagLength=strlen(GeoNoData);
	fTif.write((char*)&BIFH, 16);   FileOffset=16;   Count=0;
	for (r=0;r<TS.Down;r++)
		for (c=0;c<TS.Across;c++)
		{
			TS.Offsets[Count]=FileOffset;   Count++;   FileOffset+=128*128*PixelBytes;
			for (i=r*128+1;i<=(r+1)*128;i++)
				for(j=c*128+1;j<=(c+1)*128;j++)
					if (Kind=="SelfDEM")
					{
						if (i<=DEM.Row && j<=DEM.Col && DEM.Map[i][j]>0) TmpF=DEM.Cell[DEM.Map[i][j]].E;
						else TmpF=-9999;
						fTif.write((char*)&TmpF, PixelBytes);
					}
					else if(Kind=="Dir")
					{
						if (i<=DEM.Row && j<=DEM.Col && DEM.Map[i][j]>0) TmpL=(UInt32)DEM.Cell[DEM.Map[i][j]].D;
						else TmpL=-9999;
						fTif.write((char*)&TmpL, PixelBytes);
					}
					else if (Kind == "Slope")
					{
						if (i <= DEM.Row && j <= DEM.Col && DEM.Map[i][j]>0) TmpF = DEM.Cell[DEM.Map[i][j]].S;
						else TmpF = -9999;
						fTif.write((char*)&TmpF, PixelBytes);
					}
					else if (Kind == "Area")
					{
						if (i <= DEM.Row && j <= DEM.Col && DEM.Map[i][j]>0) TmpL = (UInt32)DEM.Cell[DEM.Map[i][j]].A;
						else TmpL = -9999;
						fTif.write((char*)&TmpL, PixelBytes);
					}
					else if (Kind == "Mark")
					{
						if (i <= DEM.Row && j <= DEM.Col && DEM.Map[i][j]>0) TmpL = (UInt32)DEM.Cell[DEM.Map[i][j]].G;
						else TmpL = -9999;
						fTif.write((char*)&TmpL, PixelBytes);
					}
			if ((UInt64)(Count-1)*100/TS.TileStrips<(UInt64)Count*100/TS.TileStrips)
				cout<<"\b\b\b\b"<<setw(3)<<(UInt64)Count*100/TS.TileStrips<<"%";
		}
	cout<<"\n\n";
	DE[11].ValueOffset=FileOffset;   FileOffset+=TS.TileStrips*8;
	fTif.write((char*)TS.Offsets, 8*TS.TileStrips);
	DE[12].ValueOffset=FileOffset;   FileOffset+=TS.TileStrips*8;
	fTif.write((char*)TS.ByteCounts, 8*TS.TileStrips);
	DE[14].ValueOffset=FileOffset;   FileOffset+=8*3;
	fTif.write((char*)DEM.Param.GeoPixelScale, 8*3);
	DE[15].ValueOffset=FileOffset;   FileOffset+=8*6;
	fTif.write((char*)DEM.Param.GeoTiePoint, 8*6);
	DE[16].ValueOffset=FileOffset;   FileOffset+=8+8*GKECount;
	fTif.write((char*)&GKH, 8);   fTif.write((char*)GKE, 8*GKECount);
	DE[17].ValueOffset=FileOffset;   FileOffset+=8*GeoDoubleNum;
	fTif.write((char*)GeoDouble, 8*GeoDoubleNum);
	DE[18].ValueOffset=FileOffset;   FileOffset+=strlen(GeoAscii);
	fTif.write(GeoAscii, strlen(GeoAscii));
	DE[19].ValueOffset=FileOffset;   FileOffset+=strlen(GeoMetaData);
	fTif.write(GeoMetaData, strlen(GeoMetaData));
	DE[20].ValueOffset=FileOffset;   FileOffset+=strlen(GeoNoData);
	fTif.write(GeoNoData, strlen(GeoNoData));
	BIFH.IFDOffset=FileOffset;   FileOffset+=8+20*DECount+8;
	fTif.write((char*)&DECount, 8);   fTif.write((char*)DE, 20*DECount);
	fTif.write((char*)&NextIFDOffset, 8);
	fTif.seekp(0);  fTif.write((char*)&BIFH, 16);   fTif.close();
	delete [] TS.Offsets;   delete [] TS.ByteCounts;   delete[] DE;   return true;
};