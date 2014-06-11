/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2014 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/


#include <osgUI/ComboBox>
#include <osgText/String>
#include <osgText/Font>
#include <osgText/Text>
#include <osg/Notify>
#include <osg/ValueObject>
#include <osg/io_utils>

using namespace osgUI;

ComboBox::ComboBox():
    _currentItem(0)
{
}

ComboBox::ComboBox(const osgUI::ComboBox& combobox, const osg::CopyOp& copyop):
    Widget(combobox, copyop),
    _items(combobox._items)
{
}

bool ComboBox::handleImplementation(osgGA::EventVisitor* ev, osgGA::Event* event)
{
    // OSG_NOTICE<<"ComboBox::handleImplementation"<<std::endl;

    osgGA::GUIEventAdapter* ea = event->asGUIEventAdapter();
    if (!ea) return false;

    switch(ea->getEventType())
    {
        case(osgGA::GUIEventAdapter::SCROLL):
            if (ea->getScrollingMotion()==osgGA::GUIEventAdapter::SCROLL_DOWN)
            {
                if (getCurrentItem()<getNumItems()-1) setCurrentItem(getCurrentItem()+1);
                return true;
            }
            else if (ea->getScrollingMotion()==osgGA::GUIEventAdapter::SCROLL_UP)
            {
                if (getCurrentItem()>0) setCurrentItem(getCurrentItem()-1);
                return true;
            }
            break;

        case(osgGA::GUIEventAdapter::KEYDOWN):
            if (ea->getKey()==osgGA::GUIEventAdapter::KEY_Down)
            {
                if (getCurrentItem()<getNumItems()-1) setCurrentItem(getCurrentItem()+1);
                return true;
            }
            else if (ea->getKey()==osgGA::GUIEventAdapter::KEY_Up)
            {
                if (getCurrentItem()>0) setCurrentItem(getCurrentItem()-1);
                return true;
            }

            break;

        case(osgGA::GUIEventAdapter::PUSH):
        {
            OSG_NOTICE<<"Button pressed "<<std::endl;
            // toggle visibility of popup.
            osgUI::Widget::Intersections intersections;
            osgGA::GUIActionAdapter* aa = ev ? ev->getActionAdapter() : 0;
            osgGA::GUIEventAdapter* ea = event ? event->asGUIEventAdapter() : 0;
//           if ((aa && ea) && aa->computeIntersections(*ea, ev->getNodePath(), intersections))
            if ((aa && ea) && computeIntersections(ev, ea, intersections))
            {
                OSG_NOTICE<<"ComboBox intersections { "<<std::endl;
                for(osgUI::Widget::Intersections::const_iterator itr =intersections.begin();
                    itr!=intersections.end();
                    ++itr)
                {
                        const osgUtil::LineSegmentIntersector::Intersection& hit = *itr;
                        OSG_NOTICE<<"   hit:drawable "<<hit.drawable.get()<<", "<<hit.drawable->getName()<<std::endl;
                        OSG_NOTICE<<"   NodePath::size() "<<hit.nodePath.size()<<std::endl;
                }
                OSG_NOTICE<<"}"<<std::endl;

                const osgUtil::LineSegmentIntersector::Intersection& hit = *intersections.begin();
                osg::Vec3d localPosition = hit.getLocalIntersectPoint();
                if (_extents.contains(localPosition, 1e-6))
                {
                    OSG_NOTICE<<"ComboBox button"<<std::endl;
                    _popup->setVisible(!_popup->getVisible());
                }

                if (_popup->getVisible() && _popup->getExtents().contains(localPosition, 1e-6))
                {
                    OSG_NOTICE<<"In pop up"<<std::endl;
                    OSG_NOTICE<<"   hit:drawable "<<hit.drawable.get()<<std::endl;
                    OSG_NOTICE<<"   NodePath::size() "<<hit.nodePath.size()<<std::endl;

                    unsigned int index=_items.size();
                    for(osg::NodePath::const_reverse_iterator itr = hit.nodePath.rbegin();
                        itr != hit.nodePath.rend();
                        ++itr)
                    {
                        if ((*itr)==this) break;
                        if ((*itr)->getUserValue("index",index)) break;
                    }

                    if (index<_items.size())
                    {
                        OSG_NOTICE<<"   index selected "<<index<<std::endl;
                        setCurrentItem(index);
                    }
                    else
                    {
                        OSG_NOTICE<<"   No index selected "<<std::endl;
                    }

                    _popup->setVisible(false);
                }
            }
            break;
        }
        case(osgGA::GUIEventAdapter::RELEASE):
            OSG_NOTICE<<"Button release "<<std::endl;
            break;

        default:
            break;
    }

    return false;
}

void ComboBox::enterImplementation()
{
    OSG_NOTICE<<"ComboBox enter"<<std::endl;
}


void ComboBox::leaveImplementation()
{
    OSG_NOTICE<<"ComboBox leave"<<std::endl;
}

void ComboBox::setCurrentItem(unsigned int i)
{
    OSG_NOTICE << "ComboBox::setCurrentItem("<<i<<")"<<std::endl;
    _currentItem = i;
    if (_buttonSwitch.valid()) _buttonSwitch->setSingleChildOn(_currentItem);
}

void ComboBox::createGraphicsImplementation()
{
    Style* style = (getStyle()!=0) ? getStyle() : Style::instance().get();

    _buttonSwitch = new osg::Switch;
    _popup = new osgUI::Popup;
    _popup->setVisible(false);

    if (!_items.empty())
    {

        float margin = (_extents.yMax()-_extents.yMin())*0.1f;
        float itemWidth = (_extents.xMax()-_extents.xMin())-2.0f*margin;
        float itemHeight = (_extents.yMax()-_extents.yMin())-margin;
        float popupHeight = itemHeight * _items.size() + 2.0f*margin;
        float popupTop = _extents.yMin()-margin;

        osg::BoundingBox popupExtents(_extents.xMin(), popupTop-popupHeight, _extents.zMin(), _extents.xMin()+itemWidth+2*margin, popupTop, _extents.zMax());
        _popup->setExtents(popupExtents);

        osg::BoundingBox popupItemExtents(_extents.xMin()+margin, popupTop-margin-itemHeight, _extents.zMin(), _extents.xMin()+itemWidth, popupTop-margin, _extents.zMax());

        unsigned int index = 0;
        for(Items::iterator itr = _items.begin();
            itr != _items.end();
            ++itr, ++index)
        {
            Item* item = itr->get();
            OSG_NOTICE<<"Creating item "<<item->getText()<<", "<<item->getColor()<<std::endl;

            // setup graphics for button
            {
                osg::ref_ptr<osg::Group> group = new osg::Group;
                if (item->getColor().a()!=0.0f) group->addChild( style->createPanel(_extents, item->getColor()) );
                if (!item->getText().empty()) group->addChild( style->createText(_extents, getAlignmentSettings(), getTextSettings(), item->getText()) );
                _buttonSwitch->addChild(group.get());
            }

            // setup graphics for popup
            {
                osg::ref_ptr<osg::Group> group = new osg::Group;
                group->setUserValue("index",index);

                if (item->getColor().a()!=0.0f) group->addChild( style->createPanel(popupItemExtents, item->getColor()) );
                if (!item->getText().empty()) group->addChild( style->createText(popupItemExtents, getAlignmentSettings(), getTextSettings(), item->getText()) );
                _popup->addChild(group.get());

                popupItemExtents.yMin() -= (itemHeight+margin);
                popupItemExtents.yMax() -= (itemHeight+margin);
            }

        }

    }
    else
    {
        _buttonSwitch->addChild( style->createPanel(_extents, osg::Vec4(1.0f,1.0f,1.0f,1.0f)) );
    }

    _buttonSwitch->setSingleChildOn(_currentItem);

    style->setupClipStateSet(_extents, getOrCreateStateSet());
    setGraphicsSubgraph(0, _buttonSwitch.get());
    addChild(_popup.get());
}
