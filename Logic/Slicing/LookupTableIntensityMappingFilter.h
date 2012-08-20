#ifndef LOOKUPTABLEINTENSITYMAPPINGFILTER_H
#define LOOKUPTABLEINTENSITYMAPPINGFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>


/**
  This ITK filter uses a lookup table to map image intensities. The input
  image should be of an integral type.
  */
template<class TInputImage, class TOutputImage>
class LookupTableIntensityMappingFilter :
    public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef LookupTableIntensityMappingFilter<TInputImage, TOutputImage>   Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>       Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::PixelType                   InputPixelType;
  typedef TOutputImage                                        OutputImageType;
  typedef typename OutputImageType::PixelType                 OutputPixelType;

  typedef itk::Image<OutputPixelType, 1>                      LookupTableType;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  itkTypeMacro(LookupTableIntensityMappingFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetLookupTable(LookupTableType *lut);

  /** The actual work */
  void ThreadedGenerateData(const OutputImageRegionType &region, int threadId);

protected:

  LookupTableIntensityMappingFilter();
  virtual ~LookupTableIntensityMappingFilter() {}

  SmartPtr<LookupTableType> m_LookupTable;
};


#endif // LOOKUPTABLEINTENSITYMAPPINGFILTER_H
