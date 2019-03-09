#pragma once

#include "DEM.h"

struct sGKH				//GeoTif GeoKey Header 8 Byte, default=1,1,0,GKCount
{
	UInt16		KeyDirVersion, KeyRevision, MinorRevision, NumberOfKeys;
};

struct sGKE				//GeoTif GeoKey Entry 8 Byte
{
	UInt16		KeyId, TagLocation, Count, ValueOffset;
};

struct sTifIFH			//Tif Image File Header 8 Byte
{
	UInt16		ByteOrder, Version;			//ByteOrder little:II 0x4949  big:MM 0x4D4D
	UInt32		IFDOffset;					//Version = 42 0x2A, IFDOffset: Directory Offset
};

struct sBTifIFH			//Big Tif Image File Header 16 Byte
{
	UInt16		ByteOrder, Version;			//ByteOrder little:II 0x4949  big:MM 0x4D4D
	UInt16		OffsetByteSize, Reversed;	//Version = 43 0x2B, ByteSize of Offsets =8
	UInt64		IFDOffset;					//Directory Offset
};

struct sBTifDE			//Big Tif Directory Entry 20 Byte
{
	UInt16		TagId,TagType;
	UInt64		TagLength, ValueOffset;
};

struct sTileStrip		//Big Tif Tile & Strip Record
{
	UInt64		Width, Length, Across, Down, TileStrips;
	UInt64		*Offsets, *ByteCounts;
};

class TTif
{
public:
	fstream		fTif;							//Tif File
	Int32		ByteOrder, Version;				//0x4949=II(Intel), 42=Tif 43=BigTif
	Int32		Compression, Photometric;		//1=No Compression, 1=Gray Image
	Int32		Planar, FillOrder, Orientation;	//1=Contiguously, 1=little(Intel), 1=TopLeft
	Int32		BitsPerSample, SamplesPerPixel;	//Pixel Size
	Int32		PixelBytes, SampleFormat;		//Pixel Bytes and Format
	float		Nodata;							//DEM Nodata Value

	TTif();
	~TTif();

	bool DEM_ReadTif(TDEM &);					//1.Read Tif File
	bool DEM_PrintTif(TDEM &, CString);			//2.Output Tif Result

private:
	sTifIFH		IFH;							//Tif IFH or BIFH
	sBTifIFH	BIFH;
	UInt64		DECount,NextIFDOffset;			//IFD.DECount, IFD.DE, IFD.NextIFDOffset
	sBTifDE		*DE;
	sTileStrip	TS;								//Tif Data Store: Tile or Strip
	UInt16		GKECount;						//GeoKey.GKH, GeoKey.GKE
	sGKH		GKH;
	sGKE		*GKE;
	UInt64		GeoDoubleNum;					//GeoDouble, GeoNoData, GeoAscii, GeoMetaData
	double		*GeoDouble;
	char		*GeoNoData, *GeoAscii, *GeoMetaData;

	UInt64		FileOffset;	
	double		Vmin, Vmax, Vmean, Vstd;

	Int32 SetTag(TDEM &, UInt32);											//1.Set Tag by TagId
	Int32 GetBuffValue(void *, UInt32, UInt64, UInt32, UInt32, UInt64);		//1.Assign Buff Value to Tag
};