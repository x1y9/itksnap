/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkVoxBoCUBImageIO.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
  Version:   $1.0$

  Copyright (c) Insight Software Consortium. All rights reserved.
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 


=========================================================================*/
#ifndef __itkVoxBoCUBImageIO_h
#define __itkVoxBoCUBImageIO_h

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

#include <fstream>
#include <string>
#include <map>
#include "itkImageIOBase.h"
#include "itkSpatialOrientation.h"
#include <stdio.h>

namespace itk
{
  class GenericCUBFileAdaptor;
  
/** \class VoxBoCUBImageIO
 *
 *  \brief Read VoxBoCUBImage file format. 
 *
 *  \ingroup IOFilters
 *
 */
class ITK_EXPORT VoxBoCUBImageIO : public ImageIOBase
{
public:
  /** Standard class typedefs. */
  typedef VoxBoCUBImageIO            Self;
  typedef ImageIOBase  Superclass;
  typedef SmartPointer<Self>  Pointer;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(VoxBoCUBImageIO, Superclass);

  /*-------- This part of the interfaces deals with reading data. ----- */

  /** Determine the file type. Returns true if this ImageIO can read the
   * file specified. */
  virtual bool CanReadFile(const char*) override;

  /** Set the spacing and dimension information for the set filename. */
  virtual void ReadImageInformation() override;
  
  /** Reads the data from disk into the memory buffer provided. */
  virtual void Read(void* buffer) override;

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine the file type. Returns true if this ImageIO can write the
   * file specified. */
  virtual bool CanWriteFile(const char*) override;

  /** Set the spacing and dimension information for the set filename. */
  virtual void WriteImageInformation() override;
  
  /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegions has been set properly. */
  virtual void Write(const void* buffer) override;


  VoxBoCUBImageIO();
  ~VoxBoCUBImageIO();
  void PrintSelf(std::ostream& os, Indent indent) const override;
  
private:
  VoxBoCUBImageIO(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  bool CheckExtension(const char*, bool &isCompressed);
  GenericCUBFileAdaptor *CreateReader(const char *filename);
  GenericCUBFileAdaptor *CreateWriter(const char *filename);
  GenericCUBFileAdaptor *m_Reader, *m_Writer;

  // Initialize the orientation map (from strings to ITK)
  void InitializeOrientationMap();

  // Orientation stuff
  typedef SpatialOrientation::ValidCoordinateOrientationFlags OrientationFlags;
  typedef std::map<std::string, OrientationFlags> OrientationMap;
  typedef std::map<OrientationFlags, std::string> InverseOrientationMap;

  OrientationMap m_OrientationMap;
  InverseOrientationMap m_InverseOrientationMap;

  // Method to swap bytes in read buffer
  void SwapBytesIfNecessary(void *buffer, unsigned long numberOfBytes);

  // Strings used in VoxBo files
  static const char *VB_IDENTIFIER_SYSTEM;
  static const char *VB_IDENTIFIER_FILETYPE;
  static const char *VB_DIMENSIONS;
  static const char *VB_SPACING;
  static const char *VB_ORIGIN;
  static const char *VB_DATATYPE;
  static const char *VB_BYTEORDER;
  static const char *VB_ORIENTATION;
  static const char *VB_BYTEORDER_MSB;
  static const char *VB_BYTEORDER_LSB;
  static const char *VB_DATATYPE_BYTE;
  static const char *VB_DATATYPE_INT;
  static const char *VB_DATATYPE_FLOAT;
  static const char *VB_DATATYPE_DOUBLE;
};

} // end namespace itk

#endif // __itkVoxBoCUBImageIO_h
