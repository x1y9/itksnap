#ifndef MESHWRAPPERBASE_H
#define MESHWRAPPERBASE_H

#include "SNAPCommon.h"
#include "GuidedMeshIO.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "ImageWrapperBase.h"
#include "MeshDataArrayProperty.h"
#include "ColorMap.h"
#include "ThreadedHistogramImageFilter.h"
#include "vtkPolyData.h"

class AbstractMeshIODelegate;
class MeshDisplayMappingPolicy;
class MeshAssembly;
class vtkDataSetAttributes;
class vtkPlaneCutter;
class vtkPlane;
class vtkStripper;
class vtkCleanPolyData;
namespace itk {
template <unsigned int VDim> class ImageBase;
}

class PlaneCutterAssembly : public itk::Object
{
public:
  irisITKObjectMacro(PlaneCutterAssembly, itk::Object);
  friend class PolyDataWrapper;
protected:

  vtkSmartPointer<vtkPlaneCutter> m_Cutter;
  vtkSmartPointer<vtkPlane> m_Plane;
  vtkSmartPointer<vtkStripper> m_Stripper;
  vtkSmartPointer<vtkCleanPolyData> m_Cleaner;
};

/**
 * @brief The PolyDataWrapper class
 */
class PolyDataWrapper : public itk::Object
{
public:
  irisITKObjectMacro(PolyDataWrapper, itk::Object);

  typedef GuidedMeshIO::FileFormat FileFormat;
  typedef itk::ImageBase<3> DisplayViewportGeometryType;

  // comparisonn functor for map with char* key
  struct strcmp
  {
     bool operator()(char const *a, char const *b) const
     {
        return std::strcmp(a, b) < 0;
     }
  };

  /** Mesh Data Property Map */
  typedef std::map<const char*, SmartPtr<MeshDataArrayProperty>, strcmp> MeshDataArrayPropertyMap;

  /** Mesh Data Type */
  typedef MeshDataArrayProperty::MeshDataType MeshDataType;

  void SetPolyData(vtkPolyData *polydata);

  vtkPolyData *GetPolyData();

  MeshDataArrayPropertyMap &GetPointDataProperties()
  { return m_PointDataProperties; }

  MeshDataArrayPropertyMap &GetCellDataProperties()
  { return m_CellDataProperties; }

  void SetFileName(const char *name)
  { m_FileName = name; }

  const char * GetFileName()
  { return m_FileName.c_str(); }

  FileFormat GetFileFormat()
  { return m_FileFormat; }

  void SetFileFormat(FileFormat fmt)
  { m_FileFormat = fmt; }

  /**
   * Compute intersection between this mesh layer and one of the slicing planes
   */
  vtkPolyData *GetIntersectionWithSlicePlane(DisplaySliceIndex                  index,
                                             const DisplayViewportGeometryType *geometry);

  friend class MeshDataArrayProperty;
protected:
  PolyDataWrapper() {}
  virtual ~PolyDataWrapper() {}

  // Update point data and cell data properties
  void UpdateDataArrayProperties();

  // Helper method for iterating data arrays in polydata
  void UpdatePropertiesFromVTKData(MeshDataArrayPropertyMap &propMap,
                                   vtkDataSetAttributes *data,
                                   MeshDataType type) const;

  // The actual storage of a poly data object
  vtkSmartPointer<vtkPolyData> m_PolyData;

  // Point Data Properties
  MeshDataArrayPropertyMap m_PointDataProperties;

  // Cell Data Properties
  MeshDataArrayPropertyMap m_CellDataProperties;

  // Cutters used to display 2D intersections of the mesh with display slices
  DisplaySlicePipelineArray<PlaneCutterAssembly> m_PlaneCut;

  // Filename (default to empty string)
  // Polydata generated from segmentation don't have a file name
  std::string m_FileName = "";

  // File format for polydata associated with a file
  FileFormat m_FileFormat = FileFormat::FORMAT_COUNT;
};


/**
 *  \class MeshAssembly
 *  \brief An abstract class representing a scene assembled by multiple meshes
 *
 *  The class wraps around a MeshMap, which maps a collection of vtkPolyData
 *  , identified by ids (id is in LabelType).
 */

class MeshAssembly : public itk::Object
{
public:
  irisITKObjectMacro(MeshAssembly, itk::Object);

  typedef std::map<LabelType, SmartPtr<PolyDataWrapper>> MeshAssemblyMap;

  /** Add a mesh to the map. If the id already exist, old mesh will be overwritten */
  void AddMesh(PolyDataWrapper *mesh, LabelType id=0u);

  /** Get the mesh by id */
  PolyDataWrapper *GetMesh(LabelType id);

  /** Check existence of id */
  bool Exist(LabelType id);

  /** Erase an entry, and take care of the deletion of data */
  void Erase(LabelType id);

  /** Get the beginning iterator to the map */
  MeshAssemblyMap::const_iterator cbegin()
  { return m_Meshes.cbegin(); }

  /** Get the end of iterator of the map */
  MeshAssemblyMap::const_iterator cend()
  { return m_Meshes.cend(); }

  MeshAssemblyMap::size_type size()
  { return m_Meshes.size(); }

  // Compute combined bounds as double array
  void GetCombinedBounds(double bounds[6]) const;

  // Compute combined bounds as array string
  std::string GetCombinedBoundsString() const;

  /** Get actual memory usage of all the polydata in the assembly in megabytes */
  double GetTotalMemoryInMB() const;

  /** Save to Registry */
  void SaveToRegistry(Registry &folder);

protected:
  MeshAssembly() {}
  virtual ~MeshAssembly() {}

  // Map storing all the meshes in the assembly
  MeshAssemblyMap m_Meshes;
};


/**
 *  \class MeshWrapperBase
 *  \brief An abstract class representing a mesh layer in ITK-SNAP
 *
 *  The class wraps around a MeshAssemblyMap, which maps MeshAssemblies, representing
 *  a scene assembled by meshes, to time points.
 *
 *  MeshWrappers are initialized by MeshIODelegates, store point/cell data and appearance
 *  properties, and can configure actors when called by renderers.
 *
 *  The class is intend to replace the MeshManager class that was solely focused on the
 *  segmentation based meshes.
 */

class MeshWrapperBase : public WrapperBase
{
public:
  irisITKAbstractObjectMacro(MeshWrapperBase, WrapperBase)

  typedef std::map<std::string, SmartPtr<itk::Object>> UserDataMapType;

  /** Mesh assembly map maps MeshAssembly to time points */
  typedef std::map<unsigned int, SmartPtr<MeshAssembly>> MeshAssemblyMapType;

  /** Maps Layer-Level Data Array Properties with unique ids */
  typedef std::map<int, SmartPtr<MeshLayerDataArrayProperty>> MeshLayerCombinedPropertyMap;

  /** Maps Layer-Level Data Array Properties with name */
  typedef std::map<const char*,
    SmartPtr<MeshLayerDataArrayProperty>, PolyDataWrapper::strcmp> MeshLayerDataArrayPropertyMap;

  /** Element level Data Array properties with name */
  typedef PolyDataWrapper::MeshDataArrayPropertyMap MeshDataArrayPropertyMap;

  typedef std::map<std::string, std::string> MetaDataMap;

  typedef GuidedMeshIO::FileFormat FileFormat;

  typedef itk::ImageBase<3> DisplayViewportGeometryType;

  //----------------------------------------------
  // Begin virtual methods definition

  /** Access the filename */
  irisGetStringMacroWithOverride(FileName)
  virtual void SetFileName(const std::string &name) override;
  virtual void SetFileName(const char *filename, unsigned int tp, LabelType id);

  /**
   * Access the nickname - which may be a custom nickname or derived from the
   * filename if there is no custom nickname
   */
  virtual const std::string &GetNickname() const override;

  // Set the custom nickname - precedence over the filename
  virtual void SetCustomNickname(const std::string &nickname) override;
  irisGetMacroWithOverride(CustomNickname, const std::string &);

  /** Fallback nickname - shown if no filename and no custom nickname set. */
  irisGetSetMacroWithOverride(DefaultNickname, const std::string &)

  /** List of tags assigned to the image layer */
  const TagList &GetTags() const override
  { return m_Tags; }
  void SetTags (const TagList &tags) override
  { m_Tags = tags; }

  /** Layer transparency */
  irisSetWithEventMacroWithOverride(Alpha, double, WrapperDisplayMappingChangeEvent)
  irisGetMacroWithOverride(Alpha, double)

  /**
   * Is the image initialized?
   */
  irisIsMacroWithOverride(Initialized)

  /** Get the MeshAssembly associated with the time point */
  virtual MeshAssembly *GetMeshAssembly(unsigned int timepoint);

  /**
   *  Set the mesh and its id for a time point
   *  Implemente in subclasses
   */
  virtual void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id) = 0;

  /** Return true if the mesh needs update */
  virtual bool IsMeshDirty(unsigned int timepoint) = 0;

  /** Whether meshes can be loaded from external file
   *  Internally generated meshes mostly should not be changed by external
   *  loading; Override this method if the meshes can be externally loaded
   *  and changed.
  */
  virtual bool IsExternalLoadable() const
  { return false; }

  /** Get the mesh display mapping. This saves renderer a type cast */
  virtual MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const = 0;

  /**
    Obtain the t-digest, from which the quantiles of the image/mesh can be
    approximated. The t-digest algorithm by T. Dunning is a fast algorithm for
    approximating statistics such as quantiles, histogram, CDF, etc. It replaces
    earliercode that separately computed the image or mesh min/max and histogram using
    separate filters and required the number of histogram bins to be specified explicitly.

    t-digest code: https://github.com/SpirentOrion/digestible
    t-digest paper: https://www.sciencedirect.com/science/article/pii/S2665963820300403
    */
  virtual TDigestDataObject *GetTDigest() override;

  /** Save the layer to registry */
  virtual void SaveToRegistry(Registry &folder);

  /**
   *  Build the layer from registry
   */
  virtual void LoadFromRegistry(Registry &folder, std::string &orig_dir,
                                std::string &crnt_dir, unsigned int nT);

  // End of virtual methods definition
  //------------------------------------------------

  /** Get the active data array property */
  SmartPtr<MeshLayerDataArrayProperty> GetActiveDataArrayProperty();

  /** Set data array with the id as active mesh data attribute */
  void SetActiveMeshLayerDataPropertyId (int id);
  int GetActiveMeshLayerDataPropertyId()
  { return m_ActiveDataPropertyId; }

  /** Set the solid color to use for this mesh when no property is selected */
  void SetSolidColor(Vector3d color);

  /** Get the solid color to use for this mesh when no property is selected */
  Vector3d GetSolidColor() const;

  /** Set the solid color to use for this mesh when no property is selected */
  void SetSliceViewOpacity(double alpha);

  /** Get the solid color to use for this mesh when no property is selected */
  double GetSliceViewOpacity() const;

  /** Get mesh layer data property map */
  MeshLayerCombinedPropertyMap &GetCombinedDataProperty()
  { return m_CombinedDataPropertyMap; }

  /** Get mesh for a timepoint and id */
  PolyDataWrapper *GetMesh(unsigned int timepoint, LabelType id);

  /** Compute intersection between a mesh and a slice plane */
  vtkPolyData *GetIntersectionWithSlicePlane(unsigned int timepoint, LabelType id, DisplaySliceIndex index);

  /**
    Helper method to merge two data property map
    We need to merge properties from all polydata in the assemblies and timepoints
    to one in the layer
    */
  void MergeDataProperties(MeshLayerDataArrayPropertyMap &dest, MeshDataArrayPropertyMap &src);

  /**
   * Return the metadata map
   */
  MetaDataMap &GetMeshMetaData()
  {
    return m_MetaDataMap;
  }

  // Update Mesh Meta Data
  void UpdateMetaData();

  /** Return the number of polydata currently exist in a timepoint */
  size_t GetNumberOfMeshes(unsigned int timepoint);

  /**
   * Get the display viewport geometries for this mesh layer
   */
  virtual const DisplayViewportGeometryType *GetDisplayViewportGeometry(DisplaySliceIndex index) const
  {
    return m_DisplayViewportGeometry[index];
  }

  /**
   * Set the display viewport geometries for this mesh layer
   */
  virtual void SetDisplayViewportGeometry(DisplaySliceIndex                  index,
                                          const DisplayViewportGeometryType *viewport_image)
  {
    m_DisplayViewportGeometry[index] = viewport_image;
  }

  // Give display mapping policy access to protected members for flexible
  // configuration and data retrieval
  friend class MeshDisplayMappingPolicy;

protected:
  MeshWrapperBase();
  virtual ~MeshWrapperBase();

  // The actual storage of polydata
  MeshAssemblyMapType m_MeshAssemblyMap;

  // Combining Point and Cell Data Properties. Uniqely indexed.
  MeshLayerCombinedPropertyMap m_CombinedDataPropertyMap;

  // Actual storage of the point data cell data properties
  MeshLayerDataArrayPropertyMap m_PointDataProperties;
  MeshLayerDataArrayPropertyMap m_CellDataProperties;

  // Indicate which data property is active
  int m_ActiveDataPropertyId = -1;

  UserDataMapType m_UserDataMap;

  // Each layer has a filename, from which it is belived to have come
  std::string m_FileName, m_FileNameShort;

  // Each layer has a nickname. But this gets complicated, because the nickname
  // can be set by the user, or it can be default for the wrapper, or it can be
  // derived from the filename. The nicknames are used in the following order.
  // - if there is a custom nickname, it is shown
  // - if there is a filename, nickname is set to the shortened filename
  // - if there is no filename, nickname is set to the default nickname
  std::string m_DefaultNickname, m_CustomNickname;

  // Tags for this image layer
  TagList m_Tags;

  bool m_Initialized = false;

  // Transparency
  double m_Alpha;

  // Solid color
  Vector3d m_SolidColor;

  // Opacity for display on slice views
  double m_SliceViewOpacity = 1.0;

  // Data Array Property Id
  int m_CombinedPropID = 0;

  // Meta Data Map
  MetaDataMap m_MetaDataMap;

	// Data property observer tag
	unsigned long m_ActiveMeshDataPropertyObserverTag;

  // Display geometry for slicing
  DisplaySlicePipelineArray<DisplayViewportGeometryType, const DisplayViewportGeometryType *> m_DisplayViewportGeometry;
};

#endif // MESHWRAPPERBASE_H
