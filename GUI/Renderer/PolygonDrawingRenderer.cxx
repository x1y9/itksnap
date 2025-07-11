#include "PolygonDrawingRenderer.h"
#include "PolygonDrawingModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"


void
PolygonDrawingRenderer::DrawVertices(AbstractRenderContext              *context,
                                        const PolygonDrawingModel::VertexList &vx,
                                        bool                                   closed)
{
  std::vector<Vector2d> polyline;
  polyline.reserve(vx.size());
  for (auto it : vx)
    polyline.push_back(Vector2d(it.x, it.y));

  context->DrawPolyLine(polyline);
  // auto path = context->CreatePath();
  // context->AddPolygonSegmentToPath(path, polyline, closed);
  // context->DrawPath(path);
}

void
PolygonDrawingRenderer::DrawSelectionBox(AbstractRenderContext           *context,
                                            const PolygonDrawingModel::BoxType &box,
                                            double                              border_x,
                                            double                              border_y)
{
  double x0 = box[0] - border_x, y0 = box[2] - border_y;
  double x1 = box[1] + border_x, y1 = box[3] + border_y;
  context->DrawRect(x0, y0, x1-x0, y1-y0);
}


void
PolygonDrawingRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                                ImageWrapperBase         *base_layer,
                                                const SubViewport        &vp)
{
  if(vp.isThumbnail)
    return;

  // Pointers to the polygon model and the parent model
  auto *parentModel = this->GetModel()->GetParent();

  // Polygon drawing state
  PolygonDrawingModel::PolygonState state = m_Model->GetState();

  // Must be in active state
  if (state == PolygonDrawingModel::INACTIVE_STATE)
    return;

  // Get appearance settings, etc
  auto *as = parentModel->GetParentUI()->GetAppearanceSettings();

  // Get appearance settings for the drawing
  auto *aeDraw = as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_MAIN);
  auto *aeClose = as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_CLOSE);
  auto *aeEdit = as->GetUIElement(SNAPAppearanceSettings::POLY_EDIT);
  auto *aeEditSelect = as->GetUIElement(SNAPAppearanceSettings::POLY_EDIT_SELECT);

  // Draw the line segments
  const PolygonDrawingModel::VertexList &vx = m_Model->GetVertices();
  const PolygonDrawingModel::VertexList &dvx = m_Model->GetDragVertices();

  // Useful iterators
  PolygonDrawingModel::VertexList::const_iterator it, itNext;

  if (state == PolygonDrawingModel::EDITING_STATE)
  {
    // TODO: store the paths between calls to render and use modification time
    // Store vertices of selected and unselected line segments
    auto path_selected = context->CreatePath(), path_unselected = context->CreatePath();
    std::vector<Vector2d> segment;
    segment.reserve(vx.size());

    bool segment_sel = false, have_sel = false, have_unsel = false;
    for (it = vx.begin(); it != vx.end(); ++it)
    {
      // Point to the next vertex, circular
      itNext = it;
      ++itNext;
      if (itNext == vx.end())
        itNext = vx.begin();

      bool interval_sel = it->selected && itNext->selected;
      if (segment.size() >= 2 && (interval_sel != segment_sel))
      {
        // We need to add the current segment to the path and start a new segment
        if (segment_sel)
        {
          context->AddPolygonSegmentToPath(path_selected, segment, false);
          have_sel = true;
        }
        else
        {
          context->AddPolygonSegmentToPath(path_unselected, segment, false);
          have_unsel = true;
        }
        segment.clear();
      }
      // We can append the current interval to the segment
      if (segment.size() == 0)
        segment.push_back(Vector2d(it->x, it->y));
      segment.push_back(Vector2d(itNext->x, itNext->y));
      segment_sel = interval_sel;
    }

    if (segment.size() >= 2)
    {
      // We need to add the current segment to the path and start a new segment
      if (segment_sel)
      {
        context->AddPolygonSegmentToPath(path_selected, segment, false);
        have_sel = true;
      }
      else
      {
        context->AddPolygonSegmentToPath(path_unselected, segment, false);
        have_unsel = true;
      }
    }

    // Draw the selected and unselected segments
    if(have_sel)
    {
      context->SetPenAppearance(*aeEditSelect);
      context->BuildPath(path_selected);
      context->DrawPath(path_selected);
    }
    if(have_unsel)
    {
      context->SetPenAppearance(*aeEdit);
      context->BuildPath(path_unselected);
      context->DrawPath(path_unselected);
    }
  }
  else
  {
    // Not editing state
    context->SetPenAppearance(*aeDraw);

    // Draw polyline
    this->DrawVertices(context, vx, false);

    // Draw the drag vertices
    this->DrawVertices(context, dvx, false);

    // If hovering over the last point, draw the closing line using
    // current appearance settings
    if(m_Model->IsHoverOverFirstVertex())
    {
      auto &last_vtx = dvx.size() ? dvx.back() : vx.back();
      context->DrawLine(last_vtx.x, last_vtx.y, vx.front().x, vx.front().y);
    }

    else if(dvx.size() + vx.size() > 2 && aeClose->GetVisible())
    {
      // Draw the stripped line.
      context->SetPenAppearance(*aeClose);
      auto &last_vtx = dvx.size() ? dvx.back() : vx.back();
      context->DrawLine(last_vtx.x, last_vtx.y, vx.front().x, vx.front().y);
    }
  }

  // draw the vertices as points
  for(it = vx.begin(); it!=vx.end();++it)
  {
    if(it->control)
    {
      auto *elt = it->selected
                    ? aeEditSelect
                    : (state == PolygonDrawingModel::DRAWING_STATE
                         ? aeDraw
                         : aeEdit);
      context->SetPenAppearance(*elt);
      context->SetPenWidth(elt->GetLineThickness() * 2);
      context->DrawPoint(it->x, it->y);
    }
  }

  // Draw the last dragging vertex point
  if(dvx.size())
  {
    context->SetPenColor(aeEdit->GetColor());
    context->DrawPoint(dvx.back().x, dvx.back().y);
  }

  // draw edit or pick box
  if(m_Model->IsDraggingPickBox())
  {
    auto &box = m_Model->GetSelectionBox();
    context->SetPenWidth(1);
    context->SetPenColor(aeEditSelect->GetColor());
    DrawSelectionBox(context, box, 0, 0);
  }
  else if (m_Model->GetSelectedVertices())
  {
    auto &box = m_Model->GetEditBox();
    Vector2d border = m_Model->GetPixelSize() * 4.0;

    context->SetPenWidth(1);
    context->SetPenColor(aeEditSelect->GetColor());
    DrawSelectionBox(context, box, border[0], border[1]);
  }
}
