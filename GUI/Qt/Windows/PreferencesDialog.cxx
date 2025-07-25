#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "DefaultBehaviorSettings.h"
#include "DeepLearningSegmentationModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include "ColorMapModel.h"

#include <QTreeView>
#include <QPushButton>
#include <QtAbstractItemViewCoupling.h>
#include <QInputDialog>
#include <QMessageBox>
#include <QtConcurrent>

Q_DECLARE_METATYPE(GlobalDisplaySettings::UIGreyInterpolation)
Q_DECLARE_METATYPE(SNAPAppearanceSettings::UIElements)
Q_DECLARE_METATYPE(LayerLayout)


PreferencesDialog::PreferencesDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::PreferencesDialog)
{
  ui->setupUi(this);

  // Set up list of interpolation modes
  ui->inInterpolationMode->clear();
  ui->inInterpolationMode->addItem("Nearest Neighbor",
                                   QVariant::fromValue(GlobalDisplaySettings::NEAREST));
  ui->inInterpolationMode->addItem("Linear", QVariant::fromValue(GlobalDisplaySettings::LINEAR));

  // Set up layoyt options
  ui->inOverlayLayout->clear();
  ui->inOverlayLayout->addItem(
    QIcon(":/root/layout_thumb_16.png"), "Stack", QVariant::fromValue(LAYOUT_STACKED));
  ui->inOverlayLayout->addItem(
    QIcon(":/root/layout_tile_16.png"), "Tile", QVariant::fromValue(LAYOUT_TILED));

  // Set up tree of appearance elements
  QStandardItemModel *model = new QStandardItemModel();

  QStandardItem *itemSliceViews = append_category_item(model->invisibleRootItem(), "Slice Views");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::BACKGROUND_2D, "Background");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::CROSSHAIRS, "Crosshair");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::RULER, "Rulers");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::MARKERS, "Anatomic Markers");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::ROI_BOX, "ROI Edges");
  append_appearance_item(
    itemSliceViews, SNAPAppearanceSettings::ROI_BOX_ACTIVE, "ROI Edges (selected)");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::PAINTBRUSH_OUTLINE, "Paintbrush");
  append_appearance_item(itemSliceViews, SNAPAppearanceSettings::GRID_LINES, "Deformation Grid");

  QStandardItem *item3DView = append_category_item(model->invisibleRootItem(), "3D View");
  append_appearance_item(item3DView, SNAPAppearanceSettings::BACKGROUND_3D, "Background");
  append_appearance_item(item3DView, SNAPAppearanceSettings::CROSSHAIRS_3D, "Crosshair");
  append_appearance_item(item3DView, SNAPAppearanceSettings::MESH_OUTLINE, "Mesh Outline");

  QStandardItem *itemThumb = append_category_item(model->invisibleRootItem(), "Zoom Thumbnail");
  append_appearance_item(itemThumb, SNAPAppearanceSettings::ZOOM_THUMBNAIL, "Thumbnail");
  append_appearance_item(itemThumb, SNAPAppearanceSettings::ZOOM_VIEWPORT, "Current Viewport Outline");
  append_appearance_item(itemThumb, SNAPAppearanceSettings::CROSSHAIRS_THUMB, "Crosshair");

  QStandardItem *itemPoly = append_category_item(model->invisibleRootItem(), "Polygon Tool");
  append_appearance_item(itemPoly, SNAPAppearanceSettings::POLY_DRAW_MAIN, "Outline (drawing)");
  append_appearance_item(itemPoly, SNAPAppearanceSettings::POLY_DRAW_CLOSE, "Completion line");
  append_appearance_item(itemPoly, SNAPAppearanceSettings::POLY_EDIT, "Outline (editing)");
  append_appearance_item(
    itemPoly, SNAPAppearanceSettings::POLY_EDIT_SELECT, "Outline (editing, selected)");

  QStandardItem *itemReg = append_category_item(model->invisibleRootItem(), "Registration Tool");
  append_appearance_item(itemReg, SNAPAppearanceSettings::REGISTRATION_WIDGETS, "Registration Widgets");
  append_appearance_item(
    itemReg, SNAPAppearanceSettings::REGISTRATION_WIDGETS_ACTIVE, "Registration Widgets (active)");
  append_appearance_item(itemReg, SNAPAppearanceSettings::REGISTRATION_GRID, "Registration Grid Lines");

  ui->treeVisualElements->setModel(model);
  ui->treeVisualElements->expandAll();

  // Create slice layout view icon paths
  for (int i = 0; i < 3; ++i)
    m_SliceLayoutPixmapPaths[i].SetDialog(this);

  // Set the correct page
  ui->stack->setCurrentIndex(0);
}

PreferencesDialog::~PreferencesDialog() { delete ui; }

void
PreferencesDialog::SetModel(GlobalPreferencesModel *model)
{
  // Copy the model
  m_Model = model;

  // Hook up the default behavior settings
  DefaultBehaviorSettings *dbs = m_Model->GetDefaultBehaviorSettings();
  makeCoupling(ui->chkLinkedZoom, dbs->GetLinkedZoomModel());
  makeCoupling(ui->chkContinuousUpdate, dbs->GetContinuousMeshUpdateModel());
  makeCoupling(ui->chkSynchronize, dbs->GetSynchronizationModel());
  makeCoupling(ui->chkSyncCursor, dbs->GetSyncCursorModel());
  makeCoupling(ui->chkSyncZoom, dbs->GetSyncZoomModel());
  makeCoupling(ui->chkSyncPan, dbs->GetSyncPanModel());
  makeCoupling(ui->chkCheckForUpdates, m_Model->GetCheckForUpdateModel());
  makeCoupling(ui->chkAutoContrast, dbs->GetAutoContrastModel());

  // Hook up the display layout properties
  GlobalDisplaySettings *gds = m_Model->GetGlobalDisplaySettings();
  makeRadioGroupCoupling(
    ui->radio_sagittal_ap, ui->radio_sagittal_pa, gds->GetFlagLayoutPatientAnteriorShownLeftModel());
  makeRadioGroupCoupling(
    ui->radio_axial_rl, ui->radio_axial_lr, gds->GetFlagLayoutPatientRightShownLeftModel());
  makeCoupling(ui->chkRemindLayout, gds->GetFlagRemindLayoutSettingsModel());

  // Initialize Dialog States
  m_IsPatientsRightShownLeft = gds->GetFlagLayoutPatientRightShownLeft();
  m_IsAnteriorShownLeft = gds->GetFlagLayoutPatientAnteriorShownLeft();

  // Layout radio buttons
  std::map<GlobalDisplaySettings::UISliceLayout, QAbstractButton *> btnmap;
  btnmap[GlobalDisplaySettings::LAYOUT_ACS] = ui->btnACS;
  btnmap[GlobalDisplaySettings::LAYOUT_ASC] = ui->btnASC;
  btnmap[GlobalDisplaySettings::LAYOUT_CAS] = ui->btnCAS;
  btnmap[GlobalDisplaySettings::LAYOUT_CSA] = ui->btnCSA;
  btnmap[GlobalDisplaySettings::LAYOUT_SAC] = ui->btnSAC;
  btnmap[GlobalDisplaySettings::LAYOUT_SCA] = ui->btnSCA;
  makeRadioGroupCoupling(ui->grpLayoutRadio, btnmap, gds->GetSliceLayoutModel());

  // The outview pixmaps for different layouts
  CutPlane cps[3];
  for (int i = 0; i < 3; ++i)
  {
    // get the lower-cased string of cutplanes for each outview
    std::string str = m_Model->GetLayoutLabelModel(i)->GetValue();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    cps[i] = m_StringToCutPlaneMap[str];
  }
  setOutViewCutPlane(cps[0], cps[1], cps[2]);

  makeCoupling(ui->chkShowThumbnail, gds->GetFlagDisplayZoomThumbnailModel());
  makeCoupling(ui->inThumbnailFraction, gds->GetZoomThumbnailSizeInPercentModel());
  makeCoupling(ui->inThumbnailMaxSize, gds->GetZoomThumbnailMaximumSizeModel());

  // Couple the interpolation mode (the domain is not provided by the model)
  makeCoupling(ui->inInterpolationMode, gds->GetGreyInterpolationModeModel());

  // Couple the layer layout model
  makeCoupling(ui->inOverlayLayout, gds->GetLayerLayoutModel());

  // Couple the color map preset selection.
  UpdateColorMapPresets();
  makeCoupling(ui->inDefaultColorMap, dbs->GetOverlayColorMapPresetModel());

  // We also want to monitor changes to the color map presets. If these changes
  // occur, we have to modify the list of presets.
  LatentITKEventNotifier::connect(m_Model->GetParentModel()->GetColorMapModel(),
                                  ColorMapModel::PresetUpdateEvent(),
                                  this,
                                  SLOT(onModelUpdate(EventBucket)));

  // Couple the list of apperance elements. First we m
  makeCoupling((QAbstractItemView *)ui->treeVisualElements, m_Model->GetActiveUIElementModel());

  // Hook up the appearance widgets
  OpenGLAppearanceElement *elt = m_Model->GetActiveUIElementAppearance();
  QtCouplingOptions        opts_elt(QtCouplingOptions::DEACTIVATE_WHEN_INVALID);
  makeCoupling(ui->chkElementVisible, elt->GetVisibilityFlagModel(), opts_elt);
  makeCoupling(ui->btnElementColor, elt->GetColorModel(), opts_elt);
  makeCoupling(ui->inElementOpacity, elt->GetAlphaModel(), opts_elt);
  makeCoupling(ui->inElementThickness, elt->GetLineThicknessModel(), opts_elt);
  makeCoupling(ui->inElementLineType, elt->GetLineTypeModel(), opts_elt);
  makeCoupling(ui->inElementFontSize, elt->GetFontSizeModel(), opts_elt);

  // Make sure the labels are activated along with the widgets
  activateOnFlag(ui->labelElementColor, elt->GetColorModel(), UIF_PROPERTY_IS_VALID);
  activateOnFlag(ui->labelElementOpacity, elt->GetColorModel(), UIF_PROPERTY_IS_VALID);
  activateOnFlag(ui->labelElementLineThickness, elt->GetLineThicknessModel(), UIF_PROPERTY_IS_VALID);
  activateOnFlag(ui->labelElementLineType, elt->GetLineTypeModel(), UIF_PROPERTY_IS_VALID);
  activateOnFlag(ui->labelElementFontSize, elt->GetFontSizeModel(), UIF_PROPERTY_IS_VALID);

  // Hook up activation for the appearance panel
  activateOnFlag(ui->grpAppearance, m_Model, GlobalPreferencesModel::UIF_VALID_UI_ELEMENT_SELECTED);

  // Hook up the mesh options
  MeshOptions *mo = m_Model->GetMeshOptions();

  makeCoupling(ui->chkGaussianSmooth, mo->GetUseGaussianSmoothingModel());
  makeCoupling(ui->inGaussianSmoothDeviation, mo->GetGaussianStandardDeviationModel());
  makeCoupling(ui->inGaussianSmoothMaxError, mo->GetGaussianErrorModel());

  makeCoupling(ui->chkMeshSmooth, mo->GetUseMeshSmoothingModel());
  makeCoupling(ui->inMeshSmoothConvergence, mo->GetMeshSmoothingConvergenceModel());
  makeCoupling(ui->inMeshSmoothFeatureAngle, mo->GetMeshSmoothingFeatureAngleModel());
  makeCoupling(ui->inMeshSmoothIterations, mo->GetMeshSmoothingIterationsModel());
  makeCoupling(ui->inMeshSmoothRelaxation, mo->GetMeshSmoothingRelaxationFactorModel());
  makeCoupling(ui->chkMeshSmoothBoundarySmoothing, mo->GetMeshSmoothingBoundarySmoothingModel());
  makeCoupling(ui->chkMeshSmoothFeatureEdgeSmoothing, mo->GetMeshSmoothingFeatureEdgeSmoothingModel());

  makeCoupling(ui->chkDecimate, mo->GetUseDecimationModel());
  makeCoupling(ui->inDecimateFeatureAngle, mo->GetDecimateFeatureAngleModel());
  makeCoupling(ui->inDecimateMaxError, mo->GetDecimateMaximumErrorModel());
  makeCoupling(ui->inDecimateTargetReduction, mo->GetDecimateTargetReductionModel());
  makeCoupling(ui->chkDecimatePreserveTopology, mo->GetDecimatePreserveTopologyModel());

  // Tool page
  makeCoupling(ui->inPaintBrushMaxSize, dbs->GetPaintbrushDefaultMaximumSizeModel());
  makeCoupling(ui->inPaintBrushInitSize, dbs->GetPaintbrushDefaultInitialSizeModel());

  // Deep learning service
  auto *dlm = m_Model->GetParentModel()->GetDeepLearningSegmentationModel();
  ui->pageDL->SetModel(dlm);
}

void
PreferencesDialog::ShowDialog()
{
  if (!this->isVisible())
  {
    m_Model->InitializePreferences();
    ui->listWidget->setCurrentRow(0);
    this->show();
  }

  this->activateWindow();
  this->raise();
}

void
PreferencesDialog::GoToPage(enum PreferencesDialogPage page)
{
  ui->listWidget->setCurrentRow(page);
}

void
PreferencesDialog::set_page_to_general_default_behavior()
{
  this->GoToPage(General);
  ui->tabWidgetGeneral->setCurrentWidget(ui->tabGeneralDefault);
}

void
PreferencesDialog::set_page_to_general_default_permissions()
{
  this->GoToPage(General);
  ui->tabWidgetGeneral->setCurrentWidget(ui->tabGeneralPermissions);
}

void
PreferencesDialog::set_page_to_slice_views_display()
{
  this->GoToPage(SliceView);
  ui->tabWidgetSliceViews->setCurrentWidget(ui->tabSliceViewsDisplay);
}

void
PreferencesDialog::set_page_to_slice_views_layout()
{
  this->GoToPage(SliceView);
  ui->tabWidgetSliceViews->setCurrentWidget(ui->tabSliceViewsLayout);
}

void
PreferencesDialog::set_page_to_appearance()
{
  this->GoToPage(Appearance);
}

void
PreferencesDialog::set_page_to_3d_smoothing()
{
  this->GoToPage(Rendering3D);
  ui->tabWidgetRendering->setCurrentWidget(ui->tabRenderingSmoothing);
}

void
PreferencesDialog::set_page_to_3d_decimation()
{
  this->GoToPage(Rendering3D);
  ui->tabWidgetRendering->setCurrentWidget(ui->tabRenderingDecimation);
}

void
PreferencesDialog::set_page_to_tools()
{
  this->GoToPage(Tools);
}

void
PreferencesDialog::set_page_to_dls()
{
  this->GoToPage(DeepLearningServer);
}

void
PreferencesDialog::on_listWidget_itemSelectionChanged()
{
  // Select the right page in the right pane
  QItemSelectionModel *selm = ui->listWidget->selectionModel();
  int                  row = selm->currentIndex().row();
  ui->stack->setCurrentIndex(row);
  ui->outPage->setText(ui->listWidget->item(row)->text());
}

void
PreferencesDialog::on_buttonBox_clicked(QAbstractButton *button)
{
  if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
  {
    m_Model->ApplyPreferences();
  }
  else if (button == ui->buttonBox->button(QDialogButtonBox::Ok))
  {
    this->accept();
  }
}

void
PreferencesDialog::on_btnElementReset_clicked()
{
  m_Model->ResetCurrentElement();
}

void
PreferencesDialog::on_btnElementResetAll_clicked()
{
  m_Model->ResetAllElements();
}

// Methods
void
PreferencesDialog::on_btnASC_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Axial, Sagittal, Coronal);
}
void
PreferencesDialog::on_btnACS_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Axial, Coronal, Sagittal);
}
void
PreferencesDialog::on_btnCAS_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Coronal, Axial, Sagittal);
}
void
PreferencesDialog::on_btnCSA_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Coronal, Sagittal, Axial);
}
void
PreferencesDialog::on_btnSAC_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Sagittal, Axial, Coronal);
}
void
PreferencesDialog::on_btnSCA_toggled(bool check)
{
  if (check)
    setOutViewCutPlane(Sagittal, Coronal, Axial);
}

void
PreferencesDialog::on_radio_axial_lr_toggled(bool check)
{
  m_IsPatientsRightShownLeft = check ? false : true;
  UpdateOutViewPixmaps();
}

void
PreferencesDialog::on_radio_sagittal_ap_toggled(bool check)
{
  m_IsAnteriorShownLeft = check ? true : false;
  UpdateOutViewPixmaps();
}

void
PreferencesDialog::setOutViewCutPlane(enum CutPlane topleft,
                                      enum CutPlane topright,
                                      enum CutPlane bottomright)
{
  m_SliceLayoutPixmapPaths[0].m_cp = topleft;
  m_SliceLayoutPixmapPaths[1].m_cp = topright;
  m_SliceLayoutPixmapPaths[2].m_cp = bottomright;

  UpdateOutViewPixmaps();
}

void
PreferencesDialog::UpdateOutViewPixmaps()
{
  ui->outViewTopLeft->setPixmap(m_SliceLayoutPixmapPaths[0].GetPath());
  ui->outViewTopRight->setPixmap(m_SliceLayoutPixmapPaths[1].GetPath());
  ui->outViewBottomRight->setPixmap(m_SliceLayoutPixmapPaths[2].GetPath());
}

void
PreferencesDialog::onModelUpdate(const EventBucket &bucket)
{
  if (bucket.HasEvent(ColorMapModel::PresetUpdateEvent()))
  {
    // Update the presets
    UpdateColorMapPresets();

    // TODO: what to do if the preset was deleted?
  }


}


void
PreferencesDialog::UpdateColorMapPresets()
{
  ColorMapModel *cmm = m_Model->GetParentModel()->GetColorMapModel();
  PopulateColorMapPresetCombo(ui->inDefaultColorMap, cmm);
}

QStandardItem *
PreferencesDialog::append_appearance_item(QStandardItem                     *parent,
                                          SNAPAppearanceSettings::UIElements elt,
                                          const QString                     &text)
{
  QStandardItem *item = new QStandardItem(text);
  item->setData(QVariant::fromValue(elt), Qt::UserRole);
  parent->appendRow(item);
  return item;
}

QStandardItem *
PreferencesDialog::append_category_item(QStandardItem *parent, const QString &text)
{
  QStandardItem *item = new QStandardItem(text);
  item->setData(QVariant::fromValue(SNAPAppearanceSettings::ELEMENT_COUNT), Qt::UserRole);
  item->setSelectable(false);
  parent->appendRow(item);
  return item;
}


void
PreferencesDialog::on_PreferencesDialog_accepted()
{
  m_Model->ApplyPreferences();
}
