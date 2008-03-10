/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmCurveData.h"
#include "gdcmDataElement.h"
#include "gdcmDataSet.h"
#include "gdcmAttribute.h"

#include <vector>

namespace gdcm
{
/*

C.10.2.1 Curve Attribute Descriptions
C.10.2.1.1 Type of data
A description of the Type of Data (50xx,0020) in this curve.
Defined Terms:
TAC = time activity curve PROF = image profile
HIST = histogram ROI = polygraphic region of interest
TABL = table of values FILT = filter kernel
POLY = poly line ECG = ecg data
PRESSURE = pressure data FLOW = flow data
PHYSIO = physio data RESP = Respiration trace
*/

class CurveDataInternal
{
public:
  CurveDataInternal():
  Group(0),
  Dimensions(0),
  NumberOfPoints(0),
  TypeOfData(),
  CurveDescription(),
  DataValueRepresentation(0),
  Data() {}

/*
(5004,0000) UL 2316                                     #   4, 1 CurveGroupLength
(5004,0005) US 1                                        #   2, 1 CurveDimensions
(5004,0010) US 1126                                     #   2, 1 NumberOfPoints
(5004,0020) CS [PHYSIO]                                 #   6, 1 TypeOfData
(5004,0022) LO (no value available)                     #   0, 0 CurveDescription
(5000,0030) SH [DPPS\NONE]                              #  10, 2 AxisUnits
(5004,0103) US 0                                        #   2, 1 DataValueRepresentation
(5000,0110) US 0\1                                      #   4, 2 CurveDataDescriptor
(5000,0112) US 0                                        #   2, 1 CoordinateStartValue
(5000,0114) US 300                                      #   2, 1 CoordinateStepValue
(5000,2500) LO [Physio_1]                               #   8, 1 CurveLabel
(5004,3000) OW 0020\0020\0020\0020\0020\0020\0020\0020\0020\0020\0020\0020\0020... # 2252, 1 CurveData
*/
// Identifier need to be in the [5000,50FF] range (no odd number):
  unsigned short Group;
  unsigned short Dimensions;
  unsigned short NumberOfPoints;
  std::string TypeOfData;
  std::string CurveDescription;
  unsigned short DataValueRepresentation;
  std::vector<char> Data;
  void Print(std::ostream &os) const {
    os << "Group           0x" <<  std::hex << Group << std::dec << std::endl;
    os << "Dimensions                         :" << Dimensions << std::endl;
    os << "NumberOfPoints                     :" << NumberOfPoints << std::endl;
    os << "TypeOfData                         :" << TypeOfData << std::endl;
    os << "CurveDescription                   :" << CurveDescription << std::endl;
    os << "DataValueRepresentation            :" << DataValueRepresentation << std::endl;
    //std::vector<bool> Data;
  }
};

CurveData::CurveData()
{
}

CurveData::~CurveData()
{
}

CurveData::CurveData(CurveData const &ov):Object(ov)
{
  delete Internal;
  Internal = new CurveDataInternal;
  // TODO: copy CurveDataInternal into other...
  *Internal = *ov.Internal;
}

void CurveData::Print(std::ostream &os) const
{
  Internal->Print( os );
}

unsigned int CurveData::GetNumberOfCurveDatas(DataSet const & ds)
{
  Tag overlay(0x5000,0x0000); // First possible overlay
  bool finished = false;
  unsigned int numoverlays = 0;
  while( !finished )
    {
    const DataElement &de = ds.FindNextDataElement( overlay );
    if( de.GetTag().GetGroup() > 0x50FF ) // last possible curve
      {
      finished = true;
      }
    else if( de.GetTag().IsPrivate() )
      {
      // Move on to the next public one:
      overlay.SetGroup( de.GetTag().GetGroup() + 1 );
      }
    else
      {
      // Yeah this is an overlay element
      ++numoverlays;
      // Move on to the next possible one:
      overlay.SetGroup( overlay.GetGroup() + 2 );
      }
    }

  return numoverlays;
}

void CurveData::Update(const DataElement & de)
{
  assert( de.GetTag().IsPublic() );
  const ByteValue* bv = de.GetByteValue();
  if( !bv ) return; // Discard any empty element (will default to another value)
  assert( bv->GetPointer() && bv->GetLength() );
  std::string s( bv->GetPointer(), bv->GetLength() );
  // What if a \0 can be found before the end of string...
  //assert( strlen( s.c_str() ) == s.size() );

  // First thing check consistency:
  if( !GetGroup() )
    {
    SetGroup( de.GetTag().GetGroup() );
    }
  else // check consistency
    {
    assert( GetGroup() == de.GetTag().GetGroup() ); // programmer error
    }

  //std::cerr << "Tag: " << de.GetTag() << std::endl;
  if( de.GetTag().GetElement() == 0x0000 ) // CurveDataGroupLength
    {
    // ??
    }
  else if( de.GetTag().GetElement() == 0x0005 ) // CurveDimensions
    {
    Attribute<0x5000,0x0005> at;
    at.SetFromDataElement( de );
    SetDimensions( at.GetValue() );
    }
  else if( de.GetTag().GetElement() == 0x0010 ) // NumberOfPoints
    {
    Attribute<0x5000,0x0010> at;
    at.SetFromDataElement( de );
    SetNumberOfPoints( at.GetValue() );
    }
  else if( de.GetTag().GetElement() == 0x0020 ) // TypeOfData
    {
    SetTypeOfData( s.c_str() );
    }
  else if( de.GetTag().GetElement() == 0x0022 ) // CurveDescription
    {
    SetCurveDescription( s.c_str() );
    }
  else if( de.GetTag().GetElement() == 0x0030 ) // AxisUnits
    {
      gdcmWarningMacro( "TODO" );
    }
  else if( de.GetTag().GetElement() == 0x0103 ) // DataValueRepresentation
    {
    Attribute<0x5000,0x0103> at;
    at.SetFromDataElement( de );
    SetDataValueRepresentation( at.GetValue() );
    }
  else if( de.GetTag().GetElement() == 0x0110 ) // CurveDataDescriptor
    {
      gdcmWarningMacro( "TODO" );
    }
  else if( de.GetTag().GetElement() == 0x0112 ) // CoordinateStartValue
    {
      gdcmWarningMacro( "TODO" );
    }
  else if( de.GetTag().GetElement() == 0x0114 ) // CoordinateStepValue
    {
      gdcmWarningMacro( "TODO" );
    }
  else if( de.GetTag().GetElement() == 0x2500 ) // CurveLabel
    {
      gdcmWarningMacro( "TODO" );
    }
  else
    {
    assert( 0 && "should not happen" );
    }

}

void CurveData::SetGroup(unsigned short group) { Internal->Group = group; }
unsigned short CurveData::GetGroup() const { return Internal->Group; }
void CurveData::SetDimensions(unsigned short dimensions) { Internal->Dimensions = dimensions; }
unsigned short CurveData::GetDimensions() const { return Internal->Dimensions; }
void CurveData::SetNumberOfPoints(unsigned short numberofpoints) { Internal->NumberOfPoints = numberofpoints; }
unsigned short CurveData::GetNumberOfPoints() const { return Internal->NumberOfPoints; }
void CurveData::SetTypeOfData(const char *typeofdata) { Internal->TypeOfData = typeofdata; }
void CurveData::SetCurveDescription(const char *curvedescription) { Internal->CurveDescription = curvedescription; }
void CurveData::SetDataValueRepresentation(unsigned short datavaluerepresentation) { Internal->DataValueRepresentation = datavaluerepresentation; }
unsigned short CurveData::GetDataValueRepresentation() const { return Internal->DataValueRepresentation; }

bool CurveData::IsEmpty() const
{
  return Internal->Data.empty();
}

void CurveData::Decode(std::istream &is, std::ostream &os)
{
}

}

