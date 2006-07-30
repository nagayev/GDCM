/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006 Mathieu Malaterre
  Copyright (c) 1993-2005 CREATIS
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmRLECodec.h"
#include "gdcmTS.h"
#include "gdcmOStream.h"
#include "gdcmIStream.h"
#include "gdcmStringStream.h"

#include <vector>

namespace gdcm
{

// TODO ideally this code should be in utilities for ease of reuse
class RLEHeader
{
public:
  unsigned long NumSegments;
  unsigned long Offset[15];

  void Print(std::ostream &os)
    {
    os << "NumSegments:" << NumSegments << "\n";
    for(int i=0; i<15; ++i)
      {
      os << i << ":" << Offset[i] << "\n";
      }
    }
};

class RLEFrame
{
public:
  void Read(IStream &is)
    {
    // read Header (64 bytes)
    is.Read((char*)(&Header), sizeof(unsigned long)*16);
    unsigned long numSegments = Header.NumSegments;
    if( numSegments >= 1 )
      {
      assert( Header.Offset[0] == 64 );
      }
    // We just check that we are indeed at the proper position start+64
    //char bytes[256*256];
    }
  void Print(std::ostream &os)
    {
    Header.Print(os);
    }
//private:
  RLEHeader Header;
  std::vector<char> Bytes;
};

class RLEInternals
{
public:
  RLEFrame Frame;
  std::vector<uint32_t> SegmentLength;
};

RLECodec::RLECodec()
{
  Internals = new RLEInternals;
}

RLECodec::~RLECodec()
{
  delete Internals;
}

bool RLECodec::CanDecode(TS const &ts)
{
  (void)ts;
  abort();
  return true;
}

// G.3.2 The RLE decoder
// Pseudo code for the RLE decoder is shown below:
// Loop until the number of output bytes equals the uncompressed segment size
// Read the next source byte into n
// If n> =0 and n <= 127 then
// output the next n+1 bytes literally
// Elseif n <= - 1 and n >= -127 then
// output the next byte -n+1 times
// Elseif n = - 128 then
// output nothing
// Endif
// Endloop

bool RLECodec::Decode(IStream &is, OStream &os)
{
  // FIXME: Do some stupid work:
  //is.Seekg( 0, std::ios::end);
  //std::streampos buf_size = is.Tellg();
  char dummy_buffer[256];
  //is.Seekg(0, std::ios::beg);
  //is.Read( dummy_buffer, buf_size);
  //SwapCode sc = is.GetSwapCode();
  //os.Write(dummy_buffer, buf_size);
  //delete[] dummy_buffer;
  //long numOutBytes = 0;
  //while()
  //  {
  //  }
  StringStream tmpos;
  // DEBUG
  is.Seekg( 0, std::ios::end);
  std::streampos buf_size = is.Tellg();
  is.Seekg(0, std::ios::beg);
  // END DEBUG

  RLEFrame &frame = Internals->Frame;
  frame.Read(is);
  frame.Print(std::cout);
  unsigned long numSegments = frame.Header.NumSegments;

  unsigned long numberOfReadBytes = 0;

  for(unsigned long i = 0; i<numSegments; ++i)
    {
    std::streampos pos = is.Tellg();
    if ( frame.Header.Offset[i] - pos != 0 )
      {
      std::cerr << "OLD POS: " << pos << " new pos " << frame.Header.Offset[i] << std::endl;
      is.Seekg(frame.Header.Offset[i], std::ios::beg);
      abort();
      }

    unsigned long numOutBytes = 0;
    char byte;
    //std::cerr << "Length: " << Length << "\n";
    //assert( (unsigned long)is.Tellg() == frame.Header.Offset[i] );
    while( numOutBytes < Length / numSegments )
      {
      //std::cerr << "numOutBytes: " << numOutBytes << "\n";
      is.Read(&byte, 1);
      numberOfReadBytes++;
      //std::cerr << "Byte: " << int(byte) << "\n";
      if( byte >= 0 /*&& byte <= 127*/ )
        {
        is.Read( dummy_buffer, byte+1 );
      numberOfReadBytes += byte+1;
        numOutBytes += byte+ 1;
        tmpos.Write( dummy_buffer, byte+1 );
        }
      else if( byte <= -1 && byte >= -127 )
        {
        char nextByte;
        is.Read( &nextByte, 1);
      numberOfReadBytes += 1;
        for( int c = 0; c < -byte + 1; ++c )
          {
          dummy_buffer[c] = nextByte;
          }
        numOutBytes += -byte + 1;
        tmpos.Write( dummy_buffer, -byte+1 );
        }
      else /* byte == -128 */
        {
        assert( byte == -128 );
        }
      }
    std::cerr << "numOutBytes:" << numOutBytes << " " << Length << "\n";
    std::cerr << "DEBUG: " << numberOfReadBytes << std::endl;
    //if( numOutBytes != Length )
    //  {
    //  std::cerr << numOutBytes << " " << Length << std::endl;
    //  }
    }


  ImageCodec::Decode(tmpos,os);
  return true;
}

} // end namespace gdcm
